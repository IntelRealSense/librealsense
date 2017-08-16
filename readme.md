<p align="center"><img src="doc/img/realsense.png" width="80%" /><br><br></p>

-----------------

Linux | Windows |
-------- | ------------ |
[![Build Status](https://travis-ci.org/IntelRealSense/librealsense.svg?branch=master)](https://travis-ci.org/IntelRealSense/librealsense) | [![Build status](https://ci.appveyor.com/api/projects/status/y9f8qcebnb9v41y4?svg=true)](https://ci.appveyor.com/project/ddiakopoulos/librealsense) |

**Intel® RealSense™ Cross Platform SDK** is a cross-platform library (Linux, Windows, Mac) for working with Intel® RealSense™ depth cameras.
Developer kits containing the necessary hardware to use this library are available for purchase at [this link](http://click.intel.com/realsense.html). 

[Whats new?](./doc/rs400_support.md) - Summary of changes in librealsense2, including RS400 support, API changes and new functionality.

## Quick Start

After downloading and installing the  Intel® RealSense™ SDK (on [Linux]() \ [Windows]() \ [Os X]()), and connecting an [Intel® RealSense™ depth camera](), you are ready to start writing your first application!

#### First things first...
Try running the SDK's Viewer application to make sure the library is installed \ compiled correctly and that the camera is properly installed as well.

To do that, run the <img src="./tools/realsense-viewer/res/icon.ico" width="25" height="25" alt="realsense-viewer icon"> [realsense-viewer](./tools/realsense-viewer) tool (can be found under *&lt;installation path&gt;/tools/realsense-viewer.exe*).
Next, choose a stream to play, and turn it on:
<p align="center"><img src="./tools/realsense-viewer/res/realsense-viewer-backup.gif" alt="realsense-viewer gif"/></p>

> In case of any issue during initial setup, or later, please first see our [FAQ & Troubleshooting]() section. 
> If you can't find an answer there, try searching our [Closed GitHub Issues](https://github.com/IntelRealSense/librealsense/issues?utf8=%E2%9C%93&q=is%3Aclosed) page,  [Community](https://communities.intel.com/community/tech/realsense) and [Support](https://www.intel.com/content/www/us/en/support/emerging-technologies/intel-realsense-technology.html) sites.
> If non of the above helped, or you're too lazy to search :wink:, [open a new issue](https://github.com/IntelRealSense/librealsense/issues/new).

#### Ready to hack!

Every Intel® RealSense™ SDK application should start by declaring a `rs2::context` object:

```cpp
//Create a context
rs2::context ctx;
```

`rs2::context` is the highest level object in the SDK. It is the entry point for querying connected devices and loading recorded sequences. 
We create an automatic (stack allocated) variable instance of the context since it is not required to be kept "alive", and after taking whatever we need from it, we can dispose of it.
After creating the context we can create a `rs2::pipeline` object:

```cpp
//Create a pipeline
rs2::pipeline p = ctx.create_pipeline();
```

Pipeline serves as a top-level API for streaming and processing frames. It has a default configuration, optimized for the best tradeoff between stream's quality and performance, which allows you to start receiving frames without any boilerplate code: 

```cpp
// Configure and start the pipeline
p.start();
```

Next, we have our "main" loop of the application. Every iteration we wait for the pipeline to fetch a synchronized set of frames, and using the depth frame we display the distance between the camera and the object in front of the center of the lens:

```cpp
while (true)
{
    //Get a depth frame
    rs2::frameset frames = p.wait_for_frames(); //Block program until frames arrive
    rs2::depth_frame depth = frames.get_depth_frame(); //Try to get a frame with depth image
    if (!depth) continue;

    // Query distance to the target:
    rs2::point2d center_depth { depth.get_width(), depth.get_height() };
    float dist_to_center = depth.get_distance(center_depth);
    
    std::cout << "You are pointing at an object " 
		      << dist_to_center << " meters away from the camera\r";

	//Stop if object gets too far
    if (dist_to_center > 2) break;
}
```

Voilà! 
You now poses the power to measure distance of objects in an image. 
Let your imagination go wild, see some of [our examples](./examples) and read the [documentation](./doc) to learn more, or see what other people are already doing with Intel® RealSense™ technologies:

# &lt; Show off here... &gt;


## Table of Contents
* Installation Guides:
  * [Linux](./doc/installation.md)
  * [Windows](./doc/installation_windows.md)
  * [Mac OS X](./doc/installation_osx.md)
* [Useful Links](#useful-links)
* [Documentation](#documentation)
* [Functionality](#functionality)
* [Compatible Devices](#compatible-devices)
* [Compatible Platforms](#compatible-platforms)
* [Integrations](#integrations)
* [License](#license)

## Useful Links
* [Intel RealSense Community](https://communities.intel.com/community/tech/realsense) - Official support, Q&A and other useful content
* [Support Site](http://www.intel.com/content/www/us/en/support/emerging-technologies/intel-realsense-technology.html) - Contains content and web ticket capability for 1:1 interaction
* [SDK Design Guidelines](http://www.mouser.com/pdfdocs/intel-realsense-sdk-design-r200.pdf) - Guidelines and tips for designing applications using RealSense cameras
* [R200 Datasheet](http://www.mouser.com/pdfdocs/intel_realsense_camera_r200.pdf) - In-depth information about the R200 camera hardware.
* [Intel RealSense Stereoscopic Depth Cameras](https://arxiv.org/abs/1705.05548) - A technical paper describing the R200, LR200, SR300 and RS400 in detail. Includes theoretical background, performance expectations, post-processing suggestions, etc.  
## Documentation

A comprehensive suite of sample and tutorial applications are provided in the `/examples` subdirectory. For new users, it is best to review the tutorial series of apps which are designed to progressively introduce API features.

* [API Architecture](./doc/api_arch.md) - Overview of the high-level concepts
* [C API](./include/librealsense/rs.h) - With doxygen-style API comments
* To build documentation locally from sources, on Ubuntu run the following commands:
  * `sudo apt-get install doxygen`
  * `doxygen doc/doxygen/doxyfile`
* [Frame Management](./doc/frame_lifetime.md) - Frame Memory Management, Threading Model and Syncronization
* [Frame Metadata](./doc/frame_metadata.md) - support for frame-metadata attributes
* [Getting Started](./doc/stepbystep/getting_started_with_openCV.md) - Getting started with OpenCV
* [Troubleshooting](./doc/troubleshooting.md) - Useful tips when debugging issues related to the camera
* [Error Handling](./doc/error_handling.md) - Documents librealsense error handling policy
* Device specific topics:
  * [RS400 and External Devices](./doc/rs400/external_devices.md) - Notes on integrating RS400 with external devices
  * [RS400 Advanced Mode](./doc/rs400/rs400_advanced_mode.md) - Overview of the Advanced Mode APIs
* [Realsense Record and Playback](./src/media/readme.md) - SDK Record and Playback functionality using ROSbag file format
## Functionality

1. Native streams: depth, color, infrared, fisheye, motion-tracking.
2. Synthetic streams: depth aligned to color and vice versa, pointcloud.
3. Intrinsic/extrinsic calibration information.
4. Majority of hardware-specific functionality for individual camera generations (UVC XU controls).
5. Multi-camera capture across heterogeneous camera architectures

## Compatible Devices

1. RealSense SR300
2. RealSense RS400 series

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

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details