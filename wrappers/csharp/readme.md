# .NET Wrapper for Intel RealSense SDK

This folder offers P/Invoke based bindings for most SDK APIs, together with a couple of examples. At the moment we only offer Visual Studio solution file to build on Windows, but it should be possible to use the same bindings on Linux with Mono. 

## Getting Started

To work with Intel RealSense from .NET you will need two libraries next to your application - `realsense2.dll` and `LibrealsenseWrapper.dll`. 

In order to get `realsense2.dll` you can either build the SDK [from source using CMake](https://github.com/IntelRealSense/librealsense/blob/master/doc/installation_windows.md) or [install the latest release](https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_windows.md).

Next navigate to `/wrappers/csharp` and open `Intel.RealSense.SDK.sln` with Visual Studio. 

Press `Ctrl + Shift + B` to build the solution. 

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
