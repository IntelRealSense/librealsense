
See also: [device](device.md)


# Stream Configurations

___
## Infrared

When only a single infrared stream is available, only a single topic is expected.

When multiple streams are available (Left/Right), two topics will be available with each providing the same format and encoding. This is true even if the sensor generates an interlaced format: it is up the server to separate into two separate streams, each distinct.

#### Format

See the [ROS2 Image format](https://github.com/ros2/common_interfaces/blob/rolling/sensor_msgs/msg/Image.msg).

#### Encoding

This should be set to `mono8`, corresponding to `Y8` in librealsense. No interlaced formats.

#### Metadata

A single metadata message is expected for all messages from the same sensor (so one for depth, IR left, IR right).

#### Timestamps

If two streams, they should be synchronizable via identical timestamps.

#### Quality of Service

- Reliability: `BEST_EFFORT`
- Durability: `VOLATILE`

___
## Motion

Otherwise know as IMU (Inertial Measurement Unit), this is a combined accelerometer and gyroscope stream.

High frequency is expected for motion messages. Therefore:

#### Topics

Rather than separate topics for each of the Accel and Gyro streams, a single topic will carry both, combined.

#### Format

See the [ROS2 Imu format](https://github.com/ros2/common_interfaces/blob/rolling/sensor_msgs/msg/Imu.msg).

#### Metadata

No metadata is expected for motion topics.

#### Timestamps

If only one of Accel/Gyro is requested, the other's values can remain zeroes. The timestamp will refer to the sample's time.

If both Accel and Gyro are requested, they are packaged together but their values may be generated independently with different timestamps. The timestamp should reflect the Gyro timestamp. The Accel values will either reflect the last values received (which may not reflect the values at that specific timestamp) or interpolated values to align with the Gyro. The latter is preferable.

#### Quality of Service

- Reliability: `BEST_EFFORT`
- Durability: `VOLATILE`
