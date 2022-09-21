# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test: device D435i

import time
import pyrealsense2 as rs
from rspy import test

# This test check Timestamp toggle option for color and depth sensor


def callback(frame):
    global expected_time_stamp_domain
    test.check_equal(int(frame.get_frame_timestamp_domain()), expected_time_stamp_domain)


################################################################################################
test.start("Start test")

ENABLE = 1
DISABLE = 0

devices = test.find_first_device_or_exit()

time.sleep(3)  # The device starts at D0 (Operational) state, allow time for it to get into idle state
depth_sensor = devices.first_depth_sensor()
color_sensor = devices.first_color_sensor()

depth_sensor.get_option(rs.option.global_time_enabled)
color_sensor.get_option(rs.option.global_time_enabled)

# Using a profile common to both L500 and D400
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
depth_sensor.start(callback)

#############################################################################################
# Test #1
expected_time_stamp_domain = 0
depth_sensor.set_option(rs.option.global_time_enabled, DISABLE)
test.check_equal(DISABLE, depth_sensor.get_option(rs.option.global_time_enabled))
time.sleep(0.2)

#############################################################################################
# Test #2
expected_time_stamp_domain = 2
depth_sensor.set_option(rs.option.global_time_enabled, ENABLE)
test.check_equal(ENABLE, depth_sensor.get_option(rs.option.global_time_enabled))
time.sleep(0.2)

#############################################################################################

depth_sensor.stop()
depth_sensor.close()

color_sensor.open(color_profile)
color_sensor.start(callback)

#############################################################################################
# Test #3
expected_time_stamp_domain = 0
color_sensor.set_option(rs.option.global_time_enabled, DISABLE)
test.check_equal(DISABLE, color_sensor.get_option(rs.option.global_time_enabled))
time.sleep(0.2)

#############################################################################################
# Test #4
expected_time_stamp_domain = 2
color_sensor.set_option(rs.option.global_time_enabled, ENABLE)
test.check_equal(ENABLE, color_sensor.get_option(rs.option.global_time_enabled))
time.sleep(0.2)

#############################################################################################

color_sensor.stop()
color_sensor.close()

test.finish()
test.print_results_and_exit()
