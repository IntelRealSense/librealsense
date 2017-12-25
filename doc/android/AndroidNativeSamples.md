#  Build RealSense SDK Samples for Android OS
This document describes how to build the Intel® RealSense™ SDK 2.0 including headless tools and examples for Android devices.

> Read about Android support [here](./Android.md).

## Instructions
1. [Root](https://www.wikihow.tech/Root-Android-Phones) your Android device.
2. Download the [Native Development Kit (NDK)](https://developer.android.com/ndk/downloads/index.html) for Linux to your host machine.
3. Install [CMake](https://cmake.org/) 3.6.1 or newer.
4. Download [ADB](https://developer.android.com/studio/command-line/adb.html) to the host machine by typing `sudo apt-get install adb`.
5. Clone the latest [RealSense&trade; SDK 2.0](https://github.com/IntelRealSense/librealsense/releases) to your host machine.
6. Change the streaming width and height to 480 and 270 respectively in [rs-depth](https://github.com/IntelRealSense/librealsense/blob/7724c1d8717f4e6b4486ae7b106559a889de4a1c/examples/C/depth/rs-depth.c#L21) and [rs-distance](https://github.com/IntelRealSense/librealsense/blob/7724c1d8717f4e6b4486ae7b106559a889de4a1c/examples/C/distance/rs-distance.c#L21) examples using Linux text editor.

7. Open Terminal on the host machine, navigate to *librealsense* root directory and type the following lines:
```shell
mkdir build && cd build
cmake .. -DANDROID_ABI=<Application Binary Interface> -DCMAKE_TOOLCHAIN_FILE=<Path to NDK folder>/build/cmake/android.toolchain.cmake
make
```

> Initialize ANDROID_ABI with one of the [supported ABIs](https://developer.android.com/ndk/guides/abis.html#sa) (`armeabi-v7a` for example).

8. When compilation done type the following lines to store the binaries at the same location to easily copy them to your Android device.
```shell
mkdir lrs_binaries && cd lrs_binaries
cp ../librealsense2.so ./
cp ../examples/C/color/rs-color ./
cp ../examples/C/depth/rs-depth ./
cp ../examples/C/distance/rs-distance ./
cp ../examples/save-to-disk/rs-save-to-disk ./
cp ../tools/data-collect/rs-data-collect ./
cp ../tools/enumerate-devices/rs-enumerate-devices ./
cp ../tools/fw-logger/rs-fw-logger ./
cp ../tools/terminal/rs-terminal ./
```
9. Connect your Android device to the host machine using USB OTG cable.
10. Create new folder and copy the binaries to your Android device using ADB by the following lines:
```shell
adb shell mkdir /storage/emulated/legacy/lrs_binaries
adb push . /storage/emulated/legacy/lrs_binaries/
```

11. Open ADB Shell and move to Super User mode by the following line:
```shell
adb shell su
```

12. Copy the binaries to the internal storage and change their permission to be executables by the following lines:
```shell
cp -R /storage/emulated/legacy/lrs_binaries /data/
cd /data/lrs_binaries
chown root:root *
chmod +x *
```

13. Use the USB OTG cable to connect the RealSense camera to your Android device.
14. Install [Terminal Emulator](https://en.wikipedia.org/wiki/Terminal_emulator) on your Android device from Google Play Store.
15. Open the Terminal Emulator application and type below lines in order to move to Super User mode and run one of the RealSense examples/tools.
```shell
su
cd /data/lrs_binaries
./rs-depth
```

## Expected Output
* Streaming Depth data using [rs-depth sample](https://github.com/IntelRealSense/librealsense/tree/master/examples/C/depth).
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/rs-depth_android.png" /></p>
