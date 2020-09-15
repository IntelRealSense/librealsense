# rs-reproject Sample

## Overview
 This example introduces the concept of spatial stream reprojection.
 The need for spatial reprojection (from here "reproject") arises from the fact
 that sometimes there is a need to transform the depth streams to different viewport.
 Reproject process lets the user translate and rotate images to another viewport using spatial information.
 That said, results of reproject are synthetic stream, and suffer from several artifacts:
1. **Sampling** - mapping stream to a different viewport will modify the resolution of the frame
               to match the resolution of target viewport. This will either cause downsampling or
               upsampling via interpolation. The interpolation used needs to be of type
               Nearest Neighbor to avoid introducing non-existing values.
2. **Occlussion** - Some pixels in the resulting image correspond to 3D coordinates that the original
               viewport did not see, because these 3D points were occluded in the original viewport.
               Such pixels may hold invalid texture values.
			  
## Expected Output

The application should open a window and display video stream from the depth camera.
The sliders in the "debug" drawer control the translation and rotation of the new perspective to be reprojected.

## Code Overview

This example is using standard `librealsense` API and `IMGUI` library for simple UI rendering:
```cpp
#include <librealsense2/rs.hpp>
#include "../example.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
```

In this example we are interested in `RS2_STREAM_DEPTH` stream:
```cpp
// Create a pipeline to easily configure and start the camera
rs2::pipeline pipe;
rs2::config cfg;
cfg.enable_stream(RS2_STREAM_DEPTH);
pipe.start(cfg);

```

SDK class responsible for stream reprojection is called `rs2::reproject`. The user initializes it with desired intrinsics and extrinsics of new viewport and applies it to depth frames via `process` method.
```cpp
rs2::reproject reproject_depth(intr, extr);
// ...
auto depth = reproject_depth.process(frameset.first_or_default(RS2_STREAM_DEPTH));
```

Next, we render the resulting depth stream:

```cpp
depth_image.render(colorized_depth, { 0, 0, app.width(), app.height() });
```

IMGUI is used to render the sliders and reset button.