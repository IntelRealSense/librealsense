# Intel® RealSense™ Cross Platform API

[ ![License] [license-image] ] [license]

[release-image]: http://img.shields.io/badge/release-1.12.0-blue.svg?style=flat
[releases]: https://github.com/IntelRealSense/librealsense/releases

[license-image]: http://img.shields.io/badge/license-Apache--2-blue.svg?style=flat
[license]: LICENSE

Platform | Build Status |
-------- | ------------ |
Linux and OS X | [![Build Status](https://travis-ci.org/IntelRealSense/librealsense.svg?branch=master)](https://travis-ci.org/IntelRealSense/librealsense) |
Windows | [![Build status](https://ci.appveyor.com/api/projects/status/y9f8qcebnb9v41y4?svg=true)](https://ci.appveyor.com/project/ddiakopoulos/librealsense) |

This project is a cross-platform library (Linux, Windows, Mac) for capturing data from the Intel® RealSense™ F200, SR300, R200, LR200 and the ZR300 cameras. This effort was initiated to better support researchers, creative coders, and app developers in domains such as robotics, virtual reality, and the internet of things. Several often-requested features of RealSense™ devices are implemented in this project, including multi-camera capture.

Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). This project is separate from the production software stack available in the [Intel® RealSense™ SDK](https://software.intel.com/en-us/intel-realsense-sdk), namely that this library only encompasses camera capture functionality without additional computer vision algorithms.

The Intel® RealSense™ Cross Platform API is experimental and not an official Intel product. It is subject to incompatible API changes in future updates.

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

  * [C API](./include/librealsense/rs.h) - With doxygen-style API comments
  * To build documentation locally from sources, on Ubuntu run the following commands:
    * `sudo apt-get install doxygen`
	* `doxygen doc/Doxygen_API/Doxyfile`
  * [What's New?](./doc/whats_new.md)
  * [Projection APIs](./doc/projection.md) - A guide on coordinate systems, calibration information, and projection
  * [Camera Spec Sheet](./doc/camera_specs.md) - A brief overview of R200, F200 and SR300
  * [Developer Notes](./doc/dev_log.md) - Several informal notes gathered during internal releases
  * [OpenCV Tutorial](./doc/stepbystep/getting_started_with_openCV.md) - Getting started with librealsense using OpenCV
  * [Stream Formats](./doc/supported_video_formats.pdf) - A list of available stream resolutions and pixel formats provided by the supported devices.
  * [Branching and Releases](./doc/branching.md) - Overview of live branches and major releases

## Functionality

1. Native streams: depth, color, infrared and fisheye.
2. Synthetic streams: rectified images, depth aligned to color and vice versa, etc.
3. Intrinsic/extrinsic calibration information.
4. Majority of hardware-specific functionality for individual camera generations (UVC XU controls).
5. Multi-camera capture across heterogeneous camera architectures (e.g. mix R200 and F200 in same application)
6. Motion-tracking sensors acquisition (ZR300 only)

## Compatible Devices

1. RealSense R200
2. RealSense F200
3. RealSense SR300
4. RealSense LR200
5. [RealSense ZR300](https://newsroom.intel.com/chip-shots/intel-announces-tools-realsense-technology-development/)

## Compatible Platforms

The library is written in standards-conforming C++11 and relies only on the C89 ABI for its public interface. It is developed and tested on the following platforms:

1. Ubuntu 14.04 and 16.04 LTS (GCC 4.9 toolchain)
2. Windows 8.1 and Windows 10 (Visual Studio 2015 Update 2)
3. Mac OS X 10.7+ (Clang toolchain)
4. [Ostro](https://ostroproject.org/)

## Hardware Requirements
Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). In addition, several consumer tablets and laptops with integrated cameras may also function, such as the [HP Spectre x2 with R200](http://store.hp.com/us/en/ContentView?storeId=10151&langId=-1&catalogId=10051&eSpotName=new-detachable).

Developer kits **require** USB 3.0. RealSense™ cameras do not provide backwards compatibility with USB 2.0. Not all USB host chipsets are compatible with this library, although it has been validated with recent generations of the Intel Host Controller chipset. An exhaustive list of incompatible hardware is not presently provided. On x86, a Haswell or newer architecture is recommended.

For small-form factor usages, this library has been demonstrated to work on the following boards:
  * [Intel Compute Stick, BOXSTK1AW32SCR](http://www.amazon.com/Intel-Compute-BOXSTK1AW32SCR-Windows-32-bit/dp/B01ASB0DJ8)
  * [MinnowBoard Max](http://minnowboard.org)
  * [Kangaroo MD2B](http://www.amazon.com/Kangaroo-MD2B-Mobile-Desktop-Computer/dp/B017J20D8U)
  * [UP Board](http://www.up-board.org/kickstarter/up-intel-realsense-technology/)
  * [Intel Joule](https://newsroom.intel.com/chip-shots/make-amazing-things-happen-iot-entrepreneurship-intel-joule/)

## Integrations

The library has been integrated with a number of third-party components and operating systems. While most of these projects are not directly supported by the team, they are useful resources for users of this library.

  * [Robotic Operating System](https://github.com/intel-ros/realsense) (Intel Supported; R200, F200, SR300 all supported)
  * [Yocto / WindRiver Linux](https://github.com/IntelRealSense/meta-intel-librealsense)
  * [Arch Linux](https://aur.archlinux.org/packages/librealsense/)

## Community Contributions
  
Additional language bindings (experimental, community maintained):
  * [Python](https://github.com/toinsson/pyrealsense)
  * [Java (generated by JavaCPP)](https://github.com/poqudrof/javacpp-presets/tree/realsense-pull)
  
Excellent blog-series by [@teknotus](https://github.com/teknotus) covering the fundamentals of working with RealSense on Linux:
  * [Intel RealSense camera on Linux](http://solsticlipse.com/2015/01/09/intel-real-sense-camera-on-linux.html)
  * [3d Camera Controls](http://solsticlipse.com/2015/02/10/intel-real-sense-on-linux-part-2-3d-camera-controls.html)
  * [Infrared, calibration, point clouds](http://solsticlipse.com/2015/03/31/intel-real-sense-3d-on-linux-macos.html)
  * [The long road to ubiquitous 3d cameras](http://solsticlipse.com/2016/09/26/long-road-to-ubiquitous-3d-cameras.html)

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
