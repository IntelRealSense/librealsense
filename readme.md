# Intel® RealSense™ Cross Platform API

[Whats new?](./doc/rs400_support.md) - Summary of changes in librealsense2, including RS400 support, API changes and new functionality.

[ ![License] [license-image] ] [license]

[release-image]: http://img.shields.io/badge/release-1.9.7-blue.svg?style=flat
[releases]: https://github.com/IntelRealSense/librealsense/releases

[license-image]: http://img.shields.io/badge/license-Apache--2-blue.svg?style=flat
[license]: LICENSE

Platform | Build Status |
-------- | ------------ |
Linux and OS X | [![Build Status](https://travis-ci.org/IntelRealSense/librealsense.svg?branch=master)](https://travis-ci.org/IntelRealSense/librealsense) |
Windows | [![Build status](https://ci.appveyor.com/api/projects/status/y9f8qcebnb9v41y4?svg=true)](https://ci.appveyor.com/project/ddiakopoulos/librealsense) |

This project is a cross-platform library (Linux, Windows, Mac) for capturing data from the Intel® RealSense™ SR300 and RS4XX depth cameras.

Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). This project is separate from the production software stack available in the [Intel® RealSense™ SDK](https://software.intel.com/en-us/intel-realsense-sdk), namely that this library only encompasses camera capture functionality without additional computer vision algorithms.

## Table of Contents
* [Installation Guide](#installation-guide)
* [Documentation](#documentation)
* [Functionality](#functionality)
* [Compatible Devices](#compatible-devices)
* [Compatible Platforms](#compatible-platforms)
* [Integrations](#integrations)
* [License](#license)

## Installation Guide

* [Linux](./doc/installation.md)
* [Windows](./doc/installation_windows.md)
* [Mac OS X](./doc/installation_osx.md)

## Documentation

A comprehensive suite of sample and tutorial applications are provided in the `/examples` subdirectory. For new users, it is best to review the tutorial series of apps which are designed to progressively introduce API features.

* To build documentation locally from sources, on Ubuntu run the following commands:
  * `sudo apt-get install doxygen`
	* `doxygen doc/doxygen/doxyfile`
* [C API](./include/librealsense2/rs.h) - With doxygen-style API comments
* [Frame Management](./doc/frame_lifetime.md) - Frame Memory Management, Threading Model and Syncronization
* [Python Bindings](./doc/python.md) - official python bindings for librealsense
* [What's New?](./doc/rs400_support.md) - changes in librealsense2
* [Getting Started](./doc/stepbystep/getting_started_with_openCV.md) - Getting started with OpenCV
* [Troubleshooting](./doc/troubleshooting.md) - Useful tips when debugging issues related to the camera
* Device specific topics:
  * [RS400 and External Devices](./doc/rs400/external_devices.md) - Notes on integrating RS400 with external devices

## Functionality

1. Native streams: depth, color, infrared and fisheye.
2. Synthetic streams: rectified images, depth aligned to color and vice versa, etc.
3. Intrinsic/extrinsic calibration information.
4. Majority of hardware-specific functionality for individual camera generations (UVC XU controls).
5. Multi-camera capture across heterogeneous camera architectures
6. Motion-tracking sensors acquisition

## Compatible Devices

1. RealSense SR300
2. RealSense RS400/410/430/450

## Compatible Platforms

The library is written in standards-conforming C++11 and relies only on the C89 ABI for its public interface. It is developed and tested on the following platforms:

1. Ubuntu 14/16 LTS (GCC 4.9 toolchain)
2. Windows 10 (Visual Studio 2015 Update 3)
3. Mac OS X 10.7+ (Clang toolchain) **Temporarily unavailable**

## Hardware Requirements
Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). In addition, several consumer tablets and laptops with integrated cameras may also function, such as the [HP Spectre x2 with R200](http://store.hp.com/us/en/ContentView?storeId=10151&langId=-1&catalogId=10051&eSpotName=new-detachable).

Developer kits **require** USB 3.0. RealSense™ cameras do not provide backwards compatibility with USB 2.0. Not all USB host chipsets are compatible with this library, although it has been validated with recent generations of the Intel Host Controller chipset. An exhaustive list of incompatible hardware is not presently provided. On x86, a Haswell or newer architecture is recommended.

For small-form factor usages, this library has been demonstrated to work on the following boards:
  * [UP Board](http://www.up-board.org/kickstarter/up-intel-realsense-technology/)
  * [Intel Joule](https://newsroom.intel.com/chip-shots/make-amazing-things-happen-iot-entrepreneurship-intel-joule/)

## Integrations

The library has been integrated with a number of third-party components and operating systems. While most of these projects are not directly supported by the team, they are useful resources for users of this library.

  * [Robotic Operating System](https://github.com/intel-ros/realsense) (Intel Supported; R200, F200, SR300 all supported)
  * [Yocto / WindRiver Linux](https://github.com/IntelRealSense/meta-intel-librealsense)
  * [Arch Linux](https://aur.archlinux.org/packages/librealsense/)

Additional language bindings (experimental, community maintained):
  * [Java (generated by JavaCPP)](https://github.com/poqudrof/javacpp-presets/tree/realsense-pull)

## License

Copyright 2016 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this project except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
