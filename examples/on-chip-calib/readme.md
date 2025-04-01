# rs-on-chip-calib example

## Overview

This example demonstrates the basic flow of performing on chip calibration for a RealSense device.
The principal is similar when calling other internal calibrations like Tare or focal-length calibrations. On chip calibration has the advantage of not needing a dedicated target.
For more information see [self-calibration white paper](https://dev.intelrealsense.com/docs/self-calibration-for-depth-cameras).

## Expected Output
Assuming camera is connected and valid depth image can be obtained (usually more than 50% valid depth points), "Completed successfully" output is expected.
User will be promped to save calibration results to flash. "Results saved to flash" is expected in case user chooses to save, "Results not saved" otherwise.

## Code Overview 

First, we include the Intel® RealSense™ Cross-Platform API.  
All but advanced functionality is provided through a single header:
```cpp
#include <librealsense2/rs.hpp> // RealSense Cross Platform API
```

Settings are supported, e.g. DDS domain for Ethernet based models. We process them and pass to the pipe.
```cpp
auto settings = rs2::cli( "on-chip-calib example" ).process( argc, argv );

// Create a pipeline - this serves as a top-level API for streaming and processing frames
rs2::pipeline pipe( settings.dump() );
```

Next we configure the desired streams. For calibration a special resolution is used.
We start the pipe and wait for streaming to start.
```cpp
// Configure and start the pipeline
rs2::config cfg;
cfg.enable_stream( RS2_STREAM_DEPTH, 256, 144, RS2_FORMAT_Z16, 90 );
rs2::pipeline_profile prof = pipe.start( cfg );
pipe.wait_for_frames(); // Wait for streaming to start, will throw on timeout
```
Once pipeline is configured, we can start the calibration.
On chip calibration receive run parameters in a JSON format. Most parameters are for internal development and image quality testing purposes. All parameters have defaults and doesn't have to be set by users.
A parameter that users might want to tweek is "speed", affecting the calibration performance, but also the time to perform it.
```cpp
// Control calibration parameters using a JSON input
//   Currently for OCC only type 0 is used.
//   Speed can be 0 - very fast, 1 - fast, 2 - medium, 3 - slow (default). Timeout should be adjusted accordingly.
//   Scan parameter 0 - intrinsic (default), 1 - extrinsic.
std::stringstream ss;
ss << "{"
   << "\n \"calib type\":" << 0
   << ",\n \"speed\":" << 2
   << ",\n \"scan parameter\":" << 0
   << "\n}";
std::string json = ss.str();
std::cout << "Starting OCC with configuration:\n" << json << std::endl;
```

rs2::auto_calibrated_device is the API object for performing calibrations. In this case with the `run_on_chip_calibration` method.
```cpp
// Get device with calibration API
rs2::device dev = prof.get_device();
rs2::auto_calibrated_device cal_dev = dev.as< rs2::auto_calibrated_device >();

// Run calibration
float health;
int timeout_ms = 9000;
rs2::calibration_table res = cal_dev.run_on_chip_calibration( json, &health, [&]( const float progress ) {
    std::cout << "progress = " << progress << "%" << std::endl;
}, timeout_ms );
```

After the calibration process, the device is using calibrated parameters. However the result is not saved and will be lost on reset or power loss.
User may save the new calibration table to flash, so the device will use it in the future.
```cpp
cal_dev.set_calibration_table( res );
std::cout << "Results saved to flash" << std::endl;
```
To restore previous calibration parameters, user can `get_calibration_table` before the calibration and save that to the flash.
Alternativly, user can reset the device, making it re-read the old calibration table at start up.
