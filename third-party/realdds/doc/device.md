
See the [list of topics used](../include/realdds/topics/topics.md) for a general overview of the topic structure.


# The DDS Device

A DDS device is a collection of topics on which information flows.

Specifically:
* `<topic-root>/`
    * `notification` — server notifications, responses, etc.
    * `control` — client requests to server
    * `metadata` — optional stream information
* `rt/<topic-root>_` — [ROS2](#ros2)-compatible streams
    * `<stream-name>` — [Image](https://github.com/ros2/common_interfaces/blob/rolling/sensor_msgs/msg/Image.msg)/[Imu](https://github.com/ros2/common_interfaces/blob/rolling/sensor_msgs/msg/Imu.msg) streams


## The Topic Root

Note that topics are all using the same `<topic-root>` that is specified in the [`device-info`](discovery.md). I.e., if the topic-root is known, all other topics can be found.

As long as the `<topic-root>` is a valid topic path, it can be anything and is entirely determined by [Discovery](discovery.md).

Currently, the topic root as used by `rs-dds-server` is built as:
> realsense/ `<device-model>` _ `<serial-number>`


## Continue with:

* [Notifications](notifications.md)
* [Initialization](initialization.md)
* [Control](control.md)
* [Streaming](streaming.md)
* [Metadata](metadata.md)
