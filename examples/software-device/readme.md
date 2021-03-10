# Software Device Sample

## Overview

This sample demonstrates usage of the `software_device` object, which allows users to create and control custom SDK device not dependent on Intel RealSense hardware. This allows for comparison of other hardware with Intel RealSense.

`software_device` can be used to generate frames from synthetic or external sources and pass them into SDK processing functionality: 
* Record and playback services
* Post-processing filters
* Spatial alignment of streams
* Temporal synchronizion of streams
* Point-cloud generation

In this example we will show how to:
* Create synthetic depth and texture frames
* Synchronize them using the `syncer` class
* Generate a 3D-model using the `pointcloud` class

## Expected Output

The application will open a window and display a 3D model of synthetic depth we created with synthetic texture.

<p align="center"><img src="https://user-images.githubusercontent.com/22448952/34941693-8195b372-f9fd-11e7-9ca1-3e39aef3ce98.png" alt="screenshot png"/></p>


## Code Overview

We declare two helper functions abstracting synthetic data generation (instead you can load data from disk, from other type of camera or any other source):
```cpp
synthetic_frame create_synthetic_texture();
void fill_synthetic_depth_data(void* data, int w, int h, int bpp, float wave_base)
```
For our example: 
* `create_synthetic_texture()` - loads a static image in PNG format encoded as embedded byte array.
* `fill_synthetic_depth_data(..)` - generates synthetic depth frame in a shape of a wave. Offset of the wave is controlled using `wave_base` parameter that we update to generate the animation effect. 

Heading to `main`:

We will use hard-coded dimentions for the depth frame:
```cpp
const int W = 640;
const int H = 480;
const int BPP_D = 2;
```

In order to generate objects of type `rs2::frame` we will declare a software device `dev`. This object will function as a converter from raw data into SDK objects and will allow us to pass our synthetic images into SDK algorithms. 
```cpp
software_device dev; // Create software-only device
// Define two sensors, one for depth and one for color streams
auto depth_s = dev.add_sensor("Depth");
auto color_s = dev.add_sensor("Color");
```

Before we can pass images into the device, we must provide details about the stream we are going to simulate. This include stream type, dimentions, format and stream intrinsics. 
In order to properly simulate depth data, we must define legal intrinsics that will be used to project pixels into 3D space.

```cpp
rs2_intrinsics depth_intrinsics{ W, H, W / 2, H / 2, W , H , RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

depth_s.add_video_stream({  RS2_STREAM_DEPTH, 0, 0,
                            W, H, 60, BPP_D,
                            RS2_FORMAT_Z16, depth_intrinsics });

```

We will do the same for the color stream:

```cpp
auto texture = create_synthetic_texture();
rs2_intrinsics color_intrinsics = { texture.x, texture.y,
        (float)texture.x / 2, (float)texture.y / 2,
        (float)texture.x / 2, (float)texture.y / 2,
        RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

color_s.add_video_stream({  RS2_STREAM_COLOR, 0, 1, texture.x,
		                        texture.y, 60, texture.bpp,
		                        RS2_FORMAT_RGBA8, color_intrinsics });
```

You can add sensor options using `add_read_only_option` method. In this example, we will simulate `RS2_OPTION_DEPTH_UNITS` option required for point-cloud generation:

```cpp
depth_s.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);
```

In order to use `syncer` class with the synthetic streams, we must specify the synchronization model for our software device. For this example we will borrow the synchronization model from the D435 camera:
```cpp
dev.create_matcher(DLR_C);
```
The last thing we need to provide in order to use `pointcloud` with our synthetic data is the extrinsic calibration between the two sensors. In this example, we will define the extrinsics to be identity (sensors share the same 3D location): 
```cpp
depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });
```
We will update `wave_base` every 1 millisecond and re-generate the depth data:

```cpp
fill_synthetic_depth_data((void*)pixels_depth.data(), W , H , BPP_D, wave_base);
```

After we created the depth frame we inject it into the depth sensor:
```cpp
depth_s.on_video_frame({ pixels_depth.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            W*BPP_D, BPP_D, // Stride and Bytes-per-pixel
            (rs2_time_t)ind * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, ind, // Timestamp, Frame# for potential sync services
            depth_stream });
```
We do the same for color sensor: 
```cpp
color_s.on_video_frame({ texture.frame.data(), // Frame pixels from capture API
            [](void*) {}, // Custom deleter (if required)
            texture.x*texture.bpp, texture.bpp, // Stride and Bytes-per-pixel
            (rs2_time_t)ind * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, ind, // Timestamp, Frame# for potential sync services
            color_stream });

```
Now we can wait for synchronized pairs from the `syncer`:
```cpp
auto fset = sync.wait_for_frames();
```
Next, we will invoke the `pointcloud` processing block to generate 3D-model and texture coordinates, similar to the [rs-pointcloud](../pointcloud) example:
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

Putting everything together you will get an interactive textured point-cloud visualization using SDK algorithms. 
