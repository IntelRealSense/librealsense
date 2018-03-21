# .NET Wrapper for Intel RealSense SDK

This folder offers P/Invoke based bindings for most SDK APIs, together with a couple of examples. At the moment we only offer Visual Studio solution file to build on Windows, but it should be possible to use the same bindings on Linux with Mono. 

## Getting Started

To work with Intel RealSense from .NET you will need two libraries next to your application - `realsense2.dll` and `Intel.RealSense.dll`. 

In order to get `realsense2.dll` you can either build the SDK [from source using CMake](https://github.com/IntelRealSense/librealsense/blob/master/doc/installation_windows.md) or [install the latest release](https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_windows.md).

Next, download the following prerequisites :

* Visual Studio 2017 only - [.NET Core 2.x](https://www.microsoft.com/net/download/visual-studio-sdks)

After installing all prerequisites, navigate to `/wrappers/csharp` and open either `Intel.RealSense.SDK.sln` for Visual Studio 2017+ or `Intel.RealSense.2015.sln` for Visual Studio 2015.

Press `Ctrl + Shift + B` to build the solution. 

If you choose to build the SDK from source and do so to a custom directory you may need to specify the path to `realsense2.dll` in the Intel.RealSense[.2015].csproj. This can be done by manually editing the csproj to add a BuildPath[x86/x64] property above the default.
e.g.
```
  <PropertyGroup>
    ...
    <BuildPathx86>your\path\to\x86\realsense2.dll</BuildPathx86> <!--[Optional] If you plan to build for x86-->
    <BuildPathx64>your\path\to\x64\realsense2.dll</BuildPathx64> <!--[Optional] If you plan to build for x64-->
    <BuildPath Condition="'$(BuildPath)'==''">..\..\..\build\$(Configuration)\realsense2.dll</BuildPath> <!--Existing default location-->
  </PropertyGroup>
```

## Hello World

Here is a minimal depth application written in C#: 

```cs
var pipe = new Pipeline();
pipe.Start();

while (true)
{
    using (var frames = pipe.WaitForFrames())
    using (var depth = frames.First(x => x.Profile.Stream == Stream.Depth) as DepthFrame)
    {
        Console.WriteLine("The camera is pointing at an object " +
            depth.GetDistance(depth.Width / 2, depth.Height / 2) + " meters away\t");

        Console.SetCursorPosition(0, 0);
    }
}
```

> **Note:** Since the SDK is holding-on to native hardware resources, it is critical to make sure you deterministically call `Release` on objects, especially those derived from `Frame`. Without releasing resources explicitly the Garbage Collector will not keep-up with new frames being allocated. Take advantage of `using` whenever you work with frames. 

## Linux and Mono

TBD

## Unity Integration 

TBD
