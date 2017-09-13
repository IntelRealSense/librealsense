# rs-align Sample

## Overview

This sample demonstrates usage of the `rs2::align` object, which allows users to align 2 streams (projection wise).

In this example, we align depth frames to their corresponding color frames. Then, we use the two frames to determine the depth of each color pixel.

Using this information, we "remove" the background of the color frame that is further (away from the camera) than some user-define distance.

The example display a GUI for controlling the max distance to show from the original color image.

## Expected Output

The application should open a window and display a video stream from the camera. 

The window should have the following elements:
- On the left side of the window is a vertical silder for controlling the depth clipping distance.
- A color image with grayed out background
- A corresponding (colorized) depth image.


## Code Overview 

As with any SDK application we include the Intel RealSense Cross Platform API:

```cpp
#include <librealsense2/rs.hpp>
```

In this example we will also use the auxiliary library of `example.hpp`:

```cpp
#include "../example.hpp"    
```

`examples.hpp` lets us easily open a new window and prepare textures for rendering.

We include 2 more header files which will help us to render GUI controls in our window application:

```cpp
#include <imgui.h>
#include "imgui_impl_glfw.h"
```

These headers are part of the [ImGui](https://github.com/ocornut/imgui) library which we use to render GUI elements. 

Next, we declare three functions to help the code look clearer:
```cpp
void render_slider(rect location, float& clipping_dist);
void remove_background(rs2::video_frame& color, const rs2::depth_frame& depth_frame, float depth_scale, float clipping_dist);
bool try_get_depth_scale(rs2::pipeline& p, float& scale);
```

`render_slider(..)`  is where all the GUI code goes, and we will not cover this function in this overview.

`remove_background(..)` takes depth and color images (that are assumed to be aligned to one another), the depth scale units, and the maximum distance the user wishes to show, and updates the color frame so that its background (any pixel with depth distance larger than the maximum allowed) is removed.

`try_get_depth_scale(..)` is a simple function that tries to find a depth sensor from the pipeline's device and retrieves its depth scale units.

Heading to `main`:

We first define some variables that will be used to show the window and render the images and GUI to the screen:

```cpp
// Create and initialize GUI related objects
    window app(1280, 720, "CPP - Align Example"); // Simple window handling
    ImGui_ImplGlfw_Init(app, false);      // ImGui library intializition 
    rs2::colorizer c;                          // Helper to colorize depth images
    texture renderer;                     // Helper for renderig images
```

Then, we create a context:

```cpp
// Create a context
rs2::context ctx;
```
`rs2::context` encapsulates all of the devices and sensors, and provides some additional functionalities.
In this example we will use the `context` to create an `rs2::align` object:

```cpp
    // Using the context to create a rs2::align object. 
    // rs2::align allows you to perform alignment of depth frames to others
    rs2::align align = ctx.create_align(RS2_STREAM_COLOR); 
```

`rs2::align` is a utility class that performs image alignment (registration) of 2 frames. Basically, each pixel from the first image will be transformed so that it matches its corresponding pixel in the second image. 
A `rs2::align` object always transforms depth images to some target image which, in this example, is specified with the `RS2_STREAM_COLOR` parameter.

Next, we define a `rs2::pipeline` which is a top level API for using RealSense depth cameras.
`rs2::pipeline` automatically chooses a camera from all connected cameras, so we can simply call `pipeline::start()` and the camera is configured and streaming:

```cpp
// Create a pipeline to easily configure and start the camera
rs2::pipeline pipe;
pipe.start(align);
```
we pass the `align` object to `Pipeline::start` as the post processing handler for incoming frames that the pipeline produces. This means that frames will first be processed by the pipeline, and once the pipeline is ready to output them, it will pass them on to `align` which will perform additional processing.

`rs2::align` contains an overload for `operator()` which takes a `rs2::frame`. This allows us to pass it as the argument for `pipeline::start`. The pipeline synchronizes its frames before publishing them, thus, by calling `start` with an `align` object, the frames it will provide will be synchronized and aligned to one another. We will soon see how to get these frames from the `align` object.

At this point of the program the camera is configured and the resulted frames are being aligned.

Before actually using the frames, we try to get the depth scale units of the depth camera. Depth scale units are used to convert the depth pixel data (16-bit unsigned) into metric units.

These units are expressed as depth in meters corresponding to a depth value of 1. For example if we have a depth pixel with a value of 2 and the depth scale units are 0.5 then that pixel is `2 X 0.5 = 1` meter away from the camera.

Now comes the interesting part of the application. We start our main loop, which breaks only when the window is closed:


```cpp
while (app) // Application still alive?
{
```

Inside the loop, the first thing we do is block the program until the `align` object returns a `rs2::frameset`. A `rs2::frameset` is an object that holds a set of frames and provides an interface for easily accessing them.

```cpp
    // Using the align object, we block the application until a frameset is available
    rs2::frameset frameset = align.wait_for_frames();
```

The `frameset` returned from `wait_for_frames` should contain a set of aligned frames, but we should check that the frames it contains are indeed valid:


```cpp
    // Trying to get both color and aligned depth frames
    rs2::video_frame color_frame = frameset.get_color_frame();
    rs2::depth_frame aligned_depth_frame = frameset.get_depth_frame();

    //If one of them is unavailable, try to obtain another frameset
    if (!aligned_depth_frame || !color_frame)
    {
        continue;
    }
```
Notice that the color frame is of type `rs2::video_frame` and the depth frame if of type `rs2::depth_frame` (which derives from `rs2::video_frame` and adds special depth related functionality).

After getting the two aligned color and depth frames, we call the `remove_background(..)` function to strip the background from the color image.
This is a simple function that performs a naive algorithm for background segmentation.
```cpp
    // Passing both frames to remove_background so it will "strip" the background
    // NOTE: in this example, we alter the buffer of the color frame, instead of copying it and altering the copy
    //  This behavior is not recommended in real application since the color frame could be used elsewhere
    remove_background(color_frame, aligned_depth_frame, depth_scale, depth_clipping_distance);

```

The  rest of the loop contains code that takes care of rendering and GUI controls. We will not elaborate on it. Let's go over `remove_background(..)` instead:


```cpp
void remove_background(rs2::video_frame& other_frame, const rs2::depth_frame& depth_frame, float depth_scale, float clipping_dist)
{
```

In the beginning of function, we take a pointer to the raw buffer of both frames, so that we could alter the color image (instead of creating a new buffer). 

```cpp
    const uint16_t* p_depth_frame = reinterpret_cast<const uint16_t*>(depth_frame.get_data());
    uint8_t* p_other_frame = reinterpret_cast<uint8_t*>(const_cast<void*>(other_frame.get_data()));
```

Next, we go over each pixel of the frame.

```cpp
    //Using OpenMP to try to parallelise the loop
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; y++)
    {
        auto depth_pixel_index = y * width;
        for (int x = 0; x < width; x++, ++depth_pixel_index)
        {
```
Calculate the depth distance of that pixel:
```cpp
            // Get the depth value of the current pixel
            auto pixels_distance = depth_scale * p_depth_frame[depth_pixel_index];

```

If that distance is invalid (`pixels_distance <= 0.f`) or further away than the maximum distance that the user requested (`pixels_distance > clipping_dist`) then we should strip off that pixel from the resulted color image.
```cpp
            // Check if the depth value is invalid (<=0) or greater than the threashold
            if (pixels_distance <= 0.f || pixels_distance > clipping_dist)
            {
```
By "strip off" we mean that we simply paint that pixel with a gray color.

```cpp
                // Calculate the offset in other frame's buffer to current pixel
                auto offset = depth_pixel_index * other_bpp;

                // Set pixel to "background" color (0x999999)
                std::memset(&p_other_frame[offset], 0x99, other_bpp);
            }
        }
    }
```

