# Software Support Model - Intel® RealSense™ SDK 2.0

## Overview

With the release of the D400 series of Intel® RealSense™ devices, Intel RealSense Group is introducing a number of changes to the **librealsense** support.

Software support for Intel® RealSense™ technology will be split into two coexisting release lineups: **librealsense 1.x** and **librealsense 2+**.

 * **librealsense 1.x** will continue to provide support for RealSense™ devices: F200, R200 and ZR300.

 * **librealsense 2+** will support the next generation of RealSense™ devices, starting with the RS300 and RS400.


 ## API Changes

**librealsense2** brings the following improvements and new capabilities (which are incompatible with older RealSense devices):

### Streaming API
* **librealsense2** provides a more flexible interface for frame acquisition.  
Instead of a single `wait_for_frames` loop, the API is based on callbacks and queues:
```cpp
// Configure queue of size one and start streaming frames into it
rs2::frame_queue queue(1);
dev.start(queue);
// This call will block the current thread until new frames become available
auto frame = queue.wait_for_frame();
auto pixels = frame.get_data(); // pointer to frame data
```
* The same API can be used in a slightly different way for **low-latency** applications:
```cpp
// Configure direct callback for new frames:
dev.start([](rs2::frame frame){
    auto pixels = frame.get_data(); // pointer to frame data
});
// The application will be notified as soon as new frames become available.
```
**Note:** This approach allows users to bypass the buffering and synchronization that was done by `wait_for_frames`.

*  Users who do need to synchronize between different streams can take advantage of the `rs2::syncer` class:
```cpp
sycner sync;
dev.start(sync);
// The following call, using the frame timestamp, will block the
// current thread until the next coherent set of frames become available
auto frames = sync.wait_for_frames();
for (auto&& frame : frames)
{
    auto pixels = frame.get_data(); // pointer to frame data
}
```
* This version of `wait_for_frames` is **thread-safe** by design. You can safely pass a `rs::frame` object to a background thread. This is done without copying the frame data and without extra dynamic allocations.

### Multi-Streaming Model

**librealsense2** eliminates limitations imposed by previous versions with regard to multi-streaming:
* Multiple applications can use librealsense2 simultaneously, as long as no two users try to stream from the same camera endpoint.  
In practice, this means that you can:
  * Stream multiple cameras within a single process
  * Stream from camera A in one process and from camera B in another process
  * Stream depth from camera A in one process while streaming fisheye / motion from the same camera in another process
  * Stream from camera A in one process while issuing controls to camera A from another process
* The following streams of the RS400 act as independent endpoints: Depth, Fisheye, Motion-tracking, Color
* Each endpoint can be exclusively locked using `open/close` methods:
```cpp
// Configure depth to run at first permitted configuration
dev.open(dev.get_stream_profiles().front());
// From this point on, device streaming is exclusively locked.
dev.close(); // Release device ownership
```
* Alternatively, users can use the  `rs2::util::config` helper class to configure multiple endpoints at once:
```cpp
rs2::util::config config;
// Declare your preferences
config.enable_all(rs2::preset::best_quality);
// The config object resolves them into concrete capabilities for the supplied camera
auto stream = config.open(dev);
stream.start([](rs2::frame) {});
```

## New Functionality

* **librealsense2** will be shipped with a built-in [Python Wrapper](../wrappers/python/) for easier integration.
* New [troubleshooting tools](../tools/) are now part of the package, including a tool for hardware log collection.
* **librealsense2** is capable of handling device disconnects and the discovery of new devices at runtime.
* [Playback & Record](../src/media/readme.md) functionality is available out-of-the-box. 

## Transition to CMake
**librealsense2** does not provide hand-written Visual Studio, QT-Creator and XCode project files as you can build **librealsense** with the IDE of your choice using portable CMake scripts.  

## Intel® RealSense™ RS400 and the Linux Kernel

* The Intel® RealSense™ RS400 series (starting with kernel 4.4.0.59) does not require any kernel patches for streaming 
* Advanced camera features may still require kernel patches. Currently, getting **hardware timestamps** is dependent on a patch that has not yet been up-streamed. Without the patch applied you can still use the camera but you will receive a system-time instead of an optical timestamp.
