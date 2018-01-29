# RS400 Advanced Mode

## Overview
RS400 is the newest in the series of stereo-based RealSense devices. It is provided in a number distinct models, all sharing the same depth-from-stereo ASIC. The models differ in optics, global vs. rolling shutter, and the projector capabilities. In addition, different use-cases may introduce different lighting conditions, aim at different range, etc...
As a result, depth-from-stereo algorithm in the hardware has to be able to adjust to vastly different stereo input images.

To achieve this goal, RS400 ASIC provides an extended set of controls aimed at advanced users, allowing to fine-tune the depth generation process. (and some other like **color-correction**)

Advanced mode hardware APIs are designed to be **safe**, in the sense that the user can't brick the device. You always have the option to exit advanced mode, and fall back to default mode of operation.
However, while tinkering with the advanced controls, depth quality and stream frame-rate are not guarantied. You are making changes on your own risk.

## Long-term vs. Short-term
* In the short-term, librealsense provides a set of advanced APIs you can use to access advanced mode. Since, this includes as much as 100 new parameters we also provide a way to serialize and load these parameters, using JSON file structure. We encourage the community to explore the range of possibilities and share interesting presets for different use-cases. The limitation of this approach is that the existing protocol is cumbersome and not efficient. We can't provide clear estimate on how much time it will take until any set of controls will take effect, and the controls are not standard in any way in the OS.
* In the long-term, Intel algorithm developers will come up with best set of recommended presets, as well as most popular presets from our customers, and these will be hard-coded into the camera firmware.
This will provide fast, reliable way to optimize the device for a specific use case.

## Programming Interface
 Here are some examples of how to use the advanced-mode API via C and C++.<br>
 
 ### Advanced Mode C API
 The following code sample checking if the connected device is in advanced-mode and moving it to advance-mode if it's not.
```c
/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/rs_advanced_mode.h>

#include <stdio.h> // printf
#include <stdlib.h> // exit

/* Function calls to librealsense may raise errors of type rs_error*/
void check_error(rs2_error* e)
{
    if (e)
    {
        printf("rs_error was raised when calling %s(%s):\n", rs2_get_failed_function(e), rs2_get_failed_args(e));
        printf("    %s\n", rs2_get_error_message(e));
        exit(1);
    }
}

int main()
{
    rs2_error* e = 0;

    // Create a context object. This object owns the handles to all connected realsense devices.
    // The returned object should be released with rs2_delete_context(...)
    rs2_context* ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error(e);

    /* Get a list of all the connected devices. */
    // The returned object should be released with rs2_delete_device_list(...)
    rs2_device_list* device_list = rs2_query_devices(ctx, &e);
    check_error(e);

    int dev_count = rs2_get_device_count(device_list, &e);
    check_error(e);
    printf("There are %d connected RealSense devices.\n", dev_count);
    if (0 == dev_count)
        return 1;

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    rs2_device* dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    // Check if Advanced-Mode is enabled
    int is_advanced_mode_enabled;
    rs2_is_enabled(dev, &is_advanced_mode_enabled, &e);
    check_error(e);

    if (!is_advanced_mode_enabled)
    {
        // Enable advanced-mode
        rs2_toggle_advanced_mode(dev, 1, &e);
        check_error(e);
    }

    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);
}
```

The following code sample showing how to serialize the current values of advanced mode controls to a JSON file,
and also how to load and apply it to the device.
```c
/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/rs_advanced_mode.h>

#include <stdio.h> // printf
#include <stdlib.h> // exit

/* Function calls to librealsense may raise errors of type rs_error*/
void check_error(rs2_error* e)
{
    if (e)
    {
        printf("rs_error was raised when calling %s(%s):\n", rs2_get_failed_function(e), rs2_get_failed_args(e));
        printf("    %s\n", rs2_get_error_message(e));
        exit(1);
    }
}

int main()
{
    rs2_error* e = 0;

    // Create a context object. This object owns the handles to all connected realsense devices.
    // The returned object should be released with rs2_delete_context(...)
    rs2_context* ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error(e);

    /* Get a list of all the connected devices. */
    // The returned object should be released with rs2_delete_device_list(...)
    rs2_device_list* device_list = rs2_query_devices(ctx, &e);
    check_error(e);

    int dev_count = rs2_get_device_count(device_list, &e);
    check_error(e);
    printf("There are %d connected RealSense devices.\n", dev_count);
    if (0 == dev_count)
        return 1;

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    rs2_device* dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    // Serialize current values of advanced mode controls to a JSON content
    rs2_raw_data_buffer* raw_data_buffer = rs2_serialize_json(dev, &e);
    check_error(e);

    // Get raw data size
    int raw_data_size = rs2_get_raw_data_size(raw_data_buffer, &e);
    check_error(e);

    // Get the pointer of the raw data
    const unsigned char* raw_data =  rs2_get_raw_data(raw_data_buffer, &e);
    check_error(e);

    // Write the raw_data to a JSON file
    const char* full_file_path = "./advanced_mode_controls.json";
    FILE *f = fopen(full_file_path, "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    fwrite(raw_data, sizeof(unsigned char), raw_data_size, f);
    fclose(f);


    // Load the JSON raw data and apply the control values
    rs2_load_json(dev, raw_data, raw_data_size, &e);
    check_error(e);

    rs2_delete_raw_data(raw_data_buffer);
    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);
}
```

