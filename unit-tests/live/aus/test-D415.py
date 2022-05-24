# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device D415

import pyrealsense2 as rs
from rspy import devices, log, test, file, repo

#############################################################################################
# Tests
#############################################################################################

test.start("Init colorizer test")
try:

    pipeline = rs.pipeline()
    pipeline.start()
    for i in range(10):
        frames = pipeline.wait_for_frames()
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Test counters names")
try:
    counters_list = rs.aus_get_counters_list()
    expected_counters_dict = \
        {
            'RS2_AUS_INTEL_REALSENSE_D415_COLOR_TIMER': -1,
            'RS2_AUS_CONNECTED_DEVICES_COUNTER': 1,
            'RS2_AUS_DS5_CONNECTED_DEVICES_COUNTER': 1,
            'RS2_AUS_INTEL_REALSENSE_D415_DEPTH_TIMER': -1.
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
            test.check((aus_val >= 0) and (aus_val <= 1))
except:
    test.unexpected_exception()
test.finish()

test.print_results_and_exit()
