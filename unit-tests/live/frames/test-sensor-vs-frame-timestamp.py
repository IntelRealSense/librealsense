# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test
import time

dev, _ = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
is_global_time_enabled_orig = False
fps = 30
has_at_least_one_frame_arrived = False


def check_hw_ts_right_before_sensor_ts(frame):
    global has_at_least_one_frame_arrived
    has_at_least_one_frame_arrived = True
    frame_ts_supported = frame.supports_frame_metadata(rs.frame_metadata_value.frame_timestamp)
    sensor_ts_supported = frame.supports_frame_metadata(rs.frame_metadata_value.sensor_timestamp)
    test.check(frame_ts_supported and sensor_ts_supported)
    hw_ts = frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
    sensor_ts = frame.get_frame_metadata(rs.frame_metadata_value.sensor_timestamp)
    delta = hw_ts - sensor_ts
    time_between_frames = 1 / fps * 1000000
    if not test.check_between(delta, min=0, max=time_between_frames):
        print("hw_ts = " + repr(hw_ts) + ", sensor_ts = " + repr(sensor_ts) + ", delta = " + repr(delta))
        print("time between frames = " + repr(time_between_frames))


######################### DEPTH SENSOR ######################################################
#############################################################################################
with test.closure("Set Depth stream time domain to HW"):
    is_global_time_enabled_orig = depth_sensor.get_option(rs.option.global_time_enabled)
    if is_global_time_enabled_orig:
        depth_sensor.set_option(rs.option.global_time_enabled, 0)
    test.check_equal(int(depth_sensor.get_option(rs.option.global_time_enabled)), 0)

#############################################################################################
with test.closure("Get Depth frame timestamp and compare it to sensor timestamp"):
    depth_profile = next(p for p in
                         depth_sensor.profiles if p.fps() == fps
                         and p.stream_type() == rs.stream.depth
                         and p.format() == rs.format.z16
                         and p.as_video_stream_profile().width() == 1280
                         and p.as_video_stream_profile().height() == 720)

    depth_sensor.open(depth_profile)
    depth_sensor.start(check_hw_ts_right_before_sensor_ts)
    time.sleep(1)
    test.check(has_at_least_one_frame_arrived)
    depth_sensor.stop()
    depth_sensor.close()

#############################################################################################
with test.closure("Restore Depth original time domain"):
    if is_global_time_enabled_orig:
        depth_sensor.set_option(rs.option.global_time_enabled, 1)
    test.check_equal(int(depth_sensor.get_option(rs.option.global_time_enabled)), 1)

#############################################################################################


color_sensor = dev.first_color_sensor()
is_global_time_enabled_orig = False
fps = 30
has_at_least_one_frame_arrived = False

##################### COLOR SENSOR ##########################################################
#############################################################################################
with test.closure("Set Color stream time domain to HW"):
    is_global_time_enabled_orig = color_sensor.get_option(rs.option.global_time_enabled)
    if is_global_time_enabled_orig:
        color_sensor.set_option(rs.option.global_time_enabled, 0)
    test.check_equal(int(color_sensor.get_option(rs.option.global_time_enabled)), 0)

#############################################################################################
with test.closure("Get Color frame timestamp and compare it to sensor timestamp"):
    color_profile = next(p for p in
                         color_sensor.profiles if p.fps() == fps
                         and p.stream_type() == rs.stream.color
                         and p.format() == rs.format.rgb8
                         and p.as_video_stream_profile().width() == 1280
                         and p.as_video_stream_profile().height() == 720)

    color_sensor.open(color_profile)
    color_sensor.start(check_hw_ts_right_before_sensor_ts)
    time.sleep(1)
    test.check(has_at_least_one_frame_arrived)
    color_sensor.stop()
    color_sensor.close()

#############################################################################################
with test.closure("Restore Color original time domain"):
    if is_global_time_enabled_orig:
        color_sensor.set_option(rs.option.global_time_enabled, 1)
    test.check_equal(int(color_sensor.get_option(rs.option.global_time_enabled)), 1)
test.print_results_and_exit()
