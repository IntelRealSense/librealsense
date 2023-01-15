# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device D435

import pyrealsense2 as rs
from rspy import test

#############################################################################################
# Tests
#############################################################################################

test.start("Init colorizer")
pipeline = rs.pipeline()
pipeline.start()
for i in range(10):
    frames = pipeline.wait_for_frames()
test.finish()

#############################################################################################

test.start("Test counters names")
counters_list = rs.aus_get_counters_list()
expected_counters_dict = \
    {
        'RS2_AUS_INTEL_REALSENSE_D435_COLOR_TIMER': -1,
        'RS2_AUS_CONNECTED_DEVICES_COUNTER': 1,
        'RS2_AUS_DS5_CONNECTED_DEVICES_COUNTER': 1,
        'RS2_AUS_INTEL_REALSENSE_D435_DEPTH_TIMER': -1.
    }
test.check_equal_lists( counters_list, expected_counters_dict.keys())
test.finish()

#############################################################################################

test.start("Test counters values")
for counter_name in expected_counters_dict.keys():
    aus_val = rs.aus_get(counter_name)
    if "COUNTER" in counter_name:
        expected_val = expected_counters_dict[counter_name]
        test.check_equal(aus_val, expected_val)
    else:
        test.check((aus_val >= 0) and (aus_val <= 1))
test.finish()

#############################################################################################

test.print_results_and_exit()
