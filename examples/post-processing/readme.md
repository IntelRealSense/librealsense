# rs-post-processing Sample

## Overview

This example demonstrates usage of the following processing blocks:

* Decimation
  * Intelligently reduces the resolution of a depth frame.
* Disparity
  * Performs transformation between depth and disparity domains.
  * Applicable for stereo-based depth sensors only (e.g. D400).
* Spatial
  * Applies edge-preserving smoothing of depth data.
* Temporal
  * Filters depth data by looking into previous frames.


## Expected Output
![expected output](https://user-images.githubusercontent.com/22654243/35924136-dd9cd1b6-0c2a-11e8-925a-84a52c0a5b96.gif)

This application displays a rotating point cloud of the depth frame, with GUI for controlling the filters' options, and a checkbox to enable/disable each one.

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

`examples.hpp` lets us easily open a new window and prepare textures for rendering.

In addition to the standard library included headers, we include 2 more header files which will help us to render GUI controls in our window application:

```cpp
#include <imgui.h>
#include "imgui_impl_glfw.h"
```

These headers are part of the [ImGui](https://github.com/ocornut/imgui) library which we use to render GUI elements.

Next we define 2 auxiliary classes, the first to represent a slider GUI element for each option of a filter, and the second is a class for encapsulating a filter alongside its options.

```cpp
/**
Helper class for controlling the filter's GUI element
*/
struct filter_slider_ui
{
    std::string name;
    std::string label;
    std::string description;
    bool is_int;
    float value;
    rs2::option_range range;

    bool render(const float3& location, bool enabled);
    static bool is_all_integers(const rs2::option_range& range);
};

/**
Class to encapsulate a filter alongside its options
*/
class filter_options
{
public:
    filter_options(const std::string name, rs2::process_interface& filter);
    filter_options(filter_options&& other);
    std::string filter_name;                                   //Friendly name of the filter
    rs2::process_interface& filter;                            //The filter in use
    std::map<rs2_option, filter_slider_ui> supported_options;  //maps from an option supported by the filter, to the corresponding slider
    std::atomic_bool is_enabled;                               //A boolean controlled by the user that determines whether to apply the filter or not
};
```


`filter_options` holds a reference to the actual filter, as a `rs2::process_interface&` which allows options controls and calling the `process()` method.
In addition it holds a map between the filter's options and its matching slider. And finally it holds a boolean for toggling the user's wish to apply the filter or not.

In addition to the auxiliary classes, the example also uses two helper functions, to allow us to make to main code less verbose.

```cpp
// Helper functions for rendering the UI
void render_ui(float w, float h, std::vector<filter_options>& filters);
// Helper function for getting data from the queues and updating the view
void update_data(rs2::frame_queue& data, rs2::frame& depth, rs2::points& points, rs2::pointcloud& pc, glfw_state& view, rs2::colorizer& color_map);
```

### Main

We first define some variables that will be used to show the window, render the images and GUI to the screen and help exchange data between threads:

```cpp
// Create a simple OpenGL window for rendering:
window app(1280, 720, "RealSense Post Processing Example");
ImGui_ImplGlfw_Init(app, false);

// Construct objects to manage view state
glfw_state original_view_orientation{};
glfw_state filtered_view_orientation{};

// Declare pointcloud objects, for calculating pointclouds and texture mappings
rs2::pointcloud original_pc;
rs2::pointcloud filtered_pc;
```

Next, we define a `rs2::pipeline` which is a top level API for using RealSense depth cameras.
`rs2::pipeline` automatically chooses a camera from all connected cameras which matches the given configuration, so we can simply call `pipeline::start(cfg)` and the camera is configured and streaming.
In this example we only require a single Depth stream, and so we request the pipeline to stream only that `enabled` stream:

```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;
rs2::config cfg;
// Use a configuration object to request only depth from the pipeline
cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
// Start streaming with the above configuration
pipe.start(cfg);
```

At this point of the program the camera is configured and streams are available from the pipeline.

Now, we create the filters that we will use in the application:

```cpp
// Decalre filters
rs2::decimation_filter dec_filter;  // Decimation - reduces depth frame density
rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise

// Declare disparity transform from depth to disparity and vice versa
const std::string disparity_filter_name = "Disparity";
rs2::disparity_transform depth_to_disparity(true);
rs2::disparity_transform disparity_to_depth(false);
```

After constructing all filters, we wrap them with a `filter_option` and push them
one by one to a vector, **with order in mind**:

```cpp
// Initialize a vector that holds filters and their options
std::vector<filter_options> filters;

// The following order of emplacment will dictate the orders in which filters are applied
filters.emplace_back("Decimate", dec_filter);
filters.emplace_back(disparity_filter_name, depth_to_disparity);
filters.emplace_back("Spatial", spat_filter);
filters.emplace_back("Temporal", temp_filter);
```

When we'll use the filters, we will iterate over the vector, and since order of
iteration is promised to remain the same, we can safely assume that filtering will
be performed in that order (Decimate --> To Disparity --> Spatial --> Temporal).

Afterwards, we declare 2 concurrent frame queues to help us exchange data between threads.
```cpp
// Declaring two concurrent queues that will be used to push and pop frames from different threads
rs2::frame_queue original_data;
rs2::frame_queue filtered_data;
```

Then, a `rs2::colorizer` to allow the point cloud visualization have a texture:
```cpp
// Declare depth colorizer for pretty visualization of depth data
 rs2::colorizer color_map;
 ```
and finally, using an atomic boolean to help flag the processing thread that it should stop:
```cpp
// Atomic boolean to allow thread safe way to stop the thread
std::atomic_bool stopped(false);
```

### Processing Thread

The `processing_thread` will do the following actions, to make the main thread more responsive (by performing less computations and non blocking calls such as `wait_for_frames`):

1. Get the data from the pipeline, and hold an instance to a filtered frame:

  ```cpp
  // Create a thread for getting frames from the device and process them
  // to prevent UI thread from blocking due to long computations.
  std::thread processing_thread([&]() {
      while (!stopped) //While application is running
      {
          rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera

          rs2::frame depth_frame = data.get_depth_frame(); //Take the depth frame from the frameset
          if (!depth_frame) // Should not happen but if the pipeline is configured differently
              return;       //  it might not provide depth and we don't want to crash

          rs2::frame filtered = depth_frame; // Does not copy the frame, only adds a reference
  ```

2. Apply all filters to the depth frame, one after the other:

  ```cpp
          /* Apply filters.
          The implemented flow of the filters pipeline is in the following order:
          1. apply decimation filter
          2. transform the scence into disparity domain
          3. apply spatial filter
          4. apply temporal filter
          5. revert the results back (if step Disparity filter was applied
          to depth domain (each post processing block is optional and can be applied independantly).
          */
          bool revert_disparity = false;
          for (auto&& filter : filters)
          {
              if (filter.is_enabled)
              {
                  filtered = filter.filter.process(filtered);
                  if (filter.filter_name == disparity_filter_name)
                  {
                      revert_disparity = true;
                  }
              }
          }
          if (revert_disparity)
          {
              filtered = disparity_to_depth.process(filtered);
          }
  ```

3. Push the original depth frame, and the filtered one, each to its respective queue:

  ```cpp
          // Push filtered & original data to their respective queues
          // Note, pushing to two different queues might cause the application to display
          //  original and filtered pointclouds from different depth frames
          //  To make sure they are synchronized you need to push them together or add some
          //  synchronization mechanisms
          filtered_data.enqueue(filtered);
          original_data.enqueue(depth_frame);
      }
  });
  ```

### Main thread

In the main thread, we shall render all GUI elements, and display the latest available
pointcloud of the original stream, and the filtered one. To do that, we declare the following:

```cpp
// Declare objects that will hold the calculated pointclouds and colored frames
// We save the last set of data to minimize flickering of the view
rs2::frame colored_depth;
rs2::frame colored_filtered;
rs2::points original_points;
rs2::points filtered_points;
```

Next, prior to starting the main loop, we define some additional variables to help with the rotation of the pointcloud:
```cpp
// Save the time of last frame's arrival
auto last_time = std::chrono::high_resolution_clock::now();
// Maximum angle for the rotation of the pointcloud
const double max_angle = 90.0;
// We'll use rotation_velocity to rotate the pointcloud for a better view of the filters effects
float rotation_velocity = 0.3f;
```

The main loop, iterates as long as the window is opened, it renders all the GUI
elements, which are comprised of checkbox per filter and sliders per filter's option
for controlling each option.

```cpp
while (app)
{
    float w = static_cast<float>(app.width());
    float h = static_cast<float>(app.height());

    // Render the GUI
    render_ui(w, h, filters);
```

Next, the main thread calls `update_data` for the original and filtered data:

```cpp
    // Try to get new data from the queues and update the view with new texture
    update_data(original_data, colored_depth, original_points, original_pc, original_view_orientation, color_map);
    update_data(filtered_data, colored_filtered, filtered_points, filtered_pc, filtered_view_orientation, color_map);

```

`update_data` does the following:

```cpp
void update_data(rs2::frame_queue& data, rs2::frame& depth, rs2::points& points, rs2::pointcloud& pc, glfw_state& view, rs2::colorizer& color_map)
{
    rs2::frame f;
    if (data.poll_for_frame(&f))  // Try to take the depth and points from the queue
    {
        points = pc.calculate(f); // Generate pointcloud from the depth data
        depth = color_map(f);     // Colorize the depth frame with a color map
        pc.map_to(depth);         // Map the colored depth to the point cloud
        view.tex.upload(depth);   //  and upload the texture to the view (without this the view will be B&W)
    }
}
```

It tries to get a frame from the relevant queue, which either holds a depth frames
or a filtered depth frame.
It then passes the frame to the `rs2::pointcloud` object for calculating a point
cloud from that depth frame. Then, the depth frame is colorized with a color map.
The colored depth frame is then mapped to the previously calculated pointcloud,
so that it can be used as its texture, and finally it is uploded to the view, for rendering.

After updating the pointcloud objects, we add some text per view port, and call
the `draw_pointcloud` function from `example.hpp` to instruct OpenGL to render it.

```cpp
    draw_text(10, 50, "Original");
    draw_text(static_cast<int>(w / 2), 50, "Filtered");

    // Draw the pointclouds of the original and the filtered frames (if the are available already)
    if (colored_depth && original_points)
    {
        glViewport(0, h / 2, w / 2, h / 2);
        draw_pointcloud(w / 2, h / 2, original_view_orientation, original_points);
    }
    if (colored_filtered && filtered_points)
    {
        glViewport(w / 2, h / 2, w / 2, h / 2);
        draw_pointcloud(w / 2, h / 2, filtered_view_orientation, filtered_points);
    }

```

At the end of the loop, we update some variables so that the next rendering calls
will display the point cloud in a slightly different orientation, thus making the
view rotate smoothly in the application.

```cpp
    // Update time of current frame's arrival
    auto curr = std::chrono::high_resolution_clock::now();

    // Time interval which must pass between movement of the pointcloud
    const std::chrono::milliseconds rotation_delta(40);

    // In order to calibrate the velocity of the rotation to the actual displaying speed, rotate
    //  pointcloud only when enough time has passed between frames
    if (curr - last_time > rotation_delta)
    {
        if (fabs(filtered_view_orientation.yaw) > max_angle)
        {
            rotation_velocity = -rotation_velocity;
        }
        original_view_orientation.yaw += rotation_velocity;
        filtered_view_orientation.yaw += rotation_velocity;
        last_time = curr;
    }
}
```

The main loop exists when the window closes.
At this point we signal the processing thread to stop, and wait for it to join.

```cpp
// Signal the processing thread to stop, and join
// (Not the safest way to join a thread, please wrap your threads in some RAII manner)
stopped = true;
processing_thread.join();

return EXIT_SUCCESS;
```
