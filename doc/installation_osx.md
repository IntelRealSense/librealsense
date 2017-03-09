# Apple OSX Installation  

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM.

The simplest method of installation uses the [Homebrew package manager](http://brew.sh/). First install [Homebrew](http://brew.sh/) via terminal and confirm it's working with ```$ brew doctor```

## Installation via Homebrew

To install librealsense, you can then install either the latest release of librealsense:

    $ brew install librealsense

Or you can install the head version from GitHub:

    $ brew install librealsense --HEAD

To install with examples, add the ```--with-examples``` flag. Use ```$ brew info librealsense``` to retrieve the location of the installation.

## Installation from source

If you would like to compile librealsense from source:

    $ brew install cmake pkg-config libusb glfw
    $ git clone https://github.com/IntelRealSense/librealsense.git
    $ cd librealsense
    $ mkdir build && cd build

For the default build:

    $ cmake ../

Alternatively, to also build examples:

    $ cmake ../ -DBUILD_EXAMPLES=true

Finally, generate and install binaries:

    $ make && make install
