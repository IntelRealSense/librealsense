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

// Create booleans to control GUI (recorded - allow play button, recording - show 'recording to file' test)
bool recorded = false;
bool recording = false;
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
First, we allow recording only if playback is not currently running.

```cpp
if (!device->as<rs2::playback>()) 
```

There are two cases where the 'record' button would be clicked: begin a recodring, or resume a recorder that has already started.
We check whether device is a recorder to indicate which of the cases it is.

In the first case, we stop the pipeline and initiate the shared pointer with a new pipeline. Then initiate a new configuration, allowing
recording to the file 'a.bag' using the function `enable_record_to_file`, and start the pipeline with the new configuration. Also, we update
the device shared pointer to point to the current (recorder) device.

```cpp
// If it is the start of a new recording
// If it is the start of a new recording (device is not a recorder yet)
if (!device->as<rs2::recorder>())
{
	pipe->stop(); // Stop the pipeline with the default configuration
	pipe = std::make_shared<rs2::pipeline>();
	rs2::config cfg; // Declare a new configuration
	cfg.enable_record_to_file("a.bag");
	pipe->start(cfg); //File will be opened at this point

	// Reset the shared pointer and make it point to the current device
	device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device());
}
```

In the second case, the device is already a recorder, so we can just use it's 'resume' function. In order to acces function of `rs2::recorder`,
we use `as<rs2::recorder>()` .

```cpp
else { // If the recording is resumed after a pause, there's no need to reset the shared pointers
	device->as<rs2::recorder>().resume(); // rs2::recorder allows access to 'resume' function
}
```

We can pause the recording using the 'pause' function of the `rs2::recorder`.

```cpp
device->as<rs2::recorder>().pause();
```

To stop recording, we need to stop the pipleine: `pipe->stop()`. We also have to release any resources that the pipeline and device hold,
including the 'a.bag' file, therefore we reset both shared pointers. 
We inititate a new pipeline with default configuration, which allows us to render frames from the camera while no recorder or playback are running.


```cpp
pipe->stop(); // Stop the pipeline that holds the file and the recorder
pipe.reset();
device.reset(); // Reset the shared pointer to the device that holds the file
pipe = std::make_shared<rs2::pipeline>(); //Reset the shared pointer with a new pipeline
pipe->start(); // Resume streaming with default configuration
device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device()); // Point to the current device
recorded = true; // Now we can run the file
recording = false;
```

In a similar way, if we have a recorded file we can play it using the 'playback' device. The flow is the same as for the recorder, only this time
we use `enable_device_from_file` function on the configuration.

In order to render frames from the playback, we check if there are ready frames in the pipeline using `poll_for_frames`.
We also call `draw_seek_bar` function which allows the user to control the playback.

```cpp
if (pipe->poll_for_frames(&frames)) // Check if new frames are ready
{
	depth = color_map(frames.get_depth_frame()); // Find and colorize the depth data for rendering
}

// Render a seek bar for the player
float2 location = { app.width() / 4, 4 * app.height() / 5 + 100 };
draw_seek_bar(device->as<rs2::playback>(), &seek_pos, location, app.width() / 2);
```

Finally, depth rendering is implemented by the `texture` class from [example.hpp](../example.hpp)
```cpp
// Render depth frames from the default configuration, the recorder or the playback
depth_image.render(depth, { app.width() / 4, 0, 3 * app.width() / 5, 3 * app.height() / 5 + 50 });
```
