# Apple OSX Installation  

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM.

The simplest method of installation uses the [Homebrew package manager](http://brew.sh/):

 1. Install XCode 6.0+ via the AppStore
 2. Install the [Homebrew package manager](http://brew.sh/) via terminal and confirm it's working with ```$ brew doctor```

Then, to install librealsense, you can then install either the latest release of librealsense:

    $ brew install librealsense

Or you can install the head version from Github:

    $ brew install librealsense --HEAD

To install with examples, add the ```--with-examples``` flag. Use ```$ brew info librealsense``` to retrieve the location of the installation.

Alternatively, if you would like to compile librealsense from source:

    $ brew install cmake pkg-config libusb
    $ git clone https://github.com/IntelRealSense/librealsense.git
    $ cd librealsense
    $ cmake .
    $ make && make install
