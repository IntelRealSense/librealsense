# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time

def check_depth_dpp_streaming(width, height, fps):
    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth, 0, width, height, rs.format.z16, fps)
    pipe = rs.pipeline()
    pipe.start(cfg)
    iterations = 0
    while iterations < 3:
        iterations += 1
        f = pipe.wait_for_frames()
    test.check_equal(iterations, 3)
    pipe.stop()


################# Checking depth dpp 640x360 streaming with fps 5/15/30/60 ##################
width = 640
height = 360

fps = 5
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

fps = 15
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

fps = 30
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

fps = 60
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()


################# Checking depth dpp 320x180 streaming with fps 5/15/30/60 ##################
width = 320
height = 180

fps = 5
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

fps = 15
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

fps = 30
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

fps = 60
test.start("Checking depth dpp " + repr(width) + "x" + repr(height) + " streaming with fps " + repr(fps))
check_depth_dpp_streaming(width, height, fps)
test.finish()

test.print_results_and_exit()
