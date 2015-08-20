# librealsense

A library for capturing data with the RealSense F200 (IVCAM 1.0, 1.5) and RealSense R200 (DS4). It targets Mac OSX 10.7+ and "recent" versions of Linux (tested & developed on Ubuntu 14.04 LTS). While the OSX and Linux example apps depend on GLFW, the capture portion requires only libusb. Libusb-1.0 is a user-space USB driver: this means that librealsense requires no kernel patches. In the future, support will be extended to Android 5.1+ to route around the typical HAL layer for the purposes of accessing the camera directly from JNI/C++ code.

Dependency management for GLFW and Libusb-1.0 is done manually at the moment, pending the creation of installer scripts to automate the process. 

## Functionality

The goal of librealsense is to provide a reasonable hardware abstraction with minimal dependencies. It is not a computer vision SDK.

## Apple OSX Installation

1. Install XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install libusb via brew:
  * `brew install libusb`
4. Install glfw3 via brew:
  * `brew tap homebrew/versions`
  * `brew install versions/glfw3`

## Ubuntu 14.04+ Installation

1. Grant appropriate permissions to detach the kernel uvc driver when a device is plugged in:
  * `sudo cp 99-uvc.rules /etc/udev/rules.d/`
  * `sudo cp uvc.conf /etc/modprobe.d/uvc.conf/`
2. Ensure apt-get is up to date
  * `sudo apt-get update && apt-get upgrade`
3. Install appropriate dependencies via apt-get
  * `sudo apt-get install glfw3`
  * `suto apt-get install libusb-dev`

## License

For internal distribution only. 

