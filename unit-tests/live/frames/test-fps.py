# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device L500*
# test:device D400*
# test:donotrun:!nightly
# test:timeout 230

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
    seconds_to_count_frames = 20
    
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

    steady_state = True
    fps_stopwatch.reset()
    
    time.sleep(seconds_to_count_frames) # Time to count frames
    
    steady_state = False # Stop counting    
    
    sensor.stop()
    sensor.close()

    fps = frames_received / seconds_to_count_frames
    return fps


delta_Hz = 1
tested_fps = [6, 15, 30, 60, 90]

dev = test.find_first_device_or_exit()
product_line = dev.get_info(rs.camera_info.product_line)

#####################################################################################################
test.start("Testing depth fps " + product_line + " device - "+ platform.system() + " OS")

for requested_fps in tested_fps:
    ds = dev.first_depth_sensor()
    #Set auto-exposure option as it might take precedence over requested FPS
    if product_line == "D400":
        ds.set_option(rs.option.enable_auto_exposure, 1)

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
        test.check(fps <= (requested_fps + delta_Hz) and fps >= (requested_fps - delta_Hz))
test.finish()


#####################################################################################################
test.start("Testing color fps " + product_line + " device - "+ platform.system() + " OS")

for requested_fps in tested_fps:
    cs = dev.first_color_sensor()
    #Set auto-exposure option as it might take precedence over requested FPS
    if product_line == "D400":
        ds.set_option(rs.option.enable_auto_exposure, 1)
    elif product_line == "L500":
        cs.set_option(rs.option.enable_auto_exposure, 0)

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
        test.check(fps <= (requested_fps + delta_Hz) and fps >= (requested_fps - delta_Hz))

test.finish()

#####################################################################################################
test.print_results_and_exit()
