# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D400*
# test:device D500*

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test
import time

# Verify IR calibration format Y16i is supported

frame_received = False

def frame_cb(frame):
    global frame_received
    test.check_equal(frame.get_profile().format(), rs.format.y16)
    frame_received = True;

#####################################################################################################
test.start("Verify supported formats")
dev = test.find_first_device_or_exit()
ds = dev.first_depth_sensor()

for profile in ds.profiles:
    

    
calib_profile = next(p for p in
          ds.profiles if p.fps() == 30
          and p.stream_type() == rs.stream.infrared
          and p.format() == rs.format.y12i)
          
sensor.open(profile)
sensor.start(frame_cb)

first_frame_sw = Stopwatch()

while not frame_received and first_frame_sw.get_elapsed() < 5:
    time.sleep(0.05)

test.check_equal(frame_received)

sensor.stop()
sensor.close()
    
test.finish()

#####################################################################################################
test.print_results_and_exit()
