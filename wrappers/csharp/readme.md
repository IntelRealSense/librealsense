# .NET Wrapper for Intel RealSense SDK

This folder offers P/Invoke based bindings for most SDK APIs, together with a couple of examples.

## Getting Started

To work with Intel RealSense from .NET you will need two libraries next to your application - `realsense2.dll` and `Intel.RealSense.dll`. 

In order to get `realsense2.dll` you can either build the SDK [from source using CMake](https://github.com/IntelRealSense/librealsense/blob/master/doc/installation_windows.md) or [install the latest release](https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_windows.md).

Next, download the following prerequisites :

* Visual Studio 2015/2017
* .NET framework >= 3.5 (.NET 3.5 is required for Unity the unity wrapper)


After installing all prerequisites, generate 'realsense2.sln' with 'BUILD_CSHARP_BINDINGS' and 'BUILD_SHARED_LIBS' flags using cmake.

- Generate the VS solution using cmake (run from librealsense root dir):
  - mkdir build
  - cd build
  - cmake .. -DBUILD_CSHARP_BINDINGS=ON -DBUILD_SHARED_LIBS=ON

The file 'realsense2.sln' should be created in 'build' folder, open the file with Visual Studio, C# examples and library will be available in the solution under 'Wrappers/csharp'.

Both the native library and the .NET wrapper are built by default as part of the examples dependencies.

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
