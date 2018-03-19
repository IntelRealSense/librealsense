# Apple macOS Installation  

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM.

First install the [Homebrew package manager](http://brew.sh/) via terminal and confirm it is working with ```$ brew doctor```

## Installation from source

If you would like to compile the legacy version of librealsense from source:

    $ brew install cmake pkg-config libusb glfw
    $ git clone https://github.com/IntelRealSense/librealsense.git
    $ cd librealsense
    $ git checkout legacy
    $ mkdir build && cd build

For the default build:

    $ cmake ../

Alternatively, to also build examples:

    $ cmake ../ -DBUILD_EXAMPLES=ON

Finally, generate and install binaries:

    $ make && make install
