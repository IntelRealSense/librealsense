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
  * `sudo apt-get install libusb-dev`
4. We are using QtCreator as an IDE for Linux development on Ubuntu. While makefiles will eventually be supported, you will need to download and install the IDE to compile the library
  * `sudo apt-get install qtcreator`

## Releases

Binary releases for OSX and Linux can be found on the Github repository page under the Releases tab. 

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
