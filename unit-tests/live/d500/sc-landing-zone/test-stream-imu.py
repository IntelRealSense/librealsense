# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S
# test not running until HSD is solved: https://hsdes.intel.com/appstore/article/#/13010957807
import pyrealsense2 as rs
from rspy import test, log
import time

# Constants
FRAMES_TO_COLLECT = 3

def check_imu_streaming(stream, fps):
    cfg = rs.config()
    cfg.enable_stream(stream, 0, rs.format.motion_xyz32f, fps)
    pipe = rs.pipeline()
    pipe.start(cfg)
    iterations = 0
    while iterations < FRAMES_TO_COLLECT:
        iterations += 1
        f = pipe.wait_for_frames()
    test.check_equal(iterations, FRAMES_TO_COLLECT)
    pipe.stop()


time.sleep(5)  # due to a WIP bug, we have to wait after enumeration for a few secs before Accel stream
################# Checking accel streaming with fps 100/200 ##################
stream = rs.stream.accel

fps = 100
test.start("Checking " + repr(stream) + " streaming with fps " + repr(fps))
check_imu_streaming(stream, fps)
test.finish()

fps = 200
test.start("Checking " + repr(stream) + " streaming with fps " + repr(fps))
check_imu_streaming(stream, fps)
test.finish()


################# Checking gyro streaming with fps 200/400 ##################
stream = rs.stream.gyro

fps = 200
test.start("Checking " + repr(stream) + " streaming with fps " + repr(fps))
check_imu_streaming(stream, fps)
test.finish()

fps = 400
test.start("Checking " + repr(stream) + " streaming with fps " + repr(fps))
check_imu_streaming(stream, fps)
test.finish()

test.print_results_and_exit()