The following code sample showing how to retrieve the advanced-mode control values. 
```c
/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/rs_advanced_mode.h>

#include <stdio.h> // printf
#include <stdlib.h> // exit

/* Function calls to librealsense may raise errors of type rs_error*/
void check_error(rs2_error* e)
{
    if (e)
    {
        printf("rs_error was raised when calling %s(%s):\n", rs2_get_failed_function(e), rs2_get_failed_args(e));
        printf("    %s\n", rs2_get_error_message(e));
        exit(1);
    }
}

int main()
{
    rs2_error* e = 0;

    // Create a context object. This object owns the handles to all connected realsense devices.
    // The returned object should be released with rs2_delete_context(...)
    rs2_context* ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error(e);

    /* Get a list of all the connected devices. */
    // The returned object should be released with rs2_delete_device_list(...)
    rs2_device_list* device_list = rs2_query_devices(ctx, &e);
    check_error(e);

    int dev_count = rs2_get_device_count(device_list, &e);
    check_error(e);
    printf("There are %d connected RealSense devices.\n", dev_count);
    if (0 == dev_count)
        return 1;

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    rs2_device* dev = rs2_create_device(device_list, 0, &e);
    check_error(e);

    const int curr_val_mode = 0; // 0 - Get current values mode
    STDepthControlGroup curr_depth_control_group;
    // Get current values of depth control group
    rs2_get_depth_control(dev, &curr_depth_control_group, curr_val_mode, &e);
    check_error(e);

    const int min_val_mode = 1; // 1 - Get minimum values mode
    STDepthControlGroup min_depth_control_group;
    // Get minimum values of depth control group
    rs2_get_depth_control(dev, &min_depth_control_group, min_val_mode, &e);
    check_error(e);

    const int max_val_mode = 2; // 2 - Get maximum values mode
    STDepthControlGroup max_depth_control_group;
    // Get maximum values of depth control group
    rs2_get_depth_control(dev, &max_depth_control_group, max_val_mode, &e);
    check_error(e);


    // Set the maximum values to depth control group
    rs2_set_depth_control(dev, &max_depth_control_group, &e);
    check_error(e);


    rs2_delete_device(dev);
    rs2_delete_device_list(device_list);
    rs2_delete_context(ctx);

    return 0;
}
```

### Advanced Mode C++ API
The following sample code showing how to enable advanced-mode in a connected D400 device.
```cpp
/* Include the librealsense CPP header files */
#include <librealsense2/rs.hpp>
#include <librealsense2/rs_advanced_mode.hpp>

#include <iostream>

using namespace std;
using namespace rs2;

int main(int argc, char** argv) try
{
    // Obtain a list of devices currently present on the system
    context ctx;
    auto devices = ctx.query_devices();
    size_t device_count = devices.size();
    if (!device_count)
    {
        cout <<"No device detected. Is it plugged in?\n";
        return EXIT_SUCCESS;
    }

    // Get the first connected device
    auto dev = devices[0];

    if (dev.is<rs400::advanced_mode>())
    {
        auto advanced_mode_dev = dev.as<rs400::advanced_mode>();
        // Check if advanced-mode is enabled
        if (!advanced_mode_dev.is_enabled())
        {
            // Enable advanced-mode
            advanced_mode_dev.toggle_advanced_mode(true);
        }
    }
    else
    {
        cout << "Current device doesn't support advanced-mode!\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
catch (const exception & e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
```

The following code sample showing how to retrieve the advanced-mode control values. 
```cpp
/* Include the librealsense CPP header files */
#include <librealsense2/rs.hpp>
#include <librealsense2/rs_advanced_mode.hpp>

#include <iostream>

using namespace std;
using namespace rs2;


int main(int argc, char** argv) try
{
    // Obtain a list of devices currently present on the system
    context ctx;
    auto devices = ctx.query_devices();
    size_t device_count = devices.size();
    if (!device_count)
    {
        cout <<"No device detected. Is it plugged in?\n";
        return EXIT_SUCCESS;
    }

    // Get the first connected device
    auto dev = devices[0];

    // Check if current device supports advanced mode
    if (dev.is<rs400::advanced_mode>())
    {
        // Get the advanced mode functionality
        auto advanced_mode_dev = dev.as<rs400::advanced_mode>();
        const int max_val_mode = 2; // 0 - Get maximum values
        // Get the maximum values of depth controls
        auto depth_control_group = advanced_mode_dev.get_depth_control(max_val_mode);

        // Apply the depth controls maximum values
        advanced_mode_dev.set_depth_control(depth_control_group);
    }
    else
    {
        cout << "Current device doesn't support advanced-mode!\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
catch (const exception & e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
```
*Note: It is recommended to set advanced mode controls when not streaming.*
