# rs-align Sample

## Overview

This sample demonstrates usage of the `rs2::software_device` object, which allows users to create software only device.

software_device can be used when user need to use SDK logic but want to inject synthetic frames or frames that he captured from device by other application.

The SDK logic includes:

record and playback services, post processing filters, alignmant of streams, syncronizion of streams and point cloud.

In this example we will show how to create synthetic depth and synthetic texture
injects them to software_device, use sync to synchronize the both streams and use pointcloud
to create the 3D model and to get the texture mapping.

## Expected Output

The application should open a window and display the 3D model of the synthetic depth we created, mapped to the synthetic texture.

<p align="center"><img src="https://user-images.githubusercontent.com/22448952/34941693-8195b372-f9fd-11e7-9ca1-3e39aef3ce98.png" alt="screenshot png"/></p>


## Code Overview

As with any SDK application we include the Intel RealSense Cross Platform API:

```cpp
#include <librealsense2/rs.hpp>
```
In addition we include also API for software device:

```cpp
#include <librealsense2/hpp/rs_internal.hpp>
```
In this example we will also use the auxiliary library of `example.hpp`:

```cpp
#include "../example.hpp"
```

`examples.hpp` lets us easily open a new window and prepare textures for rendering.


Next, we declare two functions to help the code look clearer:
```cpp
synthetic_frame create_synthetic_texture();
void fill_synthetic_depth_data(void* data, int w, int h, int bpp, float wave_base)
```

`create_synthetic_texture()`  loads png image for the texture.

`fill_synthetic_depth_data(..)` creates the synthetic depth frame.

Heading to `main`:

We first define the dimensions of the depth frames:
```cpp
const int W = 640;
const int H = 480;
const int BPP_D = 2;
```

Next, we define the depth intrinsics:
```cpp
rs2_intrinsics depth_intrinsics{ W, H, W / 2, H / 2, W , H , RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };
```

Create the texture and color intrinsics:
```cpp
auto texture = create_synthetic_texture();
rs2_intrinsics color_intrinsics = { texture.x, texture.y,
        (float)texture.x / 2, (float)texture.y / 2,
        (float)texture.x / 2, (float)texture.y / 2,
        RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };
```

Then, we declare the Software-Only Device and add it two sensors:
```cpp
software_device dev; // Create software-only device

auto depth_s = dev.add_sensor("Depth"); // Define single sensor
auto color_s = dev.add_sensor("Color"); // Define single sensor
```

Now we add video stream to depth sensor:

```cpp
depth_s.add_video_stream({  RS2_STREAM_DEPTH, 0, 0,
                            W, H, 60, BPP_D,
                            RS2_FORMAT_Z16, depth_intrinsics });

```

Add the option RS2_OPTION_DEPTH_UNITS and set it a value for the usage of point cloud.
```cpp
depth_s.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);
```

Add video stream to color sensor

```cpp
color_s.add_video_stream({  RS2_STREAM_COLOR, 0, 1, texture.x,
		                        texture.y, 60, texture.bpp,
		                        RS2_FORMAT_RGBA8, color_intrinsics });
```

Set the type of the matcher that we want the syncer will use with:

```cpp
dev.create_matcher(DLR_C);
```

Register synthetic extrinsics from depth to color:
```cpp
depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });
```

Allocate memory for the synthetic depth:
```cpp
std::vector<uint8_t> pixels_depth(W * H * BPP_D, 0);
```
Now we start to loop until the window is closed,
in each itaration we fill synthetic depth data using sin and changes wave_base as function of time
```cpp
fill_synthetic_depth_data((void*)pixels_depth.data(), W , H , BPP_D, wave_base);
```

We update the yaw and wave_base each 1 MSec.
After we created the depth frame we injects it to the depth sensor and the color frame to the color sensor
```cpp
depth_s.on_video_frame({ pixels_depth.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            W*BPP_D, BPP_D, // Stride and Bytes-per-pixel
            (rs2_time_t)ind * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, ind, // Timestamp, Frame# for potential sync services
            depth_stream });


        color_s.on_video_frame({ texture.frame.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            texture.x*texture.bpp, texture.bpp, // Stride and Bytes-per-pixel
            (rs2_time_t)ind * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, ind, // Timestamp, Frame# for potential sync services
            color_stream });

```
Then we wait for the result of syncer:
```cpp
auto fset = sync.wait_for_frames();
```

Eventualy we are using the point_cloud object to calculate the points and the texture mapping,
and call to draw_pointcloud to render it.
```cpp
auto d = fset.first_or_default(RS2_STREAM_DEPTH);
auto c = fset.first_or_default(RS2_STREAM_COLOR);

if (d && c)
{
	if (d.is<depth_frame>())
	p = pc.calculate(d.as<depth_frame>());
	pc.map_to(c);

	// Upload the color frame to OpenGL
	app_state.tex.upload(c);
}
draw_pointcloud(app, app_state, p);
```

