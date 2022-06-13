# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device L500*
# test:device D400*
# test:donotrun:!nightly

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time
import platform

# Start depth + color streams and measure frame frequency using sensor API.
# Verify that actual fps is as requested

def measure_fps(sensor, profile):
    """
    Wait for 2 seconds to be sure that frames are at steady state after start
    Count number of received frames for 5 seconds and compare actual fps to requested fps
    """
    seconds_till_steady_state = 2
    seconds_to_count_frames = 5
    
    steady_state = False
    frames_received = 0
    fps_stopwatch = Stopwatch()

    def frame_cb(frame):
        nonlocal steady_state, frames_received
        if steady_state:
            frames_received += 1

    sensor.open(profile)
    sensor.start(frame_cb)

    time.sleep(seconds_till_steady_state)

    fps_stopwatch.reset()
    steady_state = True
    
    time.sleep(seconds_to_count_frames) # Time to count frames
    
    steady_state = False # Stop counting    
    
    sensor.stop()
    sensor.close()

    fps = frames_received / seconds_to_count_frames
    return fps


acceptable_exception_rate_Hz = 0.5
tested_fps = [6, 15, 30, 60, 90]

dev = test.find_first_device_or_exit()
product_line = dev.get_info(rs.camera_info.product_line)

#####################################################################################################
test.start("Testing depth fps " + product_line + " device - "+ platform.system() + " OS")

for requested_fps in tested_fps:
    ds = dev.first_depth_sensor()
    try:
        dp = next(p for p in ds.profiles
                  if p.fps() == requested_fps 
                  and p.stream_type() == rs.stream.depth
                  and p.format() == rs.format.z16)
    except StopIteration:
        print("Requested fps: {:.1f} [Hz], not supported".format(requested_fps))
    else:
        fps = measure_fps(ds, dp)
        print("Requested fps: {:.1f} [Hz], actual fps: {:.1f} [Hz] ".format(requested_fps, fps))
        test.check(fps <= (requested_fps + acceptable_exception_rate_Hz) and fps >= (requested_fps - acceptable_exception_rate_Hz))
test.finish()


#####################################################################################################
test.start("Testing color fps " + product_line + " device - "+ platform.system() + " OS")

for requested_fps in tested_fps:
    dev = test.find_first_device_or_exit()
    cs = dev.first_color_sensor()
    try:
        cp = next(p for p in cs.profiles
                  if p.fps() == requested_fps
                  and p.stream_type() == rs.stream.color
                  and p.format() == rs.format.rgb8)
    except StopIteration:
        print("Requested fps: {:.1f} [Hz], not supported".format(requested_fps))
    else:
        fps = measure_fps(cs, cp)
        print("Requested fps: {:.1f} [Hz], actual fps: {:.1f} [Hz] ".format(requested_fps, fps))
        test.check(fps <= (requested_fps + acceptable_exception_rate_Hz) and fps >= (requested_fps - acceptable_exception_rate_Hz))

test.finish()

#####################################################################################################
test.print_results_and_exit()
