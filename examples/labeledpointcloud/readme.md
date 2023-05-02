# rs-labeledpointcloud Sample

## Overview

This sample demonstrates how to generate and visualize textured 3D labeled pointcloud.


## Expected Output
The application should open a window with labeled pointcloud. Using your mouse, you should be able to interact with the pointcloud rotating and zooming using the mouse.

## Code Overview

Similar to the [first tutorial](../capture/) we include the Cross-Platform API:
```cpp
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
```

Next, we prepared a [very short helper library](../example.hpp) encapsulating basic OpenGL rendering and window management:
```cpp
#include "example.hpp"          // Include short list of convenience functions for rendering
```

We also include the STL `<algorithm>` header for `std::min` and `std::max`.

Next, we define a `state` struct and two helper functions. `state` and `register_glfw_callbacks` handle the pointcloud's rotation in the application, and `draw_pointcloud` makes all the OpenGL calls necessary to display the pointcloud.
```cpp
// Struct for managing rotation of labeled pointcloud view
struct state { double yaw, pitch, last_x, last_y; bool ml; float offset_x, offset_y; texture tex; };

// Helper functions
void register_glfw_callbacks(window& app, state& app_state);
void draw_pointcloud(window& app, state& app_state, rs2::points& points);
```

The `example.hpp` header lets us easily open a new window and prepare textures for rendering. The `state` class (declared above) is used for interacting with the mouse, with the help of some callbacks registered through glfw.
```cpp
// Create a simple OpenGL window for rendering:
window app(1280, 720, "Lebeled RealSense Pointcloud Example");
// Construct an object to manage view state
state app_state = { 0, 0, 0, 0, false, 0, 0, 0 };
// register callbacks to allow manipulation of the pointcloud
register_glfw_callbacks(app, app_state);
```

We are going to use classes within the `rs2` namespace:
```cpp
using namespace rs2;
```

The `Pipeline` class is the entry point to the SDK's functionality:
```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
pipeline pipe;

// Defining a streaming configuration with the labeled point cloud streaming
rs2::config cfg;
cfg.enable_stream(RS2_STREAM_LABELED_POINT_CLOUD);

// Start streaming with this configuration
pipe.start(cfg);
```

Next, we wait in a loop unit for the next set of frames:
```cpp
auto data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
```

Using helper functions on the `frameset` object we check for new depth and color frames. We pass it to the `pointcloud` object to use as the texture, and also give it to OpenGL with the help of the `texture` class. We generate a new pointcloud.
```cpp
// Wait for the next set of frames from the camera
auto frames = pipe.wait_for_frames();
auto labels_frame = data.get_labeled_point_cloud_frame();

auto vertices = labels_frame.get_vertices();
auto vertices_size = labels_frame.size();

std::vector<rs2::vertex> vertices_vec;
vertices_vec.insert(vertices_vec.begin(), vertices, vertices + vertices_size)

auto labels = labels_frame.get_labels();

std::vector<uint8_t> labels_vec;
labels_vec.insert(labels_vec.begin(), labels, labels + vertices_size);
```

Finally we call `draw_labeled_pointcloud` to draw the labeled pointcloud.
```cpp
draw_labeled_pointcloud(app, app_state, points);
```

`draw_labeled_pointcloud` is primarily calls to OpenGL, but the critical portion iterates over all the vertices in the labeled pointcloud, and draw them with the color we have associated to their label.
```cpp
/* this segment actually renders the labeled pointcloud */
for (int i = 0; i < labels_to_vertices.size(); ++i)
{
    auto label = labels_to_vertices[i].first;
    auto vertices = labels_to_vertices[i].second;
    auto color = label_to_color3f[label];

    for (auto&& v : vertices)
    {
        GLfloat vert[3] = {-v.y, v.z, -v.x};
        glVertex3fv(vert);
        glColor3f(color.x, color.y, color.z);
    }
}
```
