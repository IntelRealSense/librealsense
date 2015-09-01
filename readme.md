# librealsense

A library for capturing data with the RealSense F200 (IVCAM 1.0, 1.5) and RealSense R200 (DS4). It targets Mac OSX 10.7+ and "recent" versions of Linux (tested & developed on Ubuntu 14.04 LTS). This effort is aimed at supporting prototyping efforts on new platforms and form-factors (robots, drones, VR, etc). This repository hosts both the source-code and binary releases.

Dependency management for GLFW3 and libusb-1.0 is done manually at the moment (see the corresponding sections below), pending the creation of installer scripts to automate the process. 

## Cameras

1.	DS4 / R200 (v0.2 release will support DS5; issue that it shares the same USB PID as DS4 right now)
2.	IVCAM 1.0 / F200 (v0.2 release will support the newer calibration model of IVCAM 1.5)

## Functionality

The goal of librealsense is to provide a reasonable hardware abstraction with minimal dependencies. It is not a computer vision SDK.

1.	Streams: depth, color, infrared
  *	Of the different streaming modes + fps options available, only fairly common ones have been tested. 
2.	Intrinsic/extrinsic calibration information (inc. uv map, etc)
3.	Majority of XU-exposed functionality for each camera

## API

1.	Unified API combines F200, R200, and future hardware iterations. 
2.	Example apps will work with both F200 and R200 at runtime, no code modifications necessary unless camera-specific features are used
3.	API is expressed as a simple C interface for cross-language compatibility. A C++ API is also exposed for convenience.


## Apple OSX Installation

1. Install XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install libusb via brew:
  * `brew install libusb`
4. Install glfw3 via brew:
  * `brew tap homebrew/versions`
  * `brew install versions/glfw3`

## Ubuntu 14.04+ Installation

1. Grant appropriate permissions to detach the kernel UVC driver when a device is plugged in:
  * `sudo cp 99-uvc.rules /etc/udev/rules.d/`
  * `sudo cp uvc.conf /etc/modprobe.d/uvc.conf/`
  * Reboot to enforce the new udev rules
2. Ensure apt-get is up to date
  * `sudo apt-get update && apt-get upgrade`
3. Install libusb-1.0 via apt-get
  * `sudo apt-get install libusb-dev`
4. glfw3 is not available in apt-get on Ubuntu 14.04. Use included installer script:
  * `sudo chmod a+x install_glfw3.sh`
  * `scripts/install_glfw3.sh`
5. We use QtCreator as an IDE for Linux development on Ubuntu: 
  * `sudo apt-get install qtcreator`
6. Don't want to use QtCreator? We have a makefile!
  * `make && sudo make install`
  * The example executables will build into `./bin`

## FAQ

*Q:* How is this implemented?

*A:* It uses a libusb implementation of UVC (libuvc) and thusly does not require platform-specific camera drivers. The same backend can be shared across platforms. It does not link against any existing DSAPI or IVCAM-DLL binaries. 

*Q:* Is it maintained or supported?

*A:* It is supported in the sense that bugs will be fixed if they are found and new features will be periodically added. It is not intended to replace functionality developed by other teams to support HVM (firmware updates, etc), nor materially impact SSG’s roadmap for the SDK/DCM – it is independent & outside of these major efforts. 

## License

For internal distribution only. 

("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
Material remains with Intel Corporation or its suppliers and licensors. The Material may
contain trade secrets and proprietary and confidential information of Intel Corporation
and its suppliers and licensors, and is protected by worldwide copyright and trade secret
laws and treaty provisions. No part of the Material may be used, copied, reproduced,
modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right
is granted to or conferred upon you by disclosure or delivery of the Materials, either
expressly, by implication, inducement, estoppel or otherwise. Any license under such
intellectual property rights must be express and approved by Intel in writing.

Copyright © 2015 Intel Corporation. All rights reserved.
