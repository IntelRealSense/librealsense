>:pushpin: The SDK 2.0 delivers cross-platform open source libraries & tools that allow users to develop on multiple Operating Systems & development environments.  Intel has validated SDK2.0 on Windows and Linux platforms. Please check [latest Release](https://github.com/IntelRealSense/librealsense/releases) for the build versions.  While Intel has not explicitly validated SDK2.0 on Android platforms, it is expected to work on Android as well. Please refer to the build instructions in the section below. Calibration and firmware update tools that would be used in production and manufacturing processes are not available on Android at this time.  Please contact your Intel representative for additional information.


#  Build Intel&reg; RealSense&trade; SDK 2.0 for Android OS
This document describes how to build the [Intel&reg; RealSense&trade; SDK 2.0](https://github.com/IntelRealSense/librealsense) including headless tools and examples for Android devices.

> **Disclaimer:** There is a limitation in access to the camera from a native code due to Android USB permissions policy. Therefore, building Intel&reg; RealSense&trade; SDK 2.0 for Android devices will run only on [rooted](https://en.wikipedia.org/wiki/Rooting_(Android)) devices.

## Ingredients
Before jumping to the instructions section please ensure you have all the required accessories. 
1. Linux host machine with [Ubuntu 16.04](https://www.ubuntu.com/download/desktop).
2. [Rooted Android target device](https://en.wikipedia.org/wiki/Rooting_(Android)) that supports OTG feature.
3. [USB3 OTG](https://en.wikipedia.org/wiki/USB_On-The-Go) cable.

## Instructions of how to build SDK Headless Samples & Android Application
* [Build RealSense SDK headless tools and examples](./AndroidNativeSamples.md)
* [Build an Android application for RealSense SDK](./AndroidJavaApp.md)
