# Apple macOS Installation  

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM.

The simplest method of installation uses the [Homebrew package manager](http://brew.sh/). First install [Homebrew](http://brew.sh/) via terminal and confirm it's working with ```$ brew doctor```

## Installation via Homebrew

To install the legacy version of librealsense, you can then install either the latest legacy release:

    $ brew install librealsense@1

Or you can install the head version from the legacy branch of the GitHub repository:

    $ brew install librealsense@1 --HEAD

To install with examples, add the ```--with-examples``` flag. Use ```$ brew info librealsense@1``` to retrieve the location of the installation.

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
