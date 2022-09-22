# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device D400*

import time
import pyrealsense2 as rs

from rspy import test


def get_connected_device():
    """
    Get first connected device
    :return: connected device
    """
    dev = None
    try:
        context = rs.context()
        dev = context.devices[0]
    except Exception as ex:
        print('Exception: Failed to find connected device. ', str(ex))
    return dev


def close_resources(sensor):
    """
    Stop and Close sensor.
    :param: sensor of device
    """
    if len(sensor.get_active_streams()) > 0:
        sensor.stop()
        sensor.close()


def start_sensor_test(sensor, global_time_enabled: int):
    """
    Perform depth sensor test according given global time
    :global_time_enabled int: 1 - time is enabled, 0 - time disabled
    """

    if global_time_enabled not in [0, 1]:
        raise ValueError(f'Invalid parameter in start depth test: {global_time_enabled}. Only 1 or 0 are valid.')

    try:
        sensor.set_option(rs.option.global_time_enabled, global_time_enabled)
        time.sleep(2)

        frame = frame_queue.wait_for_frame()

        if global_time_enabled == 0 and sensor.get_option(rs.option.global_time_enabled) == 0:
            test.check_equal(frame.get_frame_timestamp_domain(), rs.timestamp_domain.hardware_clock)

        elif global_time_enabled == 1 and sensor.get_option(rs.option.global_time_enabled) == 1:
            test.check_equal(frame.get_frame_timestamp_domain(),
                             rs.timestamp_domain.global_time)

    except Exception as exc:
        print(str(exc))


depth_sensor = None
color_sensor = None

try:
    device = get_connected_device()
    depth_sensor = device.first_depth_sensor()
    color_sensor = device.first_color_sensor()

    depth_profile = next(p for p in depth_sensor.profiles if p.fps() == 30
                         and p.stream_type() == rs.stream.depth
                         and p.format() == rs.format.z16
                         and p.as_video_stream_profile().width() == 640
                         and p.as_video_stream_profile().height() == 480)

    color_profile = next(p for p in color_sensor.profiles if p.fps() == 30
                         and p.stream_type() == rs.stream.color
                         and p.format() == rs.format.yuyv
                         and p.as_video_stream_profile().width() == 640
                         and p.as_video_stream_profile().height() == 480)

    depth_sensor.open(depth_profile)
    color_sensor.open(color_profile)
    frame_queue = rs.frame_queue(capacity=10, keep_frames=False)

    depth_sensor.start(frame_queue)

    # Test #1
    test.start('Start depth sensor test: global time disabled')
    start_sensor_test(depth_sensor, 0)
    test.finish()

    # Test #2
    test.start('Start depth sensor test: global time enabled')
    start_sensor_test(depth_sensor, 1)
    test.finish()

    close_resources(depth_sensor)
    color_sensor.start(frame_queue)

    # Test #3
    test.start('Start color sensor test: global time disabled')
    start_sensor_test(color_sensor, 0)
    test.finish()

    # Test #4
    test.start('Start color sensor test: global time enabled')
    start_sensor_test(color_sensor, 1)
    test.finish()

except ValueError as v:
    print(str(v))
except Exception as e:
    print("The device found has no depth sensor or ", str(e))
finally:
    # close_resources(depth_sensor)
    close_resources(color_sensor)
