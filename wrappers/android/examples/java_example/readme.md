#  Java RealSense Application for Android 
This sample demonstrates how to configure an Android application to add a RealSense dependency.  
The sample also demonstrates basic streaming using the Pipeline class and how to respond to connection events of a RealSense device.

> Read about Android support [here](../../readme.md).

## Project Configuration
Before jumping to the code, let's review the required modifications both for the module [`build.gradle file`](app/build.gradle) and to the app's manifest.
>
### Module Build Gradle
The app should add RealSense dependency in the module `build.gradle` which is currently available in a private repo.
>```java
>repositories {
>   maven{
>        url "https://dl.bintray.com/intel-realsense/librealsense"
>    }
>}
>
>dependencies {
>    implementation 'com.intel.realsense:librealsense:2.+@aar'
>}
>```
The example shows how to use any librealsense version which is greater than 2.0.0, it is recommended to specify a specific version for example:
>```java
>    implementation 'com.intel.realsense:librealsense:2.18.0@aar'
>```

Once the gradle changes are done and the gradle sync was executed, you should be able to import librealsense classes.
It might be required to invalidate caches in case your app is failing to import librealsense (File->Invalidate Caches->Invalidate and restart)

### Application Manifest
A modifications is required in the [application manifest file](app/src/main/AndroidManifest.xml).

#### Camera Permissions
This permission is required by Androis >= 9, for apps that target lower Android version this is not required.
>```xml
>    <uses-permission android:name="android.permission.CAMERA"/>
>```

## Example Code
Let's look at the only source code file in this example, the [MainActivity](app/src/main/java/com/example/realsense_java_example/MainActivity.java).

> **Note:** Since the SDK is holding-on to native hardware resources, it is critical to make sure you deterministically call `close` on objects, especially those derived from `Frame`. Without releasing resources explicitly the Garbage Collector will not keep-up with new frames being allocated. Take advantage of `try(){}` whenever you work with frames. 

### On Create
`RsContext.init` must be called once in the application's lifetime before any interaction with physical RealSense devices.
For multi activities applications use the application context instead of the activity context.
>```java
>    RsContext.init(getApplicationContext());
>```

Register to notifications regarding RealSense devices attach/detach events via the `DeviceListener`.
In this example we start the streaming thread once a device is detected.
>```java
>    mRsContext = new RsContext();
>    mRsContext.setDevicesChangedCallback(new DeviceListener() {
>        @Override
>        public void onDeviceAttach() {
>            mStreamingThread.start();
>        }
>
>       @Override
>       public void onDeviceDetach() {
>           mStreamingThread.interrupt();
>       }
>    });
>```

### Streaming
Once a device was detected we start streaming using a dedicated thread.

#### Streaming Thread
>```java
>    private Thread mStreamingThread = new Thread(new Runnable() {
>        @Override
>        public void run() {
>            try {
>                stream();
>            } catch (Exception e) {
>                e.printStackTrace();
>            }
>        }
>    });
>```

#### Streaming Function
Start streaming using a Pipeline, wait for new frames to arrive on each iteration and display the distance to pixel in the center of the depth frame

>```java
>    //Start streaming and print the distance of the center pixel in the depth frame.
>    private void stream() throws Exception {
>        Pipeline pipe = new Pipeline();
>        pipe.start();
>        final DecimalFormat df = new DecimalFormat("#.##");
>
>        while (!mStreamingThread.isInterrupted())
>        {
>            try (FrameSet frames = pipe.waitForFrames()) {
>                try (Frame f = frames.first(StreamType.DEPTH))
>                {
>                    DepthFrame depth = f.as(Extension.DEPTH_FRAME);
>                    final float deptValue = depth.getDistance(depth.getWidth()/2, depth.getHeight()/2);
>                    runOnUiThread(new Runnable() {
>                        @Override
>                        public void run() {
>                            TextView textView = findViewById(R.id.distanceTextView);
>                            textView.setText("Distance: " + df.format(deptValue));
>                        }
>                    });
>                }
>             }
>         }
>         pipe.stop();
>    }
>```
