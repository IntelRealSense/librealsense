# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time

# Constants
FRAMES_TO_COLLECT = 3

def check_sc_streaming(stream):
    cfg = rs.config()
    cfg.enable_stream(stream)
    pipe = rs.pipeline()
    pipe.start(cfg)
    iterations = 0
    while iterations < FRAMES_TO_COLLECT:
        iterations += 1
        f = pipe.wait_for_frames()
    test.check_equal(iterations, FRAMES_TO_COLLECT)
    pipe.stop()


################# Checking safety streaming ##################

stream = rs.stream.safety
test.start("Checking " + repr(stream) + " stream")
check_sc_streaming(stream)
test.finish()

################# Checking occupancy streaming ##################

stream = rs.stream.occupancy
test.start("Checking " + repr(stream) + " stream")
check_sc_streaming(stream)
test.finish()

################# Checking lpc streaming ##################

stream = rs.stream.labeled_point_cloud
test.start("Checking " + repr(stream) + " stream")
check_sc_streaming(stream)
test.finish()

test.print_results_and_exit()
