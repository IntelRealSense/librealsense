#  Native RealSense Application for Android 
This sample demonstrates how to configure an Android application to add a RealSense dependency.  
The sample also demonstrates basic streaming using the Pipeline class and how to respond to connection events of a RealSense device.

> Read about Android support [here](../../readme.md).

## Project Configuration
Before jumping to the code, let's review the required modifications both for the module [build.gradle file](app/build.gradle) and to the app's manifest.
>
### Module Build Gradle
The app should add RealSense dependency in the module build.gradle which is currently available in a private repo.
>```java
>repositories {
>    maven{
>        url "https://dl.bintray.com/intel-realsense/librealsense"
>    }
>}
>
>configurations {
>    downloadHeader
>    downloadSo
>}
>
>dependencies {
>    def version = '2.+'
>    downloadSo 'com.intel.realsense:librealsense:' + version + '@aar'
>    implementation 'com.intel.realsense:librealsense:' + version + '@aar'
>    downloadHeader 'com.intel.realsense:librealsense:' + version + '@zip'
>}
>
>task extractHeaders(type: Sync) {
>    dependsOn configurations.downloadHeader
>    from { configurations.downloadHeader.collect { zipTree(it) } }
>    into "$projectDir/src/main/cpp/include"
>}

>task extractSo(type: Sync) {
>    dependsOn configurations.downloadSo
>    from { configurations.downloadSo.collect { zipTree(it) } }
>    include("jni/**")
>    into "$buildDir/"
>}
>
>preBuild.dependsOn(extractHeaders)
>preBuild.dependsOn(extractSo)
>```

The example shows how to use any librealsense version which is greater than 2.0.0, it is recommended to specify a specific version for example:
>```java
>    def version = '2.18.0'
>    downloadSo 'com.intel.realsense:librealsense:' + version + '@aar'
>    implementation 'com.intel.realsense:librealsense:' + version + '@aar'
>    downloadHeader 'com.intel.realsense:librealsense:' + version + '@zip'
>```

Once the gradle changes are done and the gradle sync was executed, you should be able to import librealsense classes.
It might be required to invalidate caches in case your app is failing to import librealsense (File->Invalidate Caches->Invalidate and restart)

### CMakeLists
Your native app need to link with 'librealsense.so' in order to do that you will need to edit the app's [CMakeLists.txt](app/CMakeLists.txt)

#### Add RealSense Library
>```java
>    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/include")
>    add_library(realsense2 SHARED IMPORTED)
>    set_target_properties(
>        realsense2
>        PROPERTIES
>        IMPORTED_LOCATION
>        ${CMAKE_CURRENT_SOURCE_DIR}/build/jni/${ANDROID_ABI}/librealsense2.so
>    )
>```

#### Link RealSense Library
>```java
>    target_link_libraries(
>        native-lib
>        ${log-lib}
>        realsense2
>    )
>```

### Application Manifest
A modifications is required in the [application manifest file](app/src/main/AndroidManifest.xml).

#### Camera Permissions
This permission is required by Androis >= 9, for apps that target lower Android version this is not required.
>```xml
>    <uses-permission android:name="android.permission.CAMERA"/>
>```

## Example Code
### Java Code
Let's look at the only Java source code file in this example, the [MainActivity](app/src/main/java/com/example/realsense_native_example/MainActivity.java).

#### On Create
RsContext.init must be called once in the application's lifetime before any interaction with physical RealSense devices.
For multi activities applications use the application context instead of the activity context.
>```java
>    RsContext.init(getApplicationContext());
>```

Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
In this example we read RealSense version through the native API and print it to the screen once a device is detected.
>```java
>    mRsContext = new RsContext();
>    mRsContext.setDevicesChangedCallback(new DeviceListener() {
>        @Override
>        public void onDeviceAttach() {
>            printMessage();
>        }
>
>       @Override
>       public void onDeviceDetach() {
>           printMessage();
>       }
>    });
>```

#### Accessing The Native API
We read RealSense version and query the number of connected cameras using the native (C++) API via JNI:
>```java
>    int cameraCount = nGetCamerasCountFromJNI();
>    String version = nGetLibrealsenseVersionFromJNI();
>```

### Native (C++) Code
The native code of our example is located in [native-lib.cpp](app/src/main/cpp/native-lib.cpp).

#### Reading RealSense Version
>```cpp
>extern "C" JNIEXPORT jstring JNICALL
>Java_com_example_realsense_1native_1example_MainActivity_nGetLibrealsenseVersionFromJNI(JNIEnv *env, jclass type) {
>    return (*env).NewStringUTF(RS2_API_VERSION_STR);
>}
>```

#### Query The Number Of Connected Devices
>```cpp
>extern "C" JNIEXPORT jint JNICALL
>Java_com_example_realsense_1native_1example_MainActivity_nGetCamerasCountFromJNI(JNIEnv *env, jclass type) {
>    rs2::context ctx;
>    return ctx.query_devices().size();;
>}
>```