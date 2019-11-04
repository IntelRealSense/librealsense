# rs-tracking-mask Sample

> In order to run this example, a device supporting pose stream (T265) is required.

## Overview

This sample demonstrates how to "mask" out a portion of the fisheye image on T265 to prevent the tracking from using the full fisheye frame. This is only recommended if you know that some part of your fisheye frame will be occluded. The API requires the user to pass a timestamp in addition to the mask image. If this timestamp is 0 or in the past, the mask will be used immediately. If the timestamp is set to a future time, the mask will not take effect until this time. Querying `get_tracking_mask` will only return the mask once it is being used.

## Expected Output
The application should open a window in which it shows one of the fisheye streams and displays only the center portion of the frame (where the mask has been set to 1).

## Code Overview

First, we include the Intel® RealSense™ Cross-Platform API.
```cpp
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rsutil.h>
```

Then, in the `main` function, we declare the pipeline and configure it with `RS2_STREAM_FISHEYE` streams. Then, we start the pipeline.
```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;
// Create a configuration for configuring the pipeline with a non default profile
rs2::config cfg;
// Enable fisheye streams
cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
cfg.enable_stream(RS2_STREAM_FISHEYE, 2);
// Start pipeline with chosen configuration
rs2::pipeline_profile pipe_profile = pipe.start(cfg);
```

T265 provides two fisheye sensors we can use. We choose index 1 (left sensor), but if you want to set a mask for index 1, it's likely you will also want to set a slightly different mask for index 2 to exclude different pieces of that image.

```cpp
// T265 has two fisheye sensors, we can choose any of them (index 1 or 2)
const int fisheye_sensor_idx = 1;
```

Tracking masks are defined to be 8 times smaller than the fisheye image, so to get the correct size we need the sensor intrinsics.

```cpp
// Get fisheye sensor intrinsics parameters
rs2::stream_profile fisheye_stream = pipe_profile.get_stream(RS2_STREAM_FISHEYE, fisheye_sensor_idx);
rs2_intrinsics intrinsics = fisheye_stream.as<rs2::video_stream_profile>().get_intrinsics();
```

We create an OpenGL window where we will show the masked fisheye image.
```cpp
// Create an OpenGL display window and a texture to draw the fisheye image
window app(intrinsics.width, intrinsics.height, "Intel RealSense T265 Tracking Mask Example");
window_key_listener key_watcher(app);
texture fisheye_image;
```

After setting up the device, we run the main loop and iterate to get images
from the streams.
```cpp
// Main loop
while (app)
{
```

We wait until new synchronized fisheye frames are available.
```cpp
    {
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();
        // Get a frame from the fisheye stream
        rs2::video_frame fisheye_frame = frames.get_fisheye_frame(fisheye_sensor_idx);
```

If the mask has not yet been created, we create it and set it:

```cpp
            if(!mask_set) {
                int width = intrinsics.width/8;
                int height = intrinsics.height/8;
                auto mask_ptr = std::unique_ptr<uint8_t []>(new uint8_t[width*height]);
                double global_ts_ms = fisheye_frame.get_timestamp();
                // By default, mask out everything
                memset(mask_ptr.get(), 0, width*height);
                // Allow tracking on a 60 wide by 20 high region of
                // the image. Note that these are in mask coordinates,
                // so they correspond to a 60*8 = 480 wide by 20*8 =
                // 160 high region of the fisheye image.
                for(int y = 30; y < 50; y++) {
                    for(int x = 40; x < 100; x++) {
                        mask_ptr[y*width + x] = 255;
                    }
                }
                tm2.set_tracking_mask(fisheye_sensor_idx, mask_ptr.get(), width, height, global_ts_ms);
                mask_set = true;
            }
```

Then we attempt to retrieve the currently active mask:

```cpp
            int width, height;
            double global_ts_ms;
            rs2_raw_data_buffer * mask_buffer = tm2.get_tracking_mask(fisheye_sensor_idx, &mask, &width, &height, &global_ts_ms);
```

If a mask has not been set, `get_tracking_mask` will return `nullptr`, so we check for that before rendering a visualization of the masked region:

```cpp
            if(mask_buffer) {
                // Render the fisheye image with a mask
                auto image = render_mask_image(fisheye_frame, mask_buffer);
                fisheye_image.render(image.get(), RS2_FORMAT_Y8, fisheye_frame.get_width(), fisheye_frame.get_height(), {0, 0, app.width(), app.height()}, 1);
                rs2_delete_raw_data(mask_buffer);
            }
```

If there is a valid mask, `mask_buffer` will point to a `rs2_raw_data_buffer` which contains the mask (an array of `uint8_t`). Once we are done with this data, it needs to be deallocated (using `rs2_delete_raw_data` above). We can retrieve a raw pointer to the mask data using `rs2_get_raw_data`, and then proceed to mask the fisheye frame:

```cpp
std::unique_ptr<uint8_t[]> render_mask_image(rs2::video_frame & frame, rs2_raw_data_buffer * mask_buffer)
{
    rs2_error * e = nullptr;
    uint8_t * mask = (uint8_t *) rs2_get_raw_data(mask_buffer, &e);
    rs2::error::handle(e);

    uint8_t * frame_data = (uint8_t *) frame.get_data();
    int width = frame.get_width();
    int height = frame.get_height();
    int mask_width = width / 8;
    std::unique_ptr<uint8_t []> output(new uint8_t[width*height]);
    // Iterate over each pixel in the image and check the mask to see
    // if the pixel is visible to the tracking algorithm or not.
    for(int y = 0; y < height; y++) {
        int my = y / 8;
        for(int x = 0; x < width; x++) {
            int mx = x / 8;
            if(mask[my*mask_width + mx])
                output[y * width + x] = frame_data[y * width + x];
            else
                output[y * width + x] = 0;
        }
    }
    return output;
}
```

If the mask is not yet valid, we display the full fisheye frame:

```cpp
            else {
                // No mask has been set, or the mask we have set has
                // not yet taken effect. Render the full frame.
                fisheye_image.render(fisheye_frame.get_data(), RS2_FORMAT_Y8, fisheye_frame.get_width(), fisheye_frame.get_height(), {0, 0, app.width(), app.height()}, 1);
            }
```
