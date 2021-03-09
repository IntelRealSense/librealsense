# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device L500*
# test:device D400*

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test
import time
import platform

# Start depth + color streams and measure the time from stream opened until first frame arrived.
# Verify that the time do not exceeds the maximum time allowed
MAX_TIME_TO_WAIT_FOR_FIRST_FRAME = 2.5  # [sec]
open_call_stopwatch = Stopwatch()
first_frame_time = None


def time_to_first_frame(sensor, profile):
    global first_frame_time
    first_frame_time = MAX_TIME_TO_WAIT_FOR_FIRST_FRAME

    def frame_cb(frame):
        global first_frame_time
        if first_frame_time == MAX_TIME_TO_WAIT_FOR_FIRST_FRAME:
            global open_call_stopwatch
            first_frame_time = open_call_stopwatch.get_elapsed()

    open_call_stopwatch.reset()
    sensor.open(profile)
    sensor.start(frame_cb)

    while first_frame_time == MAX_TIME_TO_WAIT_FOR_FIRST_FRAME and open_call_stopwatch.get_elapsed() < MAX_TIME_TO_WAIT_FOR_FIRST_FRAME:
        time.sleep(0.05)

    sensor.stop()
    sensor.close()

    return first_frame_time


dev = test.find_first_device_or_exit()

ds = dev.first_depth_sensor()
cs = dev.first_color_sensor()

dp = next(p for p in
          ds.profiles if p.fps() == 30
          and p.stream_type() == rs.stream.depth
          and p.format() == rs.format.z16)

cp = next(p for p in
          cs.profiles if p.fps() == 30
          and p.stream_type() == rs.stream.color
          and p.format() == rs.format.rgb8)

test.start("Testing first depth frame delay on " + platform.system())
first_depth_frame_delay = time_to_first_frame(ds, dp)
print("Time until first depth frame is: {:.3f} [sec]".format(first_depth_frame_delay))
test.check(first_depth_frame_delay < MAX_TIME_TO_WAIT_FOR_FIRST_FRAME)
test.finish()

test.start("Testing first color frame delay on " + platform.system())
first_color_frame_delay = time_to_first_frame(cs, cp)
print("Time until first color frame is: {:.3f} [sec]".format(first_color_frame_delay))
test.check(first_color_frame_delay < MAX_TIME_TO_WAIT_FOR_FIRST_FRAME)
test.finish()
