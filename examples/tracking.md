

# Sample Code for Intel® RealSense™ **Tracking** cameras

This is a subset of SDK examples, for full list see [readme.md](./readme.md)

## List of Examples:

|Name | Language | Description | Experience Level | Technology |
|---|---|---|---|---|
|[Capture](./capture)| C++ | Shows how to synchronize and render multiple streams: left, right, depth and RGB streams | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md)|
|[Save To Disk](./save-to-disk)| C++ | Demonstrate how to render and save video streams on headless systems without graphical user interface (GUI) | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md) |
|[Pose](./pose)|C++|Demonstarates how to obtain data from pose frames| :star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md)|
|[Multicam](./multicam)| C++ | Present multiple cameras depth streams simultaneously, in separate windows | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md) |
|[Pose Prediction](./pose-predict)|C++|Demonstarates how to use tracking camera asynchroniously to implement simple pose prediction | :star::star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md)|
|[Trajectory](./trajectory)| C++ | Shows how to calculate and render 3D trajectory based on pose data from a tracking camera | :star::star::star: | [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md)
|[Software Device](./software-device)| C++ | Shows how to create a custom `rs2::device` | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md) |
|[Sensor Control](./sensor-control)| C++ | A tutorial for using the `rs2::sensor` API | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](./tracking.md) |