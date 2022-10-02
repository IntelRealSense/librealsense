# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device D400*

import time
import pyrealsense2 as rs

from rspy import test


def close_resources(sensor):
    """
    Stop and Close sensor.
    :sensor: sensor of device
    """
    if len(sensor.get_active_streams()) > 0:
        sensor.stop()
        sensor.close()


def set_and_verify_timestamp_domain(sensor, global_time_enabled: bool):
    """
    Perform sensor (depth or color) test according given global time
    :sensor: depth or color sensor in device
    :global_time_enabled bool: True - timestamp is enabled otherwise false
    """
    global frame_queue

    sensor.set_option(rs.option.global_time_enabled, global_time_enabled)
    time.sleep(0.3)  # Waiting for new frame from device. Need in case low FPS.
    frame = frame_queue.wait_for_frame()

    if not frame:
        test.fail()

    expected_ts_domain = rs.timestamp_domain.global_time if global_time_enabled else \
        rs.timestamp_domain.hardware_clock

    test.check_equal(sensor.get_option(rs.option.global_time_enabled), global_time_enabled)
    test.check_equal(frame.get_frame_timestamp_domain(), expected_ts_domain)


frame_queue = rs.frame_queue(capacity=1, keep_frames=False)
device = test.find_first_device_or_exit()

# Depth sensor test
depth_sensor = device.first_depth_sensor()
depth_profile = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth)
depth_sensor.open(depth_profile)
depth_sensor.start(frame_queue)

# Test #1
test.start('Check setting global time domain: depth sensor - timestamp domain is OFF')
set_and_verify_timestamp_domain(depth_sensor, False)
test.finish()

# Test #2
test.start('Check setting global time domain: depth sensor - timestamp domain is ON')
set_and_verify_timestamp_domain(depth_sensor, True)
test.finish()

close_resources(depth_sensor)

# Color sensor test
color_sensor = device.first_color_sensor()
color_profile = next(p for p in color_sensor.profiles if p.stream_type() == rs.stream.color)
color_sensor.open(color_profile)
color_sensor.start(frame_queue)

# Test #3
test.start('Check setting global time domain: color sensor - timestamp domain is OFF')
set_and_verify_timestamp_domain(color_sensor, False)
test.finish()

# Test #4
test.start('Check setting global time domain: color sensor - timestamp domain is ON')
set_and_verify_timestamp_domain(color_sensor, True)
test.finish()

close_resources(color_sensor)

test.print_results_and_exit()
