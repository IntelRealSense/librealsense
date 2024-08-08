

# Sample Code for Intel® RealSense™ **Depth** cameras

This is a subset of SDK examples, for full list see [readme.md](./readme.md)

## List of Examples:

|Name | Language | Description | Experience Level | Technology |
|---|---|---|---|---|
|[Hello-RealSense](./hello-realsense) | C++ | Demonstrates the basics of connecting to a RealSense device and using depth data | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Distance](./C/distance) | C | Equivalent to `hello-realsense` but rewritten for C users | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Color](./C/color) | C | Demonstrate how to stream color data and prints some frame information | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Capture](./capture)| C++ | Shows how to synchronize and render multiple streams: left, right, depth and RGB streams | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Save To Disk](./save-to-disk)| C++ | Demonstrate how to render and save video streams on headless systems without graphical user interface (GUI) | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Pointcloud](./pointcloud)| C++ | Showcase Projection API while generating and rendering 3D pointcloud | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[ImShow](../wrappers/opencv/imshow) | C++ & OpenCV | Minimal OpenCV application for visualizing depth data | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Multicam](./multicam)| C++ | Present multiple cameras depth streams simultaneously, in separate windows | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Depth](./C/depth) | C | Demonstrates how to stream depth data and prints a simple text-based representation of the depth image | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Spatial Alignment](./align)| C++ | Introduces the concept of spatial stream alignment, using depth-color mapping | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Advanced Alignment](./align-advanced)| C++ | Show a simple method for dynamic background removal from video | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Measure](./measure)| C++ | Lets the user measure the dimensions of 3D objects in a stream | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Post Processing](./post-processing)| C++ | Demonstrating usage of post processing filters for depth images | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Record & Playback](./record-playback)| C++ | Demonstrating usage of the recorder and playback devices | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Motion](./motion)| C++ | Demonstrates how to use data from gyroscope and accelerometer to compute the rotation of the camera | :star::star: | [![Depth with IMU](https://img.shields.io/badge/-D435i-5bc3ff.svg)](./depth.md) |
|[DNN](../wrappers/opencv/dnn)| C++ & OpenCV | Intel RealSense camera used for real-time object-detection | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Software Device](./software-device)| C++ | Shows how to create a custom `rs2::device` | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Sensor Control](./sensor-control)| C++ | A tutorial for using the `rs2::sensor` API | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[GrabCuts](../wrappers/opencv/grabcuts)| C++ & OpenCV | Simple background removal using the GrabCut algorithm | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)  |
|[Latency](../wrappers/opencv/latency-tool)| C++ & OpenCV | Basic latency estimation using computer vision | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)  |
