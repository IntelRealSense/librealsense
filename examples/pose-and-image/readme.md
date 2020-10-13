# rs-pose-and-image Sample

In order to run this example, a T265 is required.

## Overview

This sample demonstrates streaming pose data at 200Hz and image data at 30Hz using an asynchronous pipeline configured with a callback.

## Expected Output

The application will calculate and print the number of samples received from each sensor per second.

## Code Overview

We start by configuring the pipeline to pose stream, similar to `rs-pose-predict`:
```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;
// Create a configuration for configuring the pipeline with a non default profile
rs2::config cfg;
// Add pose stream
cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
// Enable both image streams
// Note: It is not currently possible to enable only one
cfg.enable_stream(RS2_STREAM_FISHEYE, 1, RS2_FORMAT_Y8);
cfg.enable_stream(RS2_STREAM_FISHEYE, 2, RS2_FORMAT_Y8);
```

Next, we define new frame callback ([learn more about callback API](../callback)) that executes for every new pose or frameset event from the sensor. The implementation of the callback is simple, we just want to keep track of how many of each type of frame are received.

Approximately once per second, we print the number of poses received in that second.

```cpp
// The callback is executed on a sensor thread and can be called simultaneously from multiple sensors
// Therefore any modification to common memory should be done under lock
std::mutex data_mutex;
uint64_t pose_counter = 0;
uint64_t frame_counter = 0;
bool first_data = true;
auto last_print = std::chrono::system_clock::now();
auto callback = [&](const rs2::frame& frame)
{
	std::lock_guard<std::mutex> lock(data_mutex);
	// Only start measuring time elapsed once we have received the
	// first piece of data
	if (first_data) {
		first_data = false;
		last_print = std::chrono::system_clock::now();
	}

	if (auto fp = frame.as<rs2::pose_frame>()) {
		pose_counter++;
	}
	else if (auto fs = frame.as<rs2::frameset>()) {
		frame_counter++;
	}

	// Print the approximate pose and image rates once per second
	auto now = std::chrono::system_clock::now();
	if (now - last_print >= std::chrono::seconds(1)) {
		std::cout << "\r" << std::setprecision(0) << std::fixed 
				  << "Pose rate: "  << pose_counter << " "
				  << "Image rate: " << frame_counter << std::flush;
		pose_counter = 0;
		frame_counter = 0;
		last_print = now;
	}
};
```

Two things to note:
1. Image frames arrive as a coordinated pair (a `rs2::frameset`)
2. This is approximate because we use the system time to decide when the interval being measured starts and stops (`now - last_print >= std::chrono::seconds(1)`) and because it measures the number of items received in the interval rather than using the data timestamps, so any system latency can make the number change a little bit.

Now we can start the pipeline with the callback, and the main thread can sleep until we are done

```cpp
// Sleep this thread until we are done
while(true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
```
