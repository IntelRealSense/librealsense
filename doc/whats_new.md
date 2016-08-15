# What's New

## ZR300 Support

ZR300 is the newest evolution in the family of Intel® RealSense™ R200 cameras. In addition to RGB, Infrared and Depth streaming, ZR300 provides built-in Fish-Eye camera, a motion module (Accelerometer, Gyroscope) and the means to synchronize between all sensors (for navigation and robotics).
The development branch supports ZR300 (as well as the LR200 variant) out of the box.

Frames from the **Fish-Eye** camera can be obtained through standard librealsense APIs:

    if(dev->supports(rs::capabilities::fish_eye))
    {
        dev->enable_stream(rs::stream::fisheye, 640, 480, rs::format::raw8, 60);
    }
    dev->start(); // start streaming
    dev->wait_for_frames(); // wait for frames
    dev->get_frame_data(rs::capabilities::fish_eye); // returns 1bpp grayscale fish-eye image

(Refer to [cpp-config-ui](./../examples/cpp-config-ui.cpp) for more advanced sample of the fish-eye sensor)

**Motion Module** events are pushed at the application at high frequency (200Hz) and should be handled asynchronously, outside the main wait_for_frames loop:

    if (dev->supports(rs::capabilities::motion_events))
    {
        // Configure motion data callback
        // and the timestamp packets callback (generated on Depth/Fish-Eye Frames and triggers from external GPIOS)
        dev->enable_motion_tracking(
          [](rs::motion_data entry)
          {
              // entry.axis holds Gyro / Accelerometer data
          },
          [](rs::timestamp_data entry)
          {
              // timestamp packets hold frame number, frame type and unified timestamp
          });
    }
    dev->start(rs::source::motion_data); // start generating motion data
    // the relevant callbacks will be invoked asynchronously
(Please refer to [cpp-motion-module](./../examples/cpp-motion-module.cpp) sample)

In order to receive proper timestamps from the microcontroller **fisheye_strobe** option must be toggled on.

## Asynchronous APIs

While wait_for_frames API is extremely convenient for applications expecting coherent sets of frames, this convenience is achieved at the expense of latency. The development branch provides easy complementing APIs that let latency-sensitive applications get frame data as soon as possible:

    dev->set_frame_callback(rs::stream::depth, [dev, &frames_queue](rs::frame frame)
    {
        frame->get_data(); // access the frame data as soon as possible
        // frame reference can be moved to separate processing thread:
        frames_queue.enqueue(std::move(frame));
        // the actual pixels will not be copied, just the slim frame reference
    });
    dev->start();
    auto frame = frames_queue.deque();
    frame->get_data(); // access frame data from the main thread

Frame object can be moved between threads **without extra copies** of the underlying data. When the last reference to frame is released, frame memory will be recycled.


(Refer to [cpp-callback-2](./../examples/cpp-callback-2.cpp) for full demo)


## Memory Management Enhancements

In effort to minimize the librealsense-imposed latency and improve the overall performance, the development branch provides an opt-in support to get the raw data from the relevant backend when it is possible, avoiding **any extra memory copies**:

    if(dev->supports(rs::capabilities::fish_eye))
    {
        dev->enable_stream(rs::stream::fisheye, 640, 480, rs::format::raw8, 60,
          rs::output_buffer_format::native); // request direct access to the buffer from the backend
    }

Once this buffer access mode is configured, nothing else has to change unless the frame has  [stride](https://en.wikipedia.org/wiki/Stride_of_an_array) != width. If this is the case, the pixels will not be continuous in memory and it's up to the application to handle it. For example (when using OpenGL) to set up custom stride the application could call:

    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.get_stride());

When using this mode, get_frame_data will return direct pointer to backend memory. Once the frame is released, memory will be returned to the backend and next frame would be requested.

(Refer to [cpp-stride](./../examples/cpp-stride.cpp) for an example)
