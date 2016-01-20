# librealsense

Platform | Build Status |
-------- | ------------ |
Linux | Travis badge |
OSX | Travis badge |
Windows | Appveyor badge | 

Librealsense is a cross-platform library for capturing data from the Intel® RealSense™ F200, SR300 and R200 cameras. This effort was initiated to better support developers in domains such as robotics, virtual reality, and the internet of things. 

Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). This project is separate from the production software stack available in the the [Intel® RealSense™ SDK](https://software.intel.com/en-us/intel-realsense-sdk), namely that librealsense only encompasses camera capture functionality without additional computer vision algorithms.

Librealsense is experimental and not an official Intel product. It is subject to significant incompatible API changes in future releases. Breaking API changes are noted through release numbers with [semver](http://semver.org/).

Dependency management for GLFW3 (example apps) and libusb-1.0 is performed through manual steps that are enumerated as part of this readme file (i.e. these packages must be installed through apt-get on Linux and Homebrew on OSX).

# Table of Contents
* [Supported Devices](#supported-devices)
* [Supported Platforms](#supported-platforms)
* [Supported Languages](#supported-languages-and-frameworks)
* [Functionality](#functionality)
* [Installation Guide](#installation-guide)
  * [Linux](#ubuntu-1404-lts-installation)
  * [OSX](#apple-osx)
  * [Windows](#windows-81)
* [Hardware Requirements](#hardware-requirements)
* [FAQ](#faq)
* [Documentation](#documentation)

## Supported Devices

1. RealSense R200
2. RealSense F200
3. RealSense SR300

## Supported Platforms

librealsense is written in standards-conforming C++11 and relies only on the C89 ABI for its public interface. It was developed and tested on the following platforms:

1. Windows 8.1 (Visual C++ 2013)
2. Ubuntu 14.04 LTS (gcc toolchain)
3. Mac OS X 10.7+ (clang toolchain)

Neither the libuvc or V4L2 backend has been validated on Ubuntu 12.04 LTS or Ubuntu 15.10, and several attempts to bring cameras up on these platforms have revealed underlying OS bugs and issues. It may be possible to compile and run librealsense on other platforms. Please file an issue or submit a pull request for ....

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

**Note:** Several scripts below invoke `wget, git, add-apt-repository` which may be blocked by your IT firewall resulting in timeouts and errors. Add necessary proxy settings to config files or append scripts with appropriate switches. 

1. Ensure apt-get is up to date
  * `sudo apt-get update && sudo apt-get upgrade`
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
  * Reboot or run `sudo udevadm control --reload-rules && udevadm trigger` to enforce the new udev rules
3. Next, choose one of the following subheadings based on desired machine configuration / kernel version (and remember to complete step 4 after). **Note: ** Multi-camera support is currently NOT supported on 3.19.xx kernels. Please update to 4.4 stable. 
  * **Updated 4.4 Stable Kernel** (recommended)
    * Run the following script to install necessary dependencies (GCC 4.9 compiler and openssl) and update kernel to v4.4-wily
      * `./scripts/install_dependencies-4.4.sh`
    * Run the following script to patch uvcvideo.ko
      * `./scripts/patch-uvcvideo-4.4.sh v4.4-wily` (note the argument provided to this version of the script)
      * This script involves shallow cloning the Linux source repository (100mb), and may take a while
  * **(OR) Stock 3.19.xx Kernel in 14.04.xx** 
    * Run the following script to patch uvcvideo.ko
      * `./scripts/patch-uvcvideo-3.19.sh`
    * (R200 Only) Install connectivity workaround
      * `./scripts/install-r200-udev-fix.sh`
      * This udev fix is not necessary for kernels >= 4.2.3
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
  * `brew install homebrew/versions/glfw3`

---

## Windows 8.1

1. librealsense should compile out of the box with Visual Studio 2013 Release 5. Particular C++11 features are known to be incompatible with earlier VS2013 releases due to internal compiler errors. GLFW is provided in the solution as a NuGet package.

librealsense has not been tested with Visual Studio Community Edition.

## Hardware Requirements
Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). In addition, several consumer tablets and laptops with integrated cameras may also function, such as the [HP Spectre x2 with R200](http://store.hp.com/us/en/ContentView?storeId=10151&langId=-1&catalogId=10051&eSpotName=new-detachable).

Developer kits require USB 3.0. Not all USB host chipsets are compatible with librealsense. An exhaustive list of incompatible hardware is not presently provided.

## FAQ

*Q:* How is this implemented?

*A:* The library communicates with RealSense devices directly via the UVC and USB protocols. It does not link against the RealSense SDK runtime. Most of the library source code is platform agnostic, but there is a small UVC abstraction layer with platform-specific backends, including:
  * A `libuvc` backend which provides user-space access to UVC devices on Linux and Mac OS X (built with libusb)
  * A `video4linux2` backend which provides kernel-space access to UVC devices on Linux
  * A `Windows Media Foundation` backend which provides kernel-space access to UVC devices on Windows

## Documentation

Documentation for librealsense is still incomplete, and may contain inaccuracies. Please send us feedback about things that are unclear or which need to be improved. For now, the following documentation is available:
  * [The librealsense C API](./include/librealsense/rs.h) - Doxygen style comments are provided for all functions, data types, and most constants
  * [Projection in librealsense](./doc/projection.md) - A guide on coordinate systems, calibration information, and projection APIs.
  * [Developer Notes](./doc/dev_log.md) - Several informal notes gathered during releases.

## License

Copyright 2015 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this project except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.