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


################# Checking depth dpp streaming with res 640x360 / 320x180 and fps 5/15/30/60 ##################

res_arr = [[640, 360], [320, 180]]
fps_arr = [5, 15, 30, 60]

for res in res_arr:
    for fps in fps_arr:
        test.start("Checking depth dpp " + repr(res[0]) + "x" + repr(res[1]) + " streaming with fps " + repr(fps))
        check_depth_dpp_streaming(res[0], res[1], fps)
        test.finish()

test.print_results_and_exit()
