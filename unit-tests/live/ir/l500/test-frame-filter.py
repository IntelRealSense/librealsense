# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

import pyrealsense2 as rs
from rspy.timer import Timer
from rspy import test
import time

# The L515 device opens the IR stream with the depth stream even if the user did not ask for it (for improving the depth quality), 
# The frame-filter role is to  make sure only requested frames will get to the user.
# This test is divided into 2 tests.
#   1. User ask for depth only - make sure he gets depth only frames
#   2. User ask for depth + IR - make sure he gets depth + IR frames

MAX_TIME_TO_WAIT_FOR_FRAMES = 10 # [sec]
NUMBER_OF_FRAMES_BEFORE_CHECK = 50 

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
device = devices[0]
depth_sensor = device.first_depth_sensor()

dp = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth 
    and p.fps() == 30)
irp = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.infrared 
    and p.fps() == 30)

n_depth_frame = 0
n_ir_frame = 0


def frames_counter(frame):
    if frame.get_profile().stream_type() == rs.stream.depth:
        global n_depth_frame
        n_depth_frame += 1
    elif frame.get_profile().stream_type() == rs.stream.infrared:
        global n_ir_frame
        n_ir_frame += 1

wait_frames_timer = Timer(MAX_TIME_TO_WAIT_FOR_FRAMES)

# Test Part 1
test.start("Ask for depth only - make sure only depth frames arrive")

depth_sensor.open(dp)
depth_sensor.start(frames_counter)
wait_frames_timer.start()

# we wait for first NUMBER_OF_FRAMES_BEFORE_CHECK frames OR MAX_TIME_TO_WAIT_FOR_FRAMES seconds
while (not wait_frames_timer.has_expired() 
    and n_depth_frame + n_ir_frame < NUMBER_OF_FRAMES_BEFORE_CHECK):
    time.sleep(1)

if wait_frames_timer.has_expired():
    print(str(NUMBER_OF_FRAMES_BEFORE_CHECK) + " frames did not arrived at "+ str(MAX_TIME_TO_WAIT_FOR_FRAMES) + " seconds , abort...")
    test.fail()
else:
    test.check(n_depth_frame >= NUMBER_OF_FRAMES_BEFORE_CHECK)
    test.check_equal(n_ir_frame, 0)

depth_sensor.stop()
depth_sensor.close()

time.sleep(1) # Allow time to ensure no more frame callbacks after stopping sensor

test.finish()

n_depth_frame = 0
n_ir_frame = 0

# Test Part 2
test.start("Ask for depth+IR - make sure both frames arrive")

depth_sensor.open([dp, irp])
depth_sensor.start(frames_counter)
wait_frames_timer.start()

# we wait for first NUMBER_OF_FRAMES_BEFORE_CHECK frames OR MAX_TIME_TO_WAIT_FOR_FRAMES seconds
while (not wait_frames_timer.has_expired() 
    and (n_depth_frame == 0 or n_ir_frame == 0)):
    time.sleep(1)

if wait_frames_timer.has_expired():
    print(str(NUMBER_OF_FRAMES_BEFORE_CHECK) + " frames did not arrived at "+ str(MAX_TIME_TO_WAIT_FOR_FRAMES) + " seconds , abort...")
    test.fail()
else:
    test.check(n_depth_frame != 0)
    test.check(n_ir_frame != 0)

depth_sensor.stop()
depth_sensor.close()

test.finish()

test.print_results_and_exit()