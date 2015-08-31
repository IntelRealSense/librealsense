CC ?= gcc
CXX ?= g++

CFLAGS := -std=c11 -fPIC -pedantic
CXXFLAGS := -std=c++11 -fPIC -pedantic -Wno-missing-field-initializers -Wno-switch

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
bin/c-capture: library examples/c-capture.c
	$(CC) examples/c-capture.c $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

bin/c-pointcloud: library examples/c-pointcloud.c
	$(CC) examples/c-pointcloud.c $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

bin/cpp-capture: library examples/cpp-capture.cpp
	$(CXX) examples/cpp-capture.cpp -std=c++11 $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

bin/cpp-multicam: library examples/cpp-multicam.cpp
	$(CXX) examples/cpp-multicam.cpp -std=c++11 $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

bin/cpp-pointcloud: library examples/cpp-pointcloud.cpp
	$(CXX) examples/cpp-pointcloud.cpp -std=c++11 $(REALSENSE_FLAGS) $(GLFW3_FLAGS) -o $@

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
