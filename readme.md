# librealsense

A libusb-based library for the RealSense F200 (IVCAM 1.0, 1.5) and RealSense R200 (DS4). It targets Apple OSX 10.7+ and "recent" versions of Linux (tested on Ubuntu 14.04). For cross-platform API development, it consumes DSAPI on Windows. 

Dependency management is done manually at the moment, pending the creation of installer scripts to automate the process. 

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
  * `suto apt-get install libusb-de`

