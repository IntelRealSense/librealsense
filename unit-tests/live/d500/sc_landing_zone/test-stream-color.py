# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time


def check_color_streaming(width, height, fps):
    cfg = rs.config()
    cfg.enable_stream(rs.stream.color, 0, width, height, rs.format.m420, fps)
    pipe = rs.pipeline()
    pipe.start(cfg)
    iterations = 0
    while iterations < 3:
        iterations += 1
        f = pipe.wait_for_frames()
    test.check_equal(iterations, 3)
    pipe.stop()


################# Checking color 640x360 streaming with fps 5/15/30 ##################
width = 640
height = 360

fps_arr = [5, 15, 30]

for fps in fps_arr:
    test.start("Checking color " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
    check_color_streaming(width, height, fps)
    test.finish()


################# Checking color 320x180 streaming with fps 5/15/30 ##################
width = 1280
height = 720

fps_arr = [5, 15, 30]
for fps in fps_arr:
    test.start("Checking color " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
    check_color_streaming(width, height, fps)
    test.finish()

test.print_results_and_exit()
