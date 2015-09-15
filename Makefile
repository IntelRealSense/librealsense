CFLAGS := -std=c11 -fPIC -pedantic
CXXFLAGS := -std=c++11 -fPIC -pedantic -O3 -Wno-missing-field-initializers -Wno-switch -Wno-multichar

# Compute list of all *.o files that participate in librealsense.so
OBJECTS = verify
OBJECTS += $(notdir $(basename $(wildcard src/*.cpp)))
OBJECTS += $(addprefix libuvc/, $(notdir $(basename $(wildcard src/libuvc/*.c))))
OBJECTS := $(addprefix obj/, $(addsuffix .o, $(OBJECTS)))

# Sets of flags used by the example programs
REALSENSE_FLAGS := -Iinclude -Llib -lrealsense -lm
GLFW3_FLAGS := `pkg-config --cflags --libs glfw3 gl glu`

# Compute a list of all example program binaries
EXAMPLES := $(wildcard examples/*.c)
EXAMPLES += $(wildcard examples/*.cpp)
EXAMPLES := $(addprefix bin/, $(notdir $(basename $(EXAMPLES))))

# Aliases for convenience
all: examples $(EXAMPLES)

install: library
	cp lib/librealsense.so /usr/local/lib
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
	$(CXX) -std=c++11 -shared $(OBJECTS) -lusb-1.0 -lpthread -o $@

# Rules for compiling librealsense source
obj/%.o: src/%.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

# Rules for compiling libuvc source
obj/libuvc/%.o: src/libuvc/%.c
	$(CC) $< $(CFLAGS) -c -o $@

# Special rule to verify that rs.h can be included by a C89 compiler
obj/verify.o: src/verify.c
	$(CC) $< -std=c89 -Iinclude -c -o $@
