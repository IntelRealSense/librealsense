# Intel&reg; RealSense&trade; SDK 2.0 for Android OS

Using the Android wrapper allows you to build both native (C/C++) and Java applications for Android.
In order to do that a RealSense Android App need to include librealsense.aar (Android Archive) in it's dependencies.
The AAR file is responsible to acquire camera access permissions and it also provides a JNI based RealSense Java API.

## Ingredients
Before jumping to the instructions section please ensure you have all the required accessories. 
1. Android target device with Android version >= 7.0 that supports OTG feature.
2. [Android Studio IDE](https://developer.android.com/studio).
3. [USB3 OTG](https://en.wikipedia.org/wiki/USB_On-The-Go) cable.

## Build A RealSense Application
Follow [Java example](examples/java_example/readme.md) or [Native example](examples/native_example/readme.md) in order to learn how to build a RealSense application.
Those application are using librealsense from Maven ropo but you can also [build RealSense AAR from source](./build_from_source.md) and add it as a dependency as described [here](https://developer.android.com/studio/projects/android-library#AddDependency)
