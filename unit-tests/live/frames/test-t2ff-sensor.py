# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time
import platform

# Start depth + color streams and measure the time from stream opened until first frame arrived using sensor API.
# Verify that the time do not exceeds the maximum time allowed
# Note - Using Windows Media Foundation to handle power management between USB actions take time (~27 ms)


def time_to_first_frame(sensor, profile, max_delay_allowed):
    """
    Wait for the first frame for 'max_delay_allowed' + 1 extra second
    If the frame arrives it will return the seconds it took since open() call
    If no frame it will return 'max_delay_allowed'
    """
    first_frame_time = max_delay_allowed
    open_call_stopwatch = Stopwatch()

    def frame_cb(frame):
        nonlocal first_frame_time, open_call_stopwatch
        if first_frame_time == max_delay_allowed:
            first_frame_time = open_call_stopwatch.get_elapsed()

    open_call_stopwatch.reset()
    sensor.open(profile)
    sensor.start(frame_cb)

    # Wait condition:
    # 1. first frame did not arrive yet
    # 2. timeout of 'max_delay_allowed' + 1 extra second reached.
    while first_frame_time == max_delay_allowed and open_call_stopwatch.get_elapsed() < max_delay_allowed + 1:
        time.sleep(0.05)

    sensor.stop()
    sensor.close()

    return first_frame_time


# The device starts at D0 (Operational) state, allow time for it to get into idle state
time.sleep(3)


#####################################################################################################
test.start("Testing device creation time on " + platform.system() + " OS")
device_creation_stopwatch = Stopwatch()
dev = test.find_first_device_or_exit()
device_creation_time = device_creation_stopwatch.get_elapsed()
max_time_for_device_creation = 1
print("Device creation time is: {:.3f} [sec] max allowed is: {:.1f} [sec] ".format(device_creation_time, max_time_for_device_creation))
test.check(device_creation_time < max_time_for_device_creation)
test.finish()


# Set maximum delay for first frame according to product line
product_line = dev.get_info(rs.camera_info.product_line)
if product_line == "D400":
    max_delay_for_depth_frame = 1
    max_delay_for_color_frame = 1
else:
    log.f( "This test support only D400 devices" )


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


#####################################################################################################
test.start("Testing first depth frame delay on " + product_line + " device - "+ platform.system() + " OS")
first_depth_frame_delay = time_to_first_frame(ds, dp, max_delay_for_depth_frame)
print("Time until first depth frame is: {:.3f} [sec] max allowed is: {:.1f} [sec] ".format(first_depth_frame_delay, max_delay_for_depth_frame))
test.check(first_depth_frame_delay < max_delay_for_depth_frame)
test.finish()


#####################################################################################################
test.start("Testing first color frame delay on " + product_line + " device - "+ platform.system() + " OS")
first_color_frame_delay = time_to_first_frame(cs, cp, max_delay_for_color_frame)
print("Time until first color frame is: {:.3f} [sec] max allowed is: {:.1f} [sec] ".format(first_color_frame_delay, max_delay_for_color_frame))
test.check(first_color_frame_delay < max_delay_for_color_frame)
test.finish()


#####################################################################################################
test.print_results_and_exit()
