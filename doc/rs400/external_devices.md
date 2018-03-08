# External Devices and RS4XX

D400 series of RealSense devices include several features for integration with external sensors.
This document will explain how these are exposed in librealsense.

## Output Trigger

In addition to the USB3 connector, D400 cameras expose headers for interaction with external devices.
The camera can provide trigger every time frame is being captured (with D400 as the master).

To enable this mode, all you need to do is:

```cpp
dev.set_option(RS2_OPTION_OUTPUT_TRIGGER, 1);
```

## Timestamp from External sensors

Some limited D400x modules can be connected with a Tracking Module (TM) which includes Fish-Eye and Motion-Tracking sensors on board, all synchronized to single clock.
If you wish to synchronize external sensor (for example a compass) with RealSense, all you need to do is trigger one of 4 GPIOs of the D400x whenever you read data from the external sensors. D400x will generate timestamp for each GPIO event, allowing you to synchronize between the data streams in software:

```cpp
rs2::util::config config;
config.enable_stream(RS2_STREAM_GPIO1, 0, 0, 0, RS2_FORMAT_GPIO_RAW);
auto stream = config.open(dev);

rs2::frame_queue events_queue;
stream.start(events_queue);

while (true)
{
    rs2::frame f = events_queue.wait_for_frame();
    std::cout << "External sensors connected to GPIO1 fired at " << f.get_timestamp() << "\n";
}
```

You can mix and match GPIO streams with other types of streams: you are free to use frame queues or callbacks, you can configure the stream directly or using `utils::config` helper class, you can use `utils::syncer` to group frames with closest GPIO event.
