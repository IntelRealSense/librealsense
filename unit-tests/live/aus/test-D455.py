# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device D455

import pyrealsense2 as rs
from rspy import test

#############################################################################################
# Tests
#############################################################################################

# get 10 frames from pipeline, then do some tests
pipeline = rs.pipeline()
pipeline.start()
for i in range(10):
    frames = pipeline.wait_for_frames()

#############################################################################################

test.start("Test counters names")
counters_list = rs.aus_get_counters_list()
expected_counters_dict = \
    {
        'RS2_AUS_CONNECTED_DEVICES_COUNTER': 1,
        'RS2_AUS_INTEL_REALSENSE_D455_CONNECTED_DEVICES_COUNTER': 1,
        'RS2_AUS_INTEL_REALSENSE_D455_DEPTH_TIMER': -1,
        'RS2_AUS_INTEL_REALSENSE_D455_GYRO_TIMER': -1,
        'RS2_AUS_INTEL_REALSENSE_D455_COLOR_TIMER': -1,
        'RS2_AUS_INTEL_REALSENSE_D455_ACCEL_TIMER': -1,
    }
test.check_equal_lists(sorted(counters_list), sorted(expected_counters_dict.keys()))
test.finish()

#############################################################################################

test.start("Test counters values")
for counter_name in expected_counters_dict.keys():
    aus_val = rs.aus_get(counter_name)
    if "COUNTER" in counter_name:
        # check that counter is equal to expected result
        expected_val = expected_counters_dict[counter_name]
        test.check_equal(aus_val, expected_val)

test.finish()

############################################################################################

# colorize 20 depth frames, then run some tests
colorizer = rs.colorizer()
for i in range(20):
    frames = pipeline.wait_for_frames()
    depth_frame = frames.get_depth_frame()
    depth_color_frame = colorizer.colorize(depth_frame)

############################################################################################

test.start("Test number of colorized frames")
counters_list = rs.aus_get_counters_list()
test.check('RS2_AUS_DEPTH_VISUALIZATION_COLORIZED_FRAMES_COUNTER' in counters_list)
colorized_frames = rs.aus_get('RS2_AUS_DEPTH_VISUALIZATION_COLORIZED_FRAMES_COUNTER')
test.check_equal(colorized_frames, 20)
test.finish()

############################################################################################

test.print_results_and_exit()
