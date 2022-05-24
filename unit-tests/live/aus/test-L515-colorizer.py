# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device L515

import pyrealsense2 as rs
from rspy import devices, log, test, file, repo

#############################################################################################
# Tests
#############################################################################################

test.start("Init colorizer test")
try:

    pipeline = rs.pipeline()
    colorizer = rs.colorizer()
    pipeline.start()
    for i in range(10):
        frames = pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        depth_color_frame = colorizer.colorize(depth_frame)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Test counters names")
try:
    counters_list = rs.aus_get_counters_list()
    expected_counters_dict = \
        {
            'RS2_AUS_DEPTH_VISUALIZATION_COLORIZER_TIMER': -1,
            'RS2_AUS_INTEL_REALSENSE_L515_ACCEL_TIMER': -1 ,
            'RS2_AUS_DEPTH_VISUALIZATION_COLORIZER_FILTER_INIT_COUNTER': 1,
            'RS2_AUS_CONNECTED_DEVICES_COUNTER': 1,
            'RS2_AUS_L500_CONNECTED_DEVICES_COUNTER': 1,
            'RS2_AUS_INTEL_REALSENSE_L515_COLOR_TIMER': -1,
            'RS2_AUS_INTEL_REALSENSE_L515_INFRARED_TIMER': -1,
            'RS2_AUS_INTEL_REALSENSE_L515_DEPTH_TIMER': -1,
            'RS2_AUS_INTEL_REALSENSE_L515_GYRO_TIMER': -1,
            'RS2_AUS_DEPTH_VISUALIZATION_COLORIZED_FRAMES_COUNTER': 10,
        }


    test.check_equal_lists( counters_list, expected_counters_dict.keys())
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Test counters values")
try:
    for counter_name in expected_counters_dict.keys():
        aus_val = rs.aus_get(counter_name)
        if "TIMER" not in counter_name:
            expected_val = expected_counters_dict[counter_name]
            test.check_equal(aus_val, expected_val)
        else:
            test.check((aus_val >= 1) and (aus_val <= 2))

except:
    test.unexpected_exception()
test.finish()

test.print_results_and_exit()
