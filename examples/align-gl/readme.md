# rs-align-gl Sample

## Overview

> Due to driver limitations built-in GPU capabilities are not available at the moment on Mac-OS

This example demonstrates the concept of spatial stream alignment **accelerated with GPU**.
For example usecase of alignment, please check out align-advanced and measure demos.
The need for spatial alignment (from here "align") arises from the fact
that not all camera streams are captured from a single viewport.


Align process lets the user translate images from one viewport to another. 
That said, results of align are synthetic streams, and suffer from several artifacts:
1. **Sampling** - mapping stream to a different viewport will modify the resolution of the frame 
              to match the resolution of target viewport. This will either cause downsampling or
              upsampling via interpolation. The interpolation used needs to be of type
              Nearest Neighbor to avoid introducing non-existing values.
2. **Occlussion** - Some pixels in the resulting image correspond to 3D coordinates that the original
              sensor did not see, because these 3D points were occluded in the original viewport.
              Such pixels may hold invalid texture values.
			  
## Expected Output

The application should open a window and display video stream from the color camera overlayed on top of depth stream data.
The slider in bottom of the window control the transparancy of the overlayed stream.
Checkboxes below allow toggling between depth to color vs color to depth alignment.

<p align="center"><img src="https://raw.githubusercontent.com/wiki/dorodnic/librealsense/align-expected.gif" alt="screenshot gif"/></p>

## Code Overview

In addition to core `realsense2` this example also depends on an auxiliary `realsense2-gl` library. 
This is not strictly part of core RealSense functionality, but rather a useful extension.

So, in addition to standard `#include <librealsense2/rs.hpp>` you need to include:
```cpp
#include <librealsense2-gl/rs_processing_gl.hpp> // Include GPU-Processing API
```

This example also uses `IMGUI` library for simple UI rendering:

> In order to allow texture sharing between processing and rendering application, `rs_processing_gl.hpp` needs to be included **after** including `GLFW`.

After defining some helper functions and `window` object, we need to initialize GL processing and rendering subsystems:
```cpp
// Once we have a window, initialize GL module
// Pass our window to enable sharing of textures between processed frames and the window
rs2::gl::init_processing(app, use_gpu_processing);
// Initialize rendering module:
rs2::gl::init_rendering();
```

In this example we are interested in `RS2_STREAM_DEPTH` and `RS2_STREAM_COLOR` streams:
```cpp
// Create a pipeline to easily configure and start the camera
rs2::pipeline pipe;
rs2::config cfg;
cfg.enable_stream(RS2_STREAM_DEPTH);
cfg.enable_stream(RS2_STREAM_COLOR);
pipe.start(cfg);

```

SDK class responsible for GL stream alignment is called `rs2::gl::align`. The user should initialize it with desired target stream and applies it to framesets via `process` method.
```cpp
// Define two align objects. One will be used to align
// to depth viewport and the other to color.
// Creating align object is an expensive operation
// that should not be performed in the main loop
rs2::gl::align align_to_depth(RS2_STREAM_DEPTH);
rs2::gl::align align_to_color(RS2_STREAM_COLOR);
// ...
frameset = align_to_depth.process(frameset);
```

This example also uses GL colorizer `rs2::gl::colorizer` to colorize the depth image.
```cpp
rs2::gl::colorizer  c;
```

Next, we render the two stream overlayed on top of each other using OpenGL blending feature:

```cpp
glEnable(GL_BLEND);
// Use the Alpha channel for blending
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

if (dir == direction::to_depth)
{
	// When aligning to depth, first render depth image
	// and then overlay color on top with transparancy
	depth_image.render(colorized_depth, { 0, 0, app.width(), app.height() });
	color_image.render(color, { 0, 0, app.width(), app.height() }, alpha);
}
else
{
	// When aligning to color, first render color image
	// and then overlay depth image on top
	color_image.render(color, { 0, 0, app.width(), app.height() });
	depth_image.render(colorized_depth, { 0, 0, app.width(), app.height() }, 1 - alpha);
}

glColor4f(1.f, 1.f, 1.f, 1.f);
glDisable(GL_BLEND);
```

IMGUI is used to render the slider and two checkboxes:
```cpp
// Render the UI:
ImGui_ImplGlfw_NewFrame(1);
render_slider({ 15.f, app.height() - 60, app.width() - 30, app.height() }, &alpha, &dir);
ImGui::Render();
```