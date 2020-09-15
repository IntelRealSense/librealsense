# rs-hdr Sample

## Overview

This sample demonstrates how to configure the camera for streaming and rendering Depth & Infrared data to the screen, using the High Dynamic Range (HDR) feature.  
For explanations on the streaming and rendering, please refer to the rs-capture sample.

## Code Overview 

### Finidng device
The HDR feature is available only for D400 Product line Devices. Finiding this device and getting its depth sensor:
```cpp
// finding a device of D400 product line for working with HDR feature
if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && 
            std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400")
{
    device = dev;
}
rs2::depth_sensor depth_sensor = device.query_sensors().front();
 ```


### HDR Configuration
Before starting the configuration, we need to disable the Auto Exposure option:
```cpp
// disable auto exposure before sending HDR configuration
if (depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
    depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
```

Next, we start to configure the HDR, by setting the size of the HDR sequence:
```cpp
// setting the HDR sequence size to 2 frames
depth_sensor.set_option(RS2_OPTION_SEQUENCE_SIZE, 2);
```
Choosing an ID for this HDR configuration. The configuration will not be saved in the firmware, but this ID can help users that configure another HDR configuration to know when the new HDR is streaming, by checking this value via the metadata:
```cpp
// configuring identifier for this hdr config (value must be in range [0,3])
depth_sensor.set_option(RS2_OPTION_SEQUENCE_NAME, 1);
```

Configuring the first HDR sequence ID: 
```cpp
// configuration for the first HDR sequence ID
depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
depth_sensor.set_option(RS2_OPTION_EXPOSURE, 8500.f);
depth_sensor.set_option(RS2_OPTION_GAIN, 16.f);
```
Configuring the second HDR sequence ID:
```cpp
// configuration for the second HDR sequence ID
depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
depth_sensor.set_option(RS2_OPTION_EXPOSURE, 150.f);
depth_sensor.set_option(RS2_OPTION_GAIN, 16.f);
```

Resetting the sequence ID to 0. This action permits the next setting of the exposure and gain option to be targetted to the normal UVC option, instead of being targetted to the HDR configuration (this call could have been omitted in this specific example):
```cpp
// after setting the HDR sequence ID option to 0, setting exposure or gain
// will be targetted to the normal (UVC) exposure and gain options (not HDR configuration)
depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 0);
```

Activating the HDR configuration in order to get the resulting streaming:
```cpp
// turning ON the HDR with the above configuration 
depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
```

Then, the pipe is configured with depth and infrared streams. 
In HDR mode the Infrared stream is used as an auxiliary invalidation filter to handle outlier Depth pixels and therefore, to enhance the outcome. The merging algorithm can also work without the infrared stream (if it is not activated by the user), but it workds better with the infrared.
```cpp
// Start streaming with depth and infrared configuration
// The HDR merging algorithm can work with both depth and infrared,or only with depth, 
// but the resulting stream is better when both depth and infrared are used.
rs2::config cfg;
cfg.enable_stream(RS2_STREAM_DEPTH);
cfg.enable_stream(RS2_STREAM_INFRARED, 1);
pipe.start(cfg);
```

### Using Merging Filter
Initializing the merging filter - will be further used in order to merge frames from both HDR sequence IDs that have been configured:
```cpp
// initializing the merging filter
rs2::depth_merge merging_filter;
```
After getting the frames, by using the wait_for_frames method, the merging filter is used:
```cpp
// merging the frames from the different HDR sequence IDs
auto merged_frameset = merging_filter.process(data). 
        apply_filter(color_map);   // Find and colorize the depth data;
app.show(merged_frameset);

```

### Using Spliting Filter 
Initializing also the spliting filter, with the requested sequence ID as 2:
```cpp
// initializing the spliting filter
rs2::depth_split spliting_filter;
// setting the required sequence ID to be shown
spliting_filter.set_option(RS2_OPTION_SELECT_ID, 2);
```

After getting the frames, by using the wait_for_frames method, the spliting filter is used:
```cpp
// getting frames only with the requested sequence ID
auto split_frameset = spliting_filter.process(data). 
apply_filter(color_map);   // Find and colorize the depth data;
app.show(split_frameset);

```

