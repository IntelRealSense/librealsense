# librealsense

A libusb based driver for the RealSense F200 (IVCAM) and RealSense R200 (DS4). It works on OSX 10.7+ and "recent" versions of Linux (tested on Ubuntu 14.04). For cross-platform API development, it also consumes DSAPI on Windows. 

## Setup

1. Install XCode
2. Install XCode command line tools
3. Install Utilities: brew, git, sublime, chrome
4. Install libusb via brew

## Linux Setup
1. See "4.2: Running the Driver" http://wiki.ros.org/libuvc_camera
2. sudo cp 99-uvc.rules /etc/udev/rules.d/
3. sudo cp uvc.conf /etc/modprobe.d/uvc.conf/

