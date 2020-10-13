# rs-pose Sample

> In order to run this example, a device supporting pose stream (T265) is required.

## Overview
This sample demonstrates how to obtain pose data from a T265 device.

## Expected Output
The application should open a window in which it prints the current x, y, z values of the device position relative to the initial position.

## Code Overview

First, we include the Intel® RealSense™ Cross-Platform API.  
All but advanced functionality is provided through a single header:
```cpp
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
```

We declare the pipeline and configure it with `RS2_STREAM_POSE` and `RS2_FORMAT_6DOF`. Then, we start the pipeline.
```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;
// Create a configuration for configuring the pipeline with a non default profile
rs2::config cfg;
// Add pose stream
cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
// Start pipeline with chosen configuration
pipe.start(cfg);
```

In each iteration, while the application is alive, we wait for new frames from the camera. From the frameset that arrives we get the frame of `RS2_STREAM_POSE` type.
```cpp
// Main loop
while (true)
{
    // Wait for the next set of frames from the camera
    auto frames = pipe.wait_for_frames();
    // Get a frame from the pose stream
    auto f = frames.first_or_default(RS2_STREAM_POSE);
```

We cast the frame that arrives to `pose_frame` in order to access its `pose_data`.
```cpp
// Cast the frame to pose_frame and get its data
auto pose_data = f.as<rs2::pose_frame>().get_pose_data();
```

Once we have `pose_data`, we can query information on the camera position and movement: 
- **Translation** (in meters, relative to initial position)
- **Velocity** (in meter/sec)
- **Rotation** (as represented in quaternion rotation, relative to initial position)
- **Angular velocity** (in radians/sec)
- **Angular acceleration** (in radians/sec^2)
- **Tracker confidence** (pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High)
- **Mapper confidence** (pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High)

In this example, we obtain the translation data and print it to the console.
```cpp
// Print the x, y, z values of the translation, relative to initial position
std::cout << "\r" << "Device Position: " << std::setprecision(3) << std::fixed << pose_data.translation.x << " " <<
    pose_data.translation.y << " " << pose_data.translation.z << " (meters)";
```
