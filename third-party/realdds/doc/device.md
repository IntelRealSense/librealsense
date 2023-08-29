
See the [list of topics used](../include/realdds/topics/) for a general overview of the topic structure.


# The DDS Device

A DDS device is a collection of topics on which information flows.

Specifically:
* `<topic-root>/`
    * `notification` — [server notifications, responses, etc.](notifications.md)
    * `control` — [client requests to server](control.md)
    * `metadata` — [optional stream information](metadata.md)
* `rt/<topic-root>_` — ROS2-compatible [streams](streaming.md)
    * `<stream-name>` — [Image](https://github.com/ros2/common_interfaces/blob/rolling/sensor_msgs/msg/Image.msg)/[Imu](https://github.com/ros2/common_interfaces/blob/rolling/sensor_msgs/msg/Imu.msg) streams


## The Topic Root

Note that topics are all using the same `<topic-root>` that is specified in the [`device-info`](discovery.md). I.e., if the topic-root is known, all other topics can be found.

As long as the `<topic-root>` is a valid topic path, it can be anything and is entirely determined by [Discovery](discovery.md).

Currently, the topic root as used by `rs-dds-adapter` is built as:
> realsense/ `<device-model>` _ `<serial-number>`


## Settings

On the client, device behavior with librealsense can be fine-tuned with certain JSON settings passed through the participant (and therefore from the librealsense context's `dds` settings):

| Field                    | Default | Type    | Description        |
|--------------------------|---------|---------|--------------------|
| device-reply-timeout-ms  |    1000 | size_t  | How long to wait for a control reply to arrive, in millisec
| disable-metadata         |   false | bool    | When true, the metadata topic will be disabled, even with metadata-enabled streams

To control device-level options of the server's, controls need to be sent.
The server should publish its device options as part of the initialization sequence, in the `device-options` message.
If available, the [`get-option` and `set-option` controls](control.md#query-option--set-option) can be used (no `owner-name`) to retrieve or update the values. **Setting these options will not take effect until the device is reset.**

The following settings may have options available for controlling:

| Setting        | Default | Type             | `option-name`   | Description
|----------------|---------|------------------|-----------------|---------------
| Domain ID      |       0 | int 0-232        | `domain-id`     | The DDS domain number to use to segment communications on the network
| IP address     |       ? | string "#.#.#.#" | `ip-address`    | The static IP that the server uses for itself (if DHCP is off)
| DHCP enable    |       ? | bool             | `dhcp`          | If on, the `ip-address` is ignored and retrieved on startup from a DHCP server
| Multicast IP   |       - | string "#.#.#.#" | `multicast-ip` | The IP address to use for multicasting (empty to disable)


## Continue with:

* [Notifications](notifications.md)
* [Initialization](initialization.md)
* [Control](control.md)
* [Streaming](streaming.md)
* [Metadata](metadata.md)
* [Stream Configurations](stream-configurations.md)
