# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time

# Constants
FRAMES_TO_COLLECT = 3

def check_depth_and_ir_streaming(fps):
    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth, 0, 1280, 720, rs.format.z16, fps)
    cfg.enable_stream(rs.stream.infrared, 1, 1280, 720, rs.format.y8, fps)
    pipe = rs.pipeline()
    pipe.start(cfg)
    iterations = 0
    while iterations < FRAMES_TO_COLLECT:
        iterations += 1
        f = pipe.wait_for_frames()
    test.check_equal(iterations, FRAMES_TO_COLLECT)
    pipe.stop()


################# Checking depth and IR streaming with fps 5/15/30/60 ##################

fps_arr = [5, 15, 30, 60]

for fps in fps_arr:
    test.start("Checking depth and IR streaming with fps " + repr(fps))
    check_depth_and_ir_streaming(fps)
    test.finish()

test.print_results_and_exit()
