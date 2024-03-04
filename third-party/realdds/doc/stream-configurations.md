
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

#### Units

Following the ROS2 standard, acceleration should be in `m/s^2`. Rotational velocity should be in `rad/sec`.

#### Metadata

No metadata is expected for motion topics.

#### Timestamps

The timestamp will always refer to the Gyro sample time: even if both Accel and Gyro values are generated independently with different timestamps!

Any Accel values, if present, will either reflect the last values received (so may not reflect the values at that specific timestamp) or interpolated values to align with the Gyro. The latter is preferable.

#### Motion Correction

The Gyro/Accel values sent as part of the format may be the raw values output by the sensor, or values that are post-calibration (with intrinsic bias and scaling corrections applied).

When raw values are output, it is up to the user to apply any intrinsic correction. Usually this would be done with librealsense if calibration of the accelerometer or gyroscope was done (see [here](../../../tools/rs-imu-calibration/)). By default, the calibration matrices are identity matrices, i.e. preserving the raw sensor values.

It is recommended that the server output values post-calibration, such that librealsense is not needed. An option ("Enable Motion Correction" in librealsense) can be used to control this.

#### Quality of Service

- Reliability: `BEST_EFFORT`
- Durability: `VOLATILE`
