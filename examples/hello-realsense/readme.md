# rs-hello-realsense Sample

## Overview

Hello RealSense example demonstrates the basics of connecting to a RealSense device and taking advantage of depth data by printing the distance to object in the center of camera feild of view.

## Expected Output
Assuming camera is connected you should see `"The camera is facing an object X meters away"` line being continously updated. X is the distance in meters to the object in the center of camera feild of view.

## Code Overview 

First, we include the Intel® RealSense™ Cross-Platform API.  
All but advanced functionality is provided through a single header:
```cpp
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
```

Next, we create and start RealSense pipeline. Pipeline is the primary high level primitive controlling camera enumeration and streaming. 
```cpp
// Create a Pipeline - this serves as a top-level API for streaming and processing frames
rs2::pipeline p;

// Configure and start the pipeline
p.start();
```
Once pipeline is configured, we can loop waiting for new frames.

RealSense cameras usually offer multiple video, motion or pose streams. `wait_for_frames` function will block until next set of coherent frames from various configured streams. 
```cpp
// Block program until frames arrive
rs2::frameset frames = p.wait_for_frames();
```
To get first frame from the depth data stream, you can use `get_depth_frame` helper function:
```
// Try to get a frame of a depth image
rs2::depth_frame depth = frames.get_depth_frame();
```
Next we query the default depth frame dimensions (these may differ from sensor to sensor):
```cpp
// Get the depth frame's dimensions
float width = depth.get_width();
float height = depth.get_height();
```
To get distance at specific pixel (center of the frame), you can use `get_distance` function:
```
// Query the distance from the camera to the object in the center of the image
float dist_to_center = depth.get_distance(width / 2, height / 2);
```
The only thing left is to print the resulting distance to the screen:
```cpp
// Print the distance
std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";
```