# Detect OS and CPU
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
machine := $(shell sh -c "$(CC) -dumpmachine || echo unknown")

# Specify BACKEND=V4L2 or BACKEND=LIBUVC to build a specific backend
BACKEND := V4L2

ifeq ($(uname_S),Darwin)
# OSX defaults to libuvc instead of V4L
BACKEND := LIBUVC
endif

LIBUSB_FLAGS := `pkg-config --cflags --libs libusb-1.0`

CFLAGS := -std=c11 -fPIC -pedantic -DRS_USE_$(BACKEND)_BACKEND $(LIBUSB_FLAGS) 
CXXFLAGS := -std=c++11 -fPIC -pedantic -Ofast -Wno-missing-field-initializers
CXXFLAGS += -Wno-switch -Wno-multichar -DRS_USE_$(BACKEND)_BACKEND $(LIBUSB_FLAGS) 

# Add specific include paths for OSX
ifeq ($(uname_S),Darwin)
CFLAGS   += -I/usr/local/include
CXXFLAGS += -I/usr/local/include
endif

ifeq (arm-linux-gnueabihf,$(machine))
CXXFLAGS += -mfpu=neon -mfloat-abi=hard -ftree-vectorize
else
CXXFLAGS += -mssse3
endif

# Compute list of all *.o files that participate in librealsense.so
OBJECTS = verify 
OBJECTS += $(notdir $(basename $(wildcard src/*.cpp)))
OBJECTS += $(addprefix libuvc/, $(notdir $(basename $(wildcard src/libuvc/*.c))))
OBJECTS := $(addprefix obj/, $(addsuffix .o, $(OBJECTS)))

# Sets of flags used by the example programs
REALSENSE_FLAGS := -Iinclude -Llib -lrealsense -lm

ifeq ($(uname_S),Darwin)
# OSX uses OpenGL as a framework
GLFW3_FLAGS := `pkg-config --cflags --libs glfw3` -lglfw3 -framework OpenGL
else
# otherwise pkg-config finds OpenGL
GLFW3_FLAGS := `pkg-config --cflags --libs glfw3 glu gl`
endif

# Compute a list of all example program binaries
EXAMPLES := $(wildcard examples/*.c)
EXAMPLES += $(wildcard examples/*.cpp)
EXAMPLES := $(addprefix bin/, $(notdir $(basename $(EXAMPLES))))

# Aliases for convenience
all: examples $(EXAMPLES)

install: library
	install -m755 -d /usr/local/include/librealsense
	cp -r include/librealsense/* /usr/local/include/librealsense
	cp lib/librealsense.so /usr/local/lib
	ldconfig

uninstall:
	rm -rf /usr/local/include/librealsense
	rm /usr/local/lib/librealsense.so
	ldconfig

clean:
	rm -rf obj
	rm -rf lib
	rm -rf bin

library: lib/librealsense.so

prepare:
	mkdir -p obj/libuvc
	mkdir -p lib
	mkdir -p bin

# Rules for building the sample programs
bin/c-%: examples/c-%.c library
	$(CC) $< $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

bin/cpp-%: examples/cpp-%.cpp library
	$(CXX) $< -std=c++11 $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

# Rules for building the library itself
lib/librealsense.so: prepare $(OBJECTS)
	$(CXX) -std=c++11 -shared $(OBJECTS) $(LIBUSB_FLAGS) -o $@

lib/librealsense.a: prepare $(OBJECTS)
	ar rvs $@ `find obj/ -name "*.o"`
 
# Rules for compiling librealsense source
obj/%.o: src/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

# Rules for compiling libuvc source
obj/libuvc/%.o: src/libuvc/%.c
	$(CC) $< $(CFLAGS) -c -o $@

# Special rule to verify that rs.h can be included by a C89 compiler
obj/verify.o: src/verify.c
	$(CC) $< -std=c89 -Iinclude -c -o $@
