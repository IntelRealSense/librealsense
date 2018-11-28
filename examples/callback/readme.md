# rs-callback Sample

## Overview

This sample demonstrates how to configure the camera for streaming frames using the pipeline's callback API.
This API is recommended when streaming high frequency data such as IMU ([Inertial measurement unit](https://en.wikipedia.org/wiki/Inertial_measurement_unit)) since the callback is invoked immediately once the a frame is ready.
This sample prints a frame counter for each stream, the code demonstrates how it can be done correctly by synchronizing the callbacks.

## Expected Output
![rs-callback](https://user-images.githubusercontent.com/18511514/48921401-37a0c680-eea8-11e8-9ab4-18e566d69a8a.PNG)

## Code Overview 

First, we include the Intel® RealSense™ Cross-Platform API.  
All but advanced functionality is provided through a single header:
```cpp
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
```

We define frame counters that will be updated every time a frame arrives.
This counters will be used in the application main loop to print how many frames arrived from each stream.
```cpp
std::map<int, int> counters;
std::map<int, std::string> stream_names;
```

The mutex object is a synchronization primitive that will be used to protect our frame counters from being simultaneously accessed by multiple threads.
```cpp
std::mutex mutex;
```

Define the frame callback which will be invoked by the pipeline on the sensors thread once a frame (or synchronised frameset) is ready.
```cpp
// Define frame callback
// The callback is executed on a sensor thread and can be called simultaneously from multiple sensors.
// Therefore any modification to shared data should be done under lock.
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

Collect the stream names from the returned `pipeline_profile` object:
```cpp
// Collect the enabled streams names
for (auto p : profiles.get_streams())
    stream_names[p.unique_id()] = p.stream_name();
```

Finally, print the frame counters once every second.
After calling `start`, the main thread will continue to execute work, so even when no other action is required, we need to keep the application alive. 
In order to protect our counters from being accessed simultaneously, access the counters using the mutex.
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
