# rs-callback Sample

## Overview

This sample demonstrates how to configure the camera for streaming frames using the pipeline's callback API. 

## Expected Output
![rs-callbak](https://user-images.githubusercontent.com/18511514/48921401-37a0c680-eea8-11e8-9ab4-18e566d69a8a.PNG)

## Code Overview 

First, we include the Intel® RealSense™ Cross-Platform API.  
All but advanced functionality is provided through a single header:
```cpp
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
```

Define frame counters
```cpp
std::map<int, int> counters;
std::map<int, std::string> stream_names;
std::mutex mutex;
```

Define the frame callback which will be invoked by the pipeline on the sensors thread.
```cpp
// Define frame callback
// The callback is executed on a sensor thread and can be called simultaneously from multiple sensors
// Therefore any modification to common memory should be done under lock
auto callback = [&](const rs2::frame& frame)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (rs2::frameset fs = frame.as<rs2::frameset>())
    {
        // With callbacks, all synchronized stream will arrive in a single frameset
        for (rs2::frame& f : fs)
            counters[f.get_profile().unique_id()]++;
    }
    else
    {
        // Stream that bypass synchronization (such as IMU) will produce single frames
        counters[frame.get_profile().unique_id()]++;
    }
};
```

The SDK API entry point is the `pipeline` class:
```cpp
// Declare the RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;

// Start streaming through the callback with default recommended configuration
// The default video configuration contains Depth and Color streams
// If a device is capable to stream IMU data, both Gyro and Accelerometer are enabled by default 
rs2::pipeline_profile profiles = pipe.start(callback);
```

Collect the stream names from the returned pipeline_profile.
```cpp
// Collect the enabled streams names
for (auto p : profiles.get_streams())
    stream_names[p.unique_id()] = p.stream_name();
```

Periodically print the frame counters.
```cpp
std::cout << "RealSense callback sample" << std::endl << std::endl;

while (true)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::lock_guard<std::mutex> lock(mutex);

    std::cout << "\r";
    for (auto p : counters)
    {
        std::cout << stream_names[p.first] << "[" << p.first << "]: " << p.second << " [frames] || ";
    }
}
```
