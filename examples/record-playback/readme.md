# rs-record-playback Sample

## Overview

This example demonstrates usage of the recorder and playback devices.

## Expected Output
![expected output]

This application displays a point cloud of the depth frame, being recorded and then re-played.

## Code Overview

### Declarations

As with any SDK application we include the Intel RealSense Cross Platform API:

```cpp
#include <librealsense2/rs.hpp>
```

In this example we will also use the auxiliary library of `example.hpp`:

```cpp
#include "../example.hpp"    
```

`example.hpp` lets us easily open a new window and prepare textures for rendering.

In addition to the standard library included headers, we include 2 header files which will help us to render GUI controls in our window application:

```cpp
#include <imgui.h>
#include "imgui_impl_glfw.h"
```
These headers are part of the [ImGui](https://github.com/ocornut/imgui) library which we use to render GUI elements.

Also, we include header files used for measuring time in the playback and for convenient display of time:

```cpp
#include <sstream>
#include <iostream>
#include <iomanip>
```

We'll use 2 helper functions for rendering the GUI, to allow us to make to main code less verbose.

```cpp
// Helper function for dispaying time conveniently
std::string pretty_time(std::chrono::nanoseconds duration);
// Helper function for rendering a seek bar
void draw_seek_bar(rs2::playback& playback, int* seek_pos, float2& location, float width);
```

### Main

We first define some variables that will be used to show the window and GUI.

```cpp
// Create a simple OpenGL window for rendering:
window app(1280, 720, "RealSense Post Processing Example");
ImGui_ImplGlfw_Init(app, false);
 ```

Next, we define a `texture`, which is a class designed to hold video frame data for rendering, `rs2::frameset ` which will hold
a set of frames from the camera and `rs2::frame` for the depth frame.
```cpp
// Declare a texture for the depth image on the GPU
texture depth_image;

// Declare frameset and frames which will hold the data from the camera
rs2::frameset frames;
rs2::frame depth;
 ```

Also, we define an `rs2::colorizer` to allow the point cloud visualization have a texture:
```cpp
// Declare depth colorizer for pretty visualization of depth data
 rs2::colorizer color_map;
 ```

Now we need to create an `rs2::pipeline` which is a top level API for using RealSense depth cameras.
`rs2::pipeline` automatically chooses a camera from all connected cameras which matches the given configuration, so we can simply call `pipeline::start(cfg)` and the camera is configured and streaming.
In order to allow different types of devices (recorder, playback) to use the pipeline, we define a shared pointer to `rs2::pipeline`. 
For now, we configure the pointed pipeline with the default configuration and start streaming the default stream. We'll use this stream
whenever there is no recorder or playback running, for continuous display of images from the camera.

```cpp
// Create a shared pointer to a pipeline
auto pipe = std::make_shared<rs2::pipeline>();
// Start streaming with default configuration
pipe->start();
```

In addition, we initialize another shared pointer, this time one which points to the device. This will let us release resources held
by the device later on in the code.

```cpp
// Initialize a shared pointer to a device with the current device on the pipeline
auto device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device());
```

Last, we declare a few booleans to help us control the code: `first_iteration_rec` indicates whether we need to reset `pipeline` and
`recorder` resources and restart with a new configuration, or continue with the previous one; the same goes for `first_iteration_play`
with `playback` instead of `recorder`. 
`recorded` indicates whether we alrady recorded a file available for playing.

```cpp
// Create booleans to control the record-playback flow
bool first_iteration_rec = true;
bool first_iteration_play = true;
bool recorded = false;
```

`seek_pos` is used for allowing the user control the playback.

```cpp
// Create a variable to control the seek bar
int seek_pos;
```

### Record and Playback

First, we wait for frames from the camera for rendering. This is relevant for frames which arrive from the recorder or from the
device with the default configuration, because those two are streaming live. We can not use `wait_for_frames` with the playback,
because we might run out of frames.
	
```cpp
// If the device is sreaming live and not from a file
if (!device->as<rs2::playback>())
{
    frames = pipe->wait_for_frames(); // wait for next set of frames from the camera
    depth = color_map(frames.get_depth_frame()); // Find and colorize the depth data
}	
```

We begin with recorder functions.