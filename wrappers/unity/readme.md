# Unity Wrapper for RealSense SDK 2.0

<p align="center"><img src="https://user-images.githubusercontent.com/22654243/35569320-93610f10-05d4-11e8-9237-23432532ad87.png" height="400" /></p>

> Screen capture of the RealSenseTextures scene

## Overview

The Unity wrapper for RealSense SDK 2.0 allows Unity developers to add streams from RealSense devices to their scenes using provided textures.
We aim to provide an easy to use prefab which allows device configuration, and texture binding using the Unity Inspector, without having to code a single line of code. 
Using this wrapper, Unity developers can get a live stream of Color, Depth and Infrared textures. In addition we provide a simple way to align textures to one another (using Depth), and an example of backgroung segmentation.


### Table of content

* [Getting Started](#getting-started)
* [Prefabs](#prefabs)

## Getting Started

### Step 1 - Import Plugins

The Unity wrapper depends on the C# wrapper provided by the RealSense SDK 2.0.
At the end of this step, the following files are expected to be available under their respective folders:

* `unity/Assets/Plugin.Managed/LibrealsenseWrapper.dll` - The .NET wrapper assembly
* `unity/Assets/RealSenseSDK2.0/Plugins/realsense2.dll` - The SDK's library

In order to get these files, either:
1. download and install [RealSense SDK 2.0](https://github.com/IntelRealSense/librealsense/releases), then copy the files from the installation location to their respective folders in the project. Or,
2. Follow [.NET wrapper instructions for building the wrapper](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/csharp#getting-started) and copy the file to their respective folders in the project.

> NOTE: Unity requires that manage assemblies (such as the C# wrapper) are targeted to .NET Framework 3.5 and lower.

### Step 2 - Open a Unity Scene

The Unity wrapper provides several example scenes to help you get started with RealSense in Unity. Open one of the following scenes under `unity/Assets/RealSenseSDK2.0/Scenes` (Sorted from basic to advanced):

1. RealSense Textures - Basic 2D scene demonstrating how to bind textures to a RealSense device stream. The Scene provides 3 live streams and 5 different textures: Depth, Infrared, Color, Colorized Depth and Color with background segmentation.


## Prefabs

### RealSenseDevice

#### Device Configuration

![image](https://user-images.githubusercontent.com/22654243/35568760-b5860d4a-05d2-11e8-847a-a0a0904e2370.png)

* Process Mode
* Configuration

##### Texture Streams

* Depth / Colorized Depth / Infrared  / Color
* Alignment

### Images

* Colorized Depth / Depth / IR / Color
* BGSeg
