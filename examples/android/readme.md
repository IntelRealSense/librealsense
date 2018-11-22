## Overview

In this example, we build librealsense for Android.
Use Android as a special Linux distribution.

## Build

1. Install Android Studio 3.2.0

1. Open Android SDK Manager in Android Studio, Install follow components:

	* Android SDK Platform 28 (Android 9.0)
	* Android SDK Build-Tools 28.0.3
	* CMake 3.6.4111459
	* Android SDK Platform-Tools 28.0.1
	* Android SDK Tools 26.1.1
	* NDK 18.0

1. Launch Android Studio, Import this project

1. Build

	1. Build librealsense for Android

		* Please make sure the paths in the script `build-librealsense-for-android.bat` for Windows or `build-librealsense-for-android.sh` for Linux are correct.
		* Run this script, and wait.
		* The libraies will be copied to folder `libs`

	1. Build the app in Android Studio

## Hack Google Pixel

1. Download AOSP source code for Pixel phone.
	* `https://source.android.com/setup/build/downloading`

1. Download AOSP Kernel source for Pixel phone.
	* `https://source.android.com/setup/build/building-kernels`

1. Apply the patches into Android OS.
	* [pixel-aosp.patch](pixel-aosp.patch)
	* [pixel-kernel.patch](pixel-kernel.patch)

1. Build Kernel and Android OS
	* `https://source.android.com/setup/build/building-kernels`
	* `https://source.android.com/setup/build/building`

1. Flash the ROM into Pixel phone.

## Todo

+ Missing error handle.
+ Missing camera configure UI.

## Usage

Make sure the RealSense camera has connected to the hacked Pixel phone.

Launch the example app.

In the UI, press "ON/OFF" button to turn on/off camera,
and then press "Play/Stop" button to start/stop streaming.
