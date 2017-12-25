#  Build an Android Application for RealSense SDK
This document describes how to build an Android application to stream Depth data with RealSense cameras.

> Read about Android support [here](./Android.md).

## Instructions
1. [Root](https://www.wikihow.tech/Root-Android-Phones) your Android device.
2. Install [Android Studio](https://developer.android.com/studio/install.html) IDE for Linux.

3. Open Android Studio, click on `configure` and choose `SDK Manager`.
```shell
./android-studio/bin/studio.sh
```
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio1.png" /></p>

4. [Install NDK](https://developer.android.com/ndk/guides/index.html#download-ndk) for Android Studio. Do the section "Download the NDK and Tools" and skip to the third step (3. Check the boxes next to LLDB, CMake...).

5. Click on `Start a new Android Studio project`.

6. Change the Application Name and the Company Domain to `realsense_app` and `example.com` respectively, tick the checkbox `Include C++ support` and click on the `Next` button.
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio2.png" /></p>

7. Choose `API 19: Android 4.4 (KitKat)` in the first listbox and click on the `Next` button.
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio3.png" /></p>

8. Choose `Empty Activity` and click on the `Next` button.
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio4.png" /></p>

9. Ensure that the `Activity Name` and the `Layout Name` are containing `MainActivity` and `activity_main` respectively and click on the `Next` button.
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio5.png" /></p>

10. Choose `C++11` at the `C++ Standard` list box, tick the checkbox `Exception Support` and click on the `Next` button.
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio6.png" /></p>

11. Clone the latest [RealSense&trade; SDK 2.0](https://github.com/IntelRealSense/librealsense/releases) to your host machine and place the librealsense folder in the Android application folder under `./realsense_app/app/src/main/cpp/`.

12. Replace the content of `MainActivity`, `native-lib.cpp`, `activity_main.xml` and `CMakeLists.txt` with [MainActivity](MainActivity.java_), [native-lib.cpp](./native-lib.cpp_), [activity_main.xml](./activity_main.xml_) and [CMakeLists.txt](./CMakeLists.txt_) respectively.
<p align="center"><img width=90% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/AndroidStudio7.png" /></p>

13. Connect the Android device to the host machine.
Click on `Run` and choose `Run 'app'`. Choose your Android device and click on the `OK` button. At the end of this process a new application supposed to appear at the Android device.
<p align="center"><img width=50% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/cellphoneScreen.png" /></p>

14. Install [Terminal Emulator](https://en.wikipedia.org/wiki/Terminal_emulator) on your Android device from Google Play Store.
15. Use the USB OTG cable to connect the RealSense camera to your Android device.
16. Open the Terminal Emulator application and type below lines in order to move to Super User mode and change the USB permissions.
```shell
su
lsusb
chmod 0777 /dev/bus/usb/<Bus number>/<Dev Number>
```
<p align="center"><img width=50% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/TerminalEmulator.png" /></p>

17. Open the `realsense_app` application.

## Expected Output
* Streaming Depth data using rooted Android S4 device.
<p align="center"><img width=50% src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/android/DepthApp.png" /></p>
