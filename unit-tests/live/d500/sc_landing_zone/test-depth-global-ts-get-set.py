# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time



################# Checking global timestamp for depth ##################

test.start("Checking global timestamp for depth")
ctx = rs.context()
dev = ctx.query_devices()[0]
depth_sensor = dev.query_sensors()[0]

test.check(depth_sensor.supports(rs.option.global_time_enabled))
global_ts_value = depth_sensor.get_option(rs.option.global_time_enabled)

gt_new_value = -1
if global_ts_value == 0:
    gt_new_value = 1
else:
    gt_new_value = 0

depth_sensor.set_option(rs.option.global_time_enabled, gt_new_value)
time.sleep(0.5)
global_ts_value = depth_sensor.get_option(rs.option.global_time_enabled)
test.check_equal(global_ts_value, gt_new_value)
test.finish()



test.print_results_and_exit()
