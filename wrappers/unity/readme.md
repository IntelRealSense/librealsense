# Unity Wrapper for RealSense SDK 2.0

This wrapper allow Unity developers to add streams from RealSense devices to their scenes.

### Table of content

* Getting Started
* Prefabs
* Scripts

### Getting Started

#### Step 1 - Import Plugins

The Unity wrapper depends on the C# wrapper provided by the RealSense SDK 2.0.
At the end of this step, the following files are expected to be available under their respective folders:

* `unity/Assets/Plugin.Managed/LibrealsenseWrapper.dll` - The .NET wrapper assembly
* `unity/Assets/RealSenseSDK2.0/Plugins/realsense2.dll` - The SDK's library

In order to get these files, either:
1. download and install [RealSense SDK 2.0](https://github.com/IntelRealSense/librealsense/releases), then copy the files from the installation location to their respective folders in the project. Or,
2. Follow [.NET wrapper instructions for building the wrapper](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/csharp#getting-started) and copy the file to their respective folders in the project.

> NOTE: Unity requires that manage assemblies (such as the C# wrapper) are targeted to .NET Framework 3.5 and lower.

#### Step 2 - Open a Unity Scene

The Unity wrapper provides several example scenes to help you get started with RealSense in Unity. Open one of the following scenes under `unity/Assets/RealSenseSDK2.0/Scenes` (Sorted from basic to advanced):

1. RealSense Textures - Basic 2D scene demonstrating how to bind textures to a RealSense device stream.
2. [TBD] Background Segmentation - Example of using Unity shaders with streams to provide a blended Color and Depth image.  


#### Texture Binding

TODO: document
