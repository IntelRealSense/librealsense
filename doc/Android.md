#  Build Intel&reg; RealSense&trade; SDK 2.0 for Android OS
This document describes how to build the [Intel&reg; RealSense&trade; SDK 2.0](https://github.com/IntelRealSense/librealsense) including headless tools and examples for Android devices.
> **Disclaimer:** Intel&reg; RealSense&trade; SDK 2.0 for Android devices is an experimental capability and is **not** officially supported by Intel RealSense group at this point. Pay attention that your target Android device should be [rooted](https://en.wikipedia.org/wiki/Rooting_(Android)) in order to run RealSense SDK tools/examples and to access the RealSense cameras. Hence **we do not take any responsibility and we are not liable for any damage caused by this installation instructions.**

> **Note:** In order to build the RealSesne SDK and run the headless tools/examples you should prepare a Linux host machine (Ubuntu 16.04 OS for example) and a target Android device.

> Note that if your Android device supports only USB2 interface you will have to change the streaming resolution before doing step 6. In [rs-depth](https://github.com/IntelRealSense/librealsense/blob/7724c1d8717f4e6b4486ae7b106559a889de4a1c/examples/C/depth/rs-depth.c#L21) and [rs-distance](https://github.com/IntelRealSense/librealsense/blob/7724c1d8717f4e6b4486ae7b106559a889de4a1c/examples/C/distance/rs-distance.c#L21) examples you should change width and height to 480 and 270 respectively.

## Instructions
1. [Root](https://www.wikihow.tech/Root-Android-Phones) your Android device.
2. Download the [Native Development Kit (NDK)](https://developer.android.com/ndk/downloads/index.html) for Linux to your host machine.
3. Install [CMake](https://cmake.org/) 3.6.1 or newer.
4. Download [ADB](https://developer.android.com/studio/command-line/adb.html) to the host machine by typing `sudo apt-get install adb`.
5. Clone the latest [RealSense&trade; SDK 2.0](https://github.com/IntelRealSense/librealsense/releases) to your host machine.
6. Open Terminal on the host machine, navigate to *librealsense* root directory and type the following lines:
* `mkdir build && cd build`
* `cmake .. -DANDROID_ABI=<Application Binary Interface> -DCMAKE_TOOLCHAIN_FILE=<Path to NDK folder>/build/cmake/android.toolchain.cmake`
* `make`
> Initialize ANDROID_ABI with one of the [supported ABIs](https://developer.android.com/ndk/guides/abis.html#sa) (`armeabi-v7a` for example).

7. When compilation done type the following lines to store the binaries at the same location to easily copy them to your Android device.
* `mkdir lrs_binaries && cd lrs_binaries`
* `cp ../librealsense2.so ./`
* `cp ../examples/C/color/rs-color ./`
* `cp ../examples/C/depth/rs-depth ./`
* `cp ../examples/C/distance/rs-distance ./`
* `cp ../examples/save-to-disk/rs-save-to-disk ./`
* `cp ../tools/data-collect/rs-data-collect ./`
* `cp ../tools/enumerate-devices/rs-enumerate-devices ./`
* `cp ../tools/fw-logger/rs-fw-logger ./`
* `cp ../tools/terminal/rs-terminal ./`

8. Connect your Android device to the host machine using [USB OTG](https://en.wikipedia.org/wiki/USB_On-The-Go) cable.
9. Create new folder and copy the binaries to your Android device using ADB by the following lines:
* `adb shell mkdir /storage/emulated/legacy/lrs_binaries`
* `adb push . /storage/emulated/legacy/lrs_binaries/`

10. Open ADB Shell and move to Super User mode by the following line:
* `adb shell su`

11. Copy the binaries to the internal storage and change their permission to be executables by the following lines:
* `cp -R /storage/emulated/legacy/lrs_binaries /data/`
* `cd /data/lrs_binaries`
* `chown root:root *`
* `chmod +x *`

12. Use the USB OTG cable to connect the RealSense camera to your Android device.
13. Install [Terminal Emulator](https://en.wikipedia.org/wiki/Terminal_emulator) on your Android device from Google Play Store.
14. Open the Terminal Emulator application and type below lines in order to move to Super User mode and run one of the RealSense examples/tools.
* `su`
* `cd /data/lrs_binaries`
* `./rs-depth`

## Expected Output
1. Streaming Depth data using [rs-depth sample](https://github.com/IntelRealSense/librealsense/tree/master/examples/C/depth).
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/rs-depth_android.png" /></p>

2. Printing connected device information using [rs-enumerate-devices tool](https://github.com/IntelRealSense/librealsense/tree/master/tools/enumerate-devices).
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/enumerate-devices_android.png" /></p>
