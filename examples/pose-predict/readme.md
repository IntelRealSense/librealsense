# rs-pose-predict Sample

> In order to run this example, a device supporting pose stream (T265) is required.

## Overview
This sample builds on the concepts presented in [`rs-pose` example](../pose/) and shows how pose data can be used asynchronously to implement simple pose prediction. 

## Expected Output
The application should open a window in which it prints the **predicted** x, y, z values of the device position relative to its initial position.

## Code Overview

We start by configuring the pipeline to pose stream, similar to previous example:
```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;
// Create a configuration for configuring the pipeline with a non default profile
rs2::config cfg;
// Add pose stream
cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
```

Next, we define new frame callback ([learn more about callback API](../callback)) that would execute for every new pose event from the sensor.
```cpp
// Define frame callback
// The callback is executed on a sensor thread and can be called simultaneously from multiple sensors
// Therefore any modification to common memory should be done under lock
std::mutex mutex;
auto callback = [&](const rs2::frame& frame)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (rs2::pose_frame fp = frame.as<rs2::pose_frame>()) {
        rs2_pose pose_data = fp.get_pose_data();
        rs2_pose predicted_pose = predict_pose(pose_data, dt_us);
        std::cout << pose_data.tracker_confidence << " : " << std::setprecision(3) << std::fixed << 
                predicted_pose.translation.x << " " <<
                predicted_pose.translation.y << " " <<
                predicted_pose.translation.z << " (meters)\r";
    }
};
```

Once pipeline is started with a callback, main thread can go to sleep until termination signal is received:
```cpp
// Start streaming through the callback with default recommended configuration
rs2::pipeline_profile profiles = pipe.start(cfg, callback);
std::cout << "started thread\n";
while(true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```
