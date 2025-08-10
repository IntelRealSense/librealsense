# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time
from rspy.timer import Timer

MAX_TIME_TO_WAIT_FOR_FRAMES = 5  # [sec]
NUM_OF_FRAMES_BEFORE_EACH_CHECK = 50

global_ts_value = None
global_ts_changed = False
global_frame_num = 0


def callback(frame):
    global global_ts_value
    global global_ts_changed
    global global_frame_num
    global_frame_num += 1
    if global_ts_changed:
        global_ts_changed = False
        domain = frame.get_frame_timestamp_domain()
        if global_ts_value == True:
            test.check_equal(domain, rs.time_domain.global_time)
        else:
            test.check_not_equal(domain, rs.time_domain.global_time)


def wait_and_check(num_of_frames_before_check):
    while not wait_for_frames_timer.has_expired() and global_frame_num < num_of_frames_before_check:
        time.sleep(0.5)
    if wait_for_frames_timer.has_expired():
        print("timer expired: " + repr(num_of_frames_before_check) + " frames did not arrived before " + repr(
            MAX_TIME_TO_WAIT_FOR_FRAMES) + "sec")
        test.fail()


wait_for_frames_timer = Timer(MAX_TIME_TO_WAIT_FOR_FRAMES)

################# Checking global timestamp for depth ##################

test.start("Checking global timestamp for depth")
dev, _ = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()

test.check(depth_sensor.supports(rs.option.global_time_enabled))

gt_value_to_set = 1
depth_sensor.set_option(rs.option.global_time_enabled, gt_value_to_set)
global_ts_value = int(depth_sensor.get_option(rs.option.global_time_enabled))
test.check_equal(global_ts_value, gt_value_to_set)

# Start streaming
depth_profile = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth and p.is_default())
depth_sensor.open(depth_profile)
depth_sensor.start(callback)

wait_for_frames_timer.start()
wait_and_check(NUM_OF_FRAMES_BEFORE_EACH_CHECK)
gt_value_to_set = 0
depth_sensor.set_option(rs.option.global_time_enabled, gt_value_to_set)
global_ts_changed = True

wait_for_frames_timer.start()
wait_and_check(2 * NUM_OF_FRAMES_BEFORE_EACH_CHECK)
gt_value_to_set = 1
depth_sensor.set_option(rs.option.global_time_enabled, gt_value_to_set)
global_ts_changed = True

time.sleep(0.5)
depth_sensor.stop()
depth_sensor.close()
test.finish()

test.print_results_and_exit()
