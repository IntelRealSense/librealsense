#  Build an Android Application for RealSense SDK with patched kernel
This document describes how to build an Android application to stream Depth data with RealSense cameras.

> Read about Android support [here](./Android.md).

## Instructions
1. Patch your Android device. Take [Pixel smartphone](https://en.wikipedia.org/wiki/Pixel_(smartphone)) as example.

    1. Download AOSP source code for Pixel phone.
        * `https://source.android.com/setup/build/downloading`

    1. Download AOSP Kernel source for Pixel phone.
        * `https://source.android.com/setup/build/building-kernels`

    1. Apply the patches to AOSP and kernel.
        * [pixel-aosp.patch](../../examples/android/pixel-aosp.patch)
        * [pixel-kernel.patch](../../examples/android/pixel-kernel.patch)

    1. Build Kernel and Android OS
        * `https://source.android.com/setup/build/building-kernels`
        * `https://source.android.com/setup/build/building`

1. Flash the ROM into Pixel phone.

1. Install [Android Studio](https://developer.android.com/studio/install.html) IDE for Linux.

1. Open Android Studio.
```shell
<path-to-android-studio>/bin/studio.sh
```
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio1.png" /></p>

Click on `Configure` and choose `SDK Manager`.
[Install Android SDK Platform, Build-Tools, LLDB, CMake and NDK](https://developer.android.com/ndk/guides/index.html#download-ndk) for Android Studio. Go to section "Download the NDK and Tools" and start with the second step (2. Click the SDK Tools tab).

1. Click on `Import project (Gradle, Eclipse ADT, etc.)`. Import the example project from `<path-to-librealsense>/examples/android`


1. Connect the Android device to the host machine.
Click on `Run` and choose `Run 'app'`. Choose your Android device and click on the `OK` button. At the end of this process a new application supposed to appear at the Android device.

1. Open the `realsense_app` application.

## Usage

Make sure the RealSense camera has connected to the hacked Pixel phone.

Launch the example app.

In the UI, press "ON/OFF" button to turn on/off camera,
and then press "Play/Stop" button to start/stop streaming.
