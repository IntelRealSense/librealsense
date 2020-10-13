# D400 Advanced Mode

## Overview
RS400 is the newest in the series of stereo-based RealSense devices. It is provided in a number distinct models, all sharing the same depth-from-stereo ASIC. The models differ in optics, global vs. rolling shutter, and the projector capabilities. In addition, different use-cases may introduce different lighting conditions, aim at different range, etc...
As a result, depth-from-stereo algorithm in the hardware has to be able to adjust to vastly different stereo input images.
To achieve this goal, RS400 ASIC provides an extended set of controls aimed at advanced users, allowing to fine-tune the depth generation process. (and some other like **color-correction**)
Advanced mode hardware APIs are designed to be **safe**, in the sense that the user can't brick the device. You always have the option to exit advanced mode, and fall back to default mode of operation.
However, while tinkering with the advanced controls, depth quality and stream frame-rate are not guaranteed. You are making changes on your own risk.

## Long-term vs. Short-term
* In the short-term, librealsense provides a set of advanced APIs you can use to access advanced mode. Since, this includes as much as 100 new parameters we also provide a way to serialize and load these parameters, using JSON file structure. We encourage the community to explore the range of possibilities and share interesting presets for different use-cases. The limitation of this approach is that the existing protocol is cumbersome and not efficient. We can't provide clear estimate on how much time it will take until any set of controls will take effect, and the controls are not standard in any way in the OS.
* In the long-term, Intel algorithm developers will come up with best set of recommended presets, as well as most popular presets from our customers, and these will be hard-coded into the camera firmware.
This will provide fast, reliable way to optimize the device for a specific use case.

## Building Advanced Mode APIs
To build advanced mode APIs in addition to librealsense, you need to configure `cmake` with the following additional parameter: `-DBUILD_RS400_EXTRAS=true`. For example:
`cmake .. -DBUILD_EXAMPLES=true -DBUILD_RS400_EXTRAS=true`

You will need one of the following compilers (or newer) for JSON support:
* GCC 4.9
* Clang 3.4
* Microsoft Visual C++ 2015 / Build Tools 14.0.25123.0
(based on [github.com/nlohmann/json](https://github.com/nlohmann/json))

Once the library is built, navigate to the output folder (./build/rs400/examples from librealsense folder on Linux) and run ``rs400-advanced-mode-sample``:
![RS400 Advanced Mode Sample](advanced_mode_sample.png)
This application allows you to adjust various controls live and load existing presets (you can drag & drop JSON file into the application)

## Programming interface
At the moment, RS400 advanced mode produces single library `rs400-advanced-mode` (.so / .dll) dependent on librealsense.
This library provides C wrappers for the various controls:
```c
#include <rs400_advanced_mode/rs400_advanced_mode.h>

// Create one of the low level control groups:
STDepthControlGroup depth_control;

printf("Reading deepSeaMedianThreshold from Depth Control...\n");
// Query current values from the device:
int result = get_depth_control(dev, &depth_control);
if (result)
{
    printf("Advanced mode get failed!\n");
    return EXIT_FAILURE;
}
printf("deepSeaMedianThreshold = %d\n", depth_control.deepSeaMedianThreshold);

printf("Writing Depth Control back to the device...\n");
// Write new values to the device:
result = set_depth_control(dev, &depth_control);
if (result)
{
    printf("Advanced mode set failed!\n");
    return EXIT_FAILURE;
}
```
(see [./rs400/examples/c-sample.c](https://github.com/IntelRealSense/librealsense/blob/development/rs400/examples/c-sample.c))

In addition, you can use advanced mode functionality in your C++ application without linking with any additional dependencies by just including `rs400_advanced_mode/rs400_advanced_mode.hpp` (under `/rs400/include`):
```cpp
#include <rs400_advanced_mode/rs400_advanced_mode.hpp>

// Define a lambda to tie the advanced mode to an existing realsense device (dev)
auto send_receive = [&dev](const std::vector<uint8_t>& input)
{
    return dev->debug().send_and_receive_raw_data(input);
};
// Create advanced mode abstraction on to of that
rs400::advanced_mode advanced(send_receive);

// Create one of the low level control groups
rs400::STDepthControlGroup depth_control;
advanced.get(&depth_control); // Query current values
std::cout << "deepSeaMedianThreshold: " << depth_control.deepSeaMedianThreshold << std::endl;
```
It is recommended to set advanced mode controls when not streaming. 
