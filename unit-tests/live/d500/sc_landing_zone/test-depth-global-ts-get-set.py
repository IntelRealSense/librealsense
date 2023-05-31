# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time

global_ts_value = -1
global_ts_changed = False
def callback(frame):
    global global_ts_value
    global global_ts_changed
    if global_ts_changed:
        global_ts_changed = False
        domain = frame.get_frame_timestamp_domain()
        if global_ts_value == 1:
            test.check_equal(domain, rs.time_domain.global_time)
        else:
            test.check_not_equal(domain, rs.time_domain.global_time)


################# Checking global timestamp for depth ##################

test.start("Checking global timestamp for depth")
dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()

test.check(depth_sensor.supports(rs.option.global_time_enabled))

global_ts_orig_value = depth_sensor.get_option(rs.option.global_time_enabled)

gt_value_to_set = 1
depth_sensor.set_option(rs.option.global_time_enabled, gt_value_to_set)
global_ts_value = depth_sensor.get_option(rs.option.global_time_enabled)
test.check_equal(global_ts_value, gt_value_to_set)

# Start streaming
depth_profile = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth)
depth_sensor.open(depth_profile)
depth_sensor.start(callback)

time.sleep(0.5)
gt_value_to_set = 0
depth_sensor.set_option(rs.option.global_time_enabled, gt_value_to_set)
global_ts_changed = True

time.sleep(0.5)
gt_value_to_set = 1
depth_sensor.set_option(rs.option.global_time_enabled, gt_value_to_set)
global_ts_changed = True

time.sleep(0.5)
depth_sensor.stop()
depth_sensor.close()
test.finish()

test.print_results_and_exit()
