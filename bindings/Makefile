default_target: all

all: python_bindings

python_bindings: pylibrs.so

pylibrs.so: ../build/librealsense.a python.cpp
	c++ -O3 -shared -std=c++11 $^ -o $@ `python-config --cflags --ldflags` `pkg-config --cflags --libs libusb`
