# librealsense

A cross-platform library for capturing data from the RealSense F200, SR300 and R200 cameras. This effort is aimed at supporting prototyping efforts on new platforms and form-factors (robots, drones, VR, etc).

Dependency management for GLFW3 (example apps) and libusb-1.0 is performed through manual steps that are enumerated as part of this readme file (i.e. these packages must be installed through apt-get on Linux and Homebrew on OSX).

**Note:** On Linux, the libusb/libuvc backend has been deprecated in favor of the V4L2 backend.  

# Table of Contents
* [Supported Devices](#supported-devices)
* [Supported Platforms](#supported-platforms)
* [Supported Languages](#supported-languages-and-frameworks)
* [Functionality](#functionality)
* [Installation Guide](#installation-guide)
  * [Linux](#ubuntu-1404-lts-installation)
  * [OSX](#apple-osx)
  * [Windows](#windows-81)
* [FAQ](#faq)
* [Example Applications](#example-applications)
* [Developer Notes](#dev-notes)

## Supported Devices

1. RealSense R200
2. RealSense F200
3. RealSense SR300

## Supported Platforms

librealsense is written in standards-conforming C++11 and relies only on the C89 ABI for its public interface. It was developed and tested on the following platforms:

1. Windows 8.1 (Visual C++ 2013)
2. Ubuntu 14.04 LTS (gcc toolchain)
3. Mac OS X 10.7+ (clang toolchain)

Neither the libuvc or V4L2 backend has been validated on Ubuntu 12.04 LTS or Ubuntu 15.10, and several attempts to bring cameras up on these platforms have revealed underlying OS bugs and issues. It may be possible to compile and run librealsense on other platforms. Please let us know if you do, as well as any steps you found necessary to do so, so that we can update this list.

## Supported Languages and Frameworks

1. C - Core library API exposed via the C89 ABI
2. C++ - Single header file (rs.hpp) wrapper around C API, providing classes and exceptions
3. C# - Single source file (rs.cs) wrapper for P/Invoke to C API, providing classes and exceptions
4. Java - Source files (com/intel/rs/*.java + rs-jni.c) providing a JNI wrapper over C API, providing classes and exceptions, out parameters emulated via Out<T> wrapper class
	* Requires librealsense.so / realsense.dll to be compiled on a machine with the JDK installed
5. Python - Single source file (rs.py) wrapper using ctypes to access C API, providing classes and exceptions; out parameters converted to return values using Python tuples

Our intent is to provide bindings and wrappers for as many languages and frameworks as possible. Our core library exposes its functionality via C, and we intend for our various bindings and wrappers to look and feel "native" to their respective languages, for instance, using classes to represent objects, throwing exceptions to report errors, and following established naming conventions and casing styles of the language or framework in question.

## Functionality

1. Native streams: depth, color, infrared
2. Synthetic streams: rectified images, depth aligned to color and vice versa, etc.
3. Intrinsic/extrinsic calibration information
4. Majority of XU-exposed functionality for each camera
5. Multi-camera capture across heterogeneous camera architectures (e.g. mix R200 and F200 in same application) 

# Installation Guide

## Ubuntu 14.04 LTS Installation

1. Ensure apt-get is up to date
  * `sudo apt-get update && apt-get upgrade`
2. Install libusb-1.0 via apt-get
  * `sudo apt-get install libusb-1.0-0-dev`
3. glfw3 is not available in apt-get on Ubuntu 14.04. Use included installer script:
  * `scripts/install_glfw3.sh`
4. **Follow the installation instructions for your desired backend (see below)**
5. We use QtCreator as an IDE for Linux development on Ubuntu
  * **Note:** QtCreator is presently configured to use the V4L2 backend by default
  * `sudo apt-get install qtcreator`
  * `sudo scripts/install_qt.sh` (we also need qmake from the full qt5 distribution)
  * `all.pro` contains librealsense and all example applications
  * From the QtCreator top menu: Clean => Run QMake => Build
  * Built QtCreator projects will be placed into `./bin/debug` or `./bin/release`
6. We also provide a makefile if you'd prefer to use your own favorite text editor
  * `make && sudo make install`
  * The example executables will build into `./bin`

### Video4Linux backend

1. Ensure no cameras are presently plugged into the system.
2. Install udev rules 
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
3. Next, choose one of the following subheadings based on desired machine configuration / kernel version (and remember to complete step 4 after).
  * **Stock 3.19.xx Kernel in 14.04.03** 
    * Run the following script to patch uvcvideo.ko
        * `scripts/patch-uvcvideo-ko-generic.sh`
    * (R200 Only) Install connectivity workaround
      * `cd scripts && ./install-r200-udev-fix.sh`
      * This udev fix is not necessary for kernels >= 4.2.3
  * **(OR) Updated 4.2.3 Unstable Kernel**
    * Install the 4.2.3 Kernel or move to step 2 if already installed. 
      * [Tutorial](http://www.yourownlinux.com/2015/10/how-to-install-linux-kernel-4-2-3-in-linux.html)
    * Run the following script to patch uvcvideo.ko
      * `scripts/patch-uvcvideo-ko-4.2.3.sh`
      * This script involves cloning the Linux source repository (about 1GB), and may take a while
4. Reload the uvcvideo driver
  * `sudo modprobe uvcvideo`

### LibUVC backend

**Note:** This backend has been deprecated on Linux.

The libuvc backend requires that the default linux uvcvideo.ko driver be unloaded before libusb can touch the device. This is because uvcvideo will own a UVC device the moment is is plugged in; user-space applications do not have permission to access the devie handle. See below regarding the udev rule workaround: 

libuvc is known to have issues with some versions of SR300 and R200 firmware (1.0.71.xx series of firmwares are problematic). 

1. Grant appropriate permissions to detach the kernel UVC driver when a device is plugged in:
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
  * `sudo cp config/uvc.conf /etc/modprobe.d/`
  * Either reboot or run `sudo udevadm control --reload-rules && udevadm trigger` to enforce the new udev rules
2. Use the makefile to build the LibUVC backend
  * `make BACKEND=LIBUVC`
  * `sudo make install`

---

## Apple OSX 

1. Install XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install libusb via brew:
  * `brew install libusb`
4. Install glfw3 via brew:
  * `brew tap homebrew/versions`
  * `brew install versions/glfw3`

---

## Windows 8.1

1. librealsense should compile out of the box with Visual Studio 2013 Release 5. Particular C++11 features are known to be incompatible with earlier VS2013 releases due to internal compiler errors. 

## FAQ

*Q:* How is this implemented?

*A:* The library communicates with RealSense devices directly via the UVC and USB protocols. It does not link against the RealSense SDK. Most of the library source code is platform agnostic, but there is a small UVC abstraction layer with platform-specific backends, including:
  * A `libuvc` backend which provides user-space access to UVC devices on Linux and Mac OS X (built with libusb)
  * A `video4linux2` backend which provides kernel-space access to UVC devices on Linux
  * A `Windows Media Foundation` backend which provides kernel-space access to UVC devices on Windows

## Example Applications

**Note:** Many example applications have various runtime features that are exposed through different mouse click and key events.

Example | Platform | Description
------------- | ------------- | -------------
c-capture | All | Basic example using the C API to pull depth, color, and IR
c-enumerate | All | A commandline application (no window) to describe properties and resolutions of all connected cameras
c-pointcloud | All | Demonstration of pointcloud projection using the C API
cpp-capture | All |  Basic example using the C++ API to pull depth, color, and IR
cpp-multicam | All | For all connected cameras, draw depth and color streams (limited by window size)
cpp-pointcloud | All | Demonstration of pointcloud projection using the C++ API (click to change the stream source: color, depth, or IR)
cpp-alignimages | All | Shows the use of synthetic streams (color aligned to depth, depth aligned to color)
cpp-restart | Windows | Runtime re-configuration of streaming modes (start, stop, reconfigure, start, etc)
cs-capture | Windows | Basic example to pull depth, color, and IR images in C#. 
cs-config | Windows | Allow user to configure which streams they want to view
py-enumerate | All | A commandline application to describe properties and resolutions of all connected cameras
java-enumerate | All | A commandline application to describe properties and resolutions of all connected cameras

## Dev Notes 

October 20th, 2015

1. (R200) ALL of 640x480, 628x468, 492x372, 480x360, 332x252, 320x240 are now available for DEPTH and INFRARED streams.
2. (All) It is safe to hardcode 640x480 or 320x240 as DEPTH or INFRARED resolutions.
3. (R200) For the resolutions which are 12 pixels larger than the native depth resolution, the depth image is centered and padded.
4. (R200) For the resolutions which are 12 pixels smaller than the native infrared resolution, the IR image is cropped and centered.
5. (All) If the same resolution is requested for DEPTH and INFRARED streams, they will be pixel-for-pixel aligned and have the same intrinsics.
6. New stream mode: RECTIFIED_COLOR – Equivalent to DSAPI’s rectified third modes, removes COLOR image distortion and cancels out rotation relative to DEPTH stream.
  * Suitable for use as a background for augmented reality rendering, images produced via rectilinear perspective projection will overlay RECTIFIED_COLOR images correctly.
7. New stream mode: DEPTH_ALIGNED_TO_RECTIFIED_COLOR – Maps data coming from the DEPTH stream to match the intrinsic and extrinsic camera properties of the DEPTH stream.
  * Suitable for pre-loading the depth map for augmented reality rendering, or performing tracking that needs to be pixel accurate with the RECTIFIED_COLOR image.

## License

Please refer to the LICENSE file for details regarding terms of the pre-release evaulation license.

Copyright © 2015 Intel Corporation. All rights reserved.
