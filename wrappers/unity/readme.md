# Unity Wrapper for RealSense SDK 2.0

<p align="center"><img src="https://user-images.githubusercontent.com/22654243/35569320-93610f10-05d4-11e8-9237-23432532ad87.png" height="400" /></p>

> Screen capture of the `RealSenseTextures` scene

## Overview

The Unity wrapper for RealSense SDK 2.0 allows Unity developers to add streams from RealSense devices to their scenes using provided textures.
We aim to provide an easy to use prefab which allows device configuration, and texture binding using the Unity Inspector, without having to code a single line of code.
Using this wrapper, Unity developers can get a live stream of Color, Depth and Infrared textures. In addition we provide a simple way to align textures to one another (using Depth), and an example of background segmentation.


### Table of content

* [Getting Started](#getting-started)
* [Prefabs](#prefabs)
    * [RealSenseDevice](#realSenseDevice)
    * [Images](#images)

## Getting Started

### Step 1 - Import Plugins

The Unity wrapper depends on the C# wrapper provided by the RealSense SDK 2.0.
At the end of this step, the following files are expected to be available under their respective folders:

* `unity/Assets/Plugin.Managed/Intel.RealSense.dll` - The .NET wrapper assembly (for .NET 3.5)
* `unity/Assets/RealSenseSDK2.0/Plugins/realsense2.dll` - The SDK's library (that was used to build the .NET wrapper)

In order to get these files, either:
1. download and install [RealSense SDK 2.0](https://github.com/IntelRealSense/librealsense/releases), then copy the files from the installation location to their respective folders in the project. Or,
2. Follow [.NET wrapper instructions for building the wrapper](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/csharp#getting-started) and copy the file to their respective folders in the project.

  > NOTE: Unity requires that manage assemblies (such as the C# wrapper) are targeted to .NET Framework 3.5 and lower. The .NET wrapper provides assemblies for multiple targets, one of which is .NET 3.5 (net35).

### Step 2 - Open a Unity Scene

The Unity wrapper provides several example scenes to help you get started with RealSense in Unity. Open one of the following scenes under `unity/Assets/RealSenseSDK2.0/Scenes` (Sorted from basic to advanced):

1. RealSense Textures - Basic 2D scene demonstrating how to bind textures to a RealSense device stream. The Scene provides 3 live streams and 5 different textures: Depth, Infrared, Color, Colorized Depth and Color with background segmentation.


## Prefabs

### RealSenseDevice

The RealSenseDevice provides an encapsulation of a single device connected to the system. The following image displays the RealSenseDevice script:


![image](https://user-images.githubusercontent.com/22654243/36370418-77bf062a-1567-11e8-98ee-c2489aeb1208.png)

##### Process Mode
This option indicates which threading model the user expects to use.

* Multithread - A separate thread will be used to raise device frames.
* UnityThread - Unity thread will be used to raise new frames from the device.

Note that this option affects all other scripts that do any sort of frame processing.

##### Configuration
The device is configured the same way that a `Pipeline` is, i.e. with a `Config`. In order to make it available via Unity Inspector, a `Configuration` object is exposed for each RealSenseDevice. When Starting the scene, the device will try to start streaming the requested configuration (`Pipeline.Start(config)` is called).
Upon starting the device, the device will begin raising frames via its `OnNewSample` and `OnNewSampleSet` public events. These frames are raised either from a separate thread or from the Unity thread, depending on the user's choice of Process Mode.

In addition to stream configuration, the panel also allows users to select a specific device by providing its serial number.

![image](https://user-images.githubusercontent.com/22654243/36370385-5dbddb2a-1567-11e8-9c52-aa7ee988f19f.png)


##### RealSense Device Inspector

Once the device is streaming, Unity Inspector will show the device's sensors and allow controlling their options via the GUI under the RealSenseDeviceInspector script. Changing these options affect the sensor directly on runtime:

![image](https://user-images.githubusercontent.com/22654243/36370003-007a7bc2-1566-11e8-979f-e31617643e7a.png)


##### Texture Streams

Under the `RealSenseDeivce` object in Unity's Hierarchy view, you can find a number of textures that bind to the device's frame callback and allow user to bind a texture to be updated upon frame arrival.


![image](https://user-images.githubusercontent.com/22654243/36472152-90796e10-16f9-11e8-9b24-5d0abe159fbe.png)


Each texture stream is associated with the `RealSenseStreamTexture` script which allows user to bind their textures so that they will be updated with each new frame. The following screenshot displays the configurations available for users when using a texture stream:

![image](https://user-images.githubusercontent.com/22654243/35589395-5d7dd314-060c-11e8-8909-073b662df0c0.png)

* Source Stream Type - Indicates to the script from which stream of the device it should take frames.
* Texture Format - Indicates which texture should be created from the input frame.
* Texture Binding - Allows the user to bind textures to the script. Multiple textures can be bound to a single script.
* Fetch Frames From Device - Toggle whether the script should fetch the frames from the device, or should wait for the user to pass frame to is using its `OnFrame` method.

The `Alignment` object is a special case of texture stream which uses the `AlignImages` script. This script takes `FrameSet`s from the device, performs image alignment between the frames of the frame set, and passes them to each of the scripts stream texture: `from` and `to` which will provide textures that are aligned to one another.

![image](https://user-images.githubusercontent.com/22654243/35590061-5f9a9108-060e-11e8-9018-8c9f1db991ad.png)

An example usage of this script is to perform background segmentation between depth and color images by turning each colored pixel that is not within the given range into a grayscale pixel. The scene presents this example in the bottom right image labeled "Background Segmentation".
Segmentation is performed using the `BGSeg` shader.

##### PointCloud

Also under the `RealSenseDeivce` object in Unity's Hierarchy view, you can find PointCloud object that provides a 3D point cloud of the depth data in the form of Unity Particles (Using the Particle System).

The PointCloud object uses the `PointCloudGenerator.cs` script which allows some user control over the output:

![image](https://user-images.githubusercontent.com/22654243/36472070-43c9e13a-16f9-11e8-9607-3aadf97c7bb4.png)


* Gradient - is a color gradient used to color the particles
* Points Size - Controls the size of the Particles
* Skip Particles - A factor >= 1 meaning how many points to skip when creating the particles.

### Images

The Unity wrapper comes with a set of Unity Materials alongside matching Shaders to provide users with textures created from the device's frames.

To get a texture, we provide several Image objects under the `Images` prefab:

![image](https://user-images.githubusercontent.com/22654243/35591778-cc043fce-0613-11e8-8138-3aa440e54513.png)

> The above image shows the `ColorizedDepthImage`
