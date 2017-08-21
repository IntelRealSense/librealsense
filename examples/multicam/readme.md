# rs-multicam sample

## Overview

The multicam sample demonstrates the ability to use the SDK for streaming and rendering multiple devices.

## Expected Output

The application should open a window that is split to a number of view ports (as the number of connected cameras).
Each part of the split window should display a single stream from a different camera, and at the top left should appear the name of the stream. 

In the following example we've used two Intel® RealSense™ Depth Cameras pointing at different locations.

<p align="center"><img src="example_screenshot.gif" alt="screenshot gif"/></p>

## Code Overview 

As with any SDK application we include the Intel RealSense Cross Platform API:

```cpp
#include <librealsense2/rs.hpp>     // Include RealSense Cross Platform API
```

In this example we will also use two auxiliary APIs:

```cpp
#include <librealsense2/rsutil.hpp> // Include Config utility class
#include "example.hpp"              // Include short list of convenience functions for rendering
```

`examples.hpp` lets us easily open a new window and prepare textures for rendering.

`librealsense2/rsutil.hpp` contains common utilities that assist with device configuration, and with devices connections and disconnections.

We define some additional objects to help manage the state of the application:
```cpp
// Structs for managing connected devices
struct dev_data { rs2::device dev; texture tex; rs2::frame_queue queue; };
struct state { std::mutex m; std::map<std::string, dev_data> frames; rs2::colorizer depth_to_frame; };
```

And declare two helper functions to handle the multiple devices:

```cpp
// Helper functions
void add_device(state& app_state, rs2::device& dev);
void remove_devices(state& app_state, const rs2::event_information& info);
```
We will step into these functions later in this overview.


The first object we use is a `window`, that will be used to display the images from all the cameras.

```cpp
// Create a simple OpenGL window for rendering:
window app(1280, 960, "CPP Multi-Camera Example");
```

The `window` class resides in `example.hpp` and lets us easily open a new window and prepare textures for rendering.

Declare a `state` object:
```cpp
// Construct an object to help track connected cameras
state app_state;
```

This object encapsulates the connected cameras and is used to manage the state of the program.

Next, we declare a `rs2::context`.

```cpp
// Create librealsense context for managing devices
context ctx;
```
`context` encapsulates all of the devices and sensors, and provides some additional functionalities.

In this example we will ask the `context` to notify us on device changes (connection and disconnections).
 
```cpp
// Register callback for tracking which devices are currently connected
ctx.set_devices_changed_callback([&app_state](event_information& info){
    remove_devices(app_state, info);

    for (auto&& dev : info.get_new_devices())
        add_device(app_state, dev);
});
```
Using `set_devices_changed_callback` we can register any callable object that accepts `event_information&` as its paramter.
`event_information` contains a list of devices that were disconnected, and the list of the currently connected devices.
In this case, once the `context` notifies us about a change, we update the program's `state` to reflect the current connected devices.

After we registered to get notification from the `context` we query it for all connected devices, and add each one:
```cpp
// Initial population of the device list
for (auto&& dev : ctx.query_devices()) // Query the list of connected RealSense devices
    add_device(app_state, dev);
```

`add_device` adds the device to the list of devices in use, configures it and starts it.
To do that, we declare a `Config` object that will help with device configuration:

```cpp
// Helper function for adding new devices to the application
void add_device(state& app_state, rs2::device& dev)
{
    // Create a config object to help configure how the device will stream
    rs2::util::config c;

```

Using the `Config`, we check if the device supports the depth stream, and if not, we choose the color stream by default.

```cpp
    // Select depth if possible, default to color otherwise
    if (c.can_enable_stream(dev, RS2_STREAM_DEPTH, rs2::preset::best_quality))
        c.enable_stream(RS2_STREAM_DEPTH, rs2::preset::best_quality);
    else
        c.enable_stream(RS2_STREAM_COLOR, rs2::preset::best_quality);
```

In order to start streaming from a device, we first need to `open` it and only later `start` it:

```cpp
    // Open the device for streaming
        auto s = c.open(dev);
```

Update the `state` to hold this new device, and create a `frame_queue` inplace.
We use the device's serial number as the unique identifier of the device:
```cpp
    // Register device in app state
    std::string sn(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    std::lock_guard<std::mutex> lock(app_state.m);
    app_state.frames.emplace(sn, dev_data{ dev, texture(), rs2::frame_queue(1) });
```

`frame_queue` is a container of frames that implements a queue. It is initialized with a capacity of `1`, meaning that 
only a single (the latest) frame is kept inside the queue. We use this queue as a "double buffer", to allow the application to handle frames without blocking the arrival of newer frames.
 
The last thing to do is start the device:

```cpp
    // Start streaming. The callback processes depth frames and then stores them for displaying
    s.start([&app_state, sn](rs2::frame f) {
        std::lock_guard<std::mutex> lock(app_state.m);
        app_state.frames[sn].queue(app_state.depth_to_frame(f));
    });
}
```

When calling `start` we can register any callable object that accepts a `frame` as its paramter.
In this case we register a lambda expression that upon frame arrival, simply puts the frame in the queue.

(Back to main...)

After adding the device, we begin our main loop of the application.
In this loop we will display all of the streams from all the devices.

```cpp
while (app) // Application still alive?
{
    // This app uses the device list asyncronously, so we need to lock it
    std::lock_guard<std::mutex> lock(app_state.m);
```

Every iteration we calculate the width, height and position of every view port (which displays a single stream):

```cpp
    // Calculate the size of each stream given stream count and window size
    int c = std::ceil(std::sqrt(app_state.frames.size()));
    int r = std::ceil(app_state.frames.size() / static_cast<float>(c));
    float w = app.width()/c;
    float h = app.height()/r;
```

Then, we take a single frame from each stream, and render it to the screen (in its correct location):
```cpp
    int stream_no = 0;
    for (auto&& kvp : app_state.frames)
    {
        auto&& device = kvp.second;
        // If we've gotten a new image from the device, upload that
        frame f;
        if (device.queue.poll_for_frame(&f))
            device.tex.upload(f);

        // Display the latest frame in the right position on screen
        device.tex.show({ w*(stream_no%c), h*(stream_no / c), w, h });
        ++stream_no;
    }
}
```

