# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time
import platform


# Start depth + color streams and measure the time from stream opened until first frame arrived using pipeline API.
# Verify that the time does not exceed the maximum time allowed
# Note - Using Windows Media Foundation to handle power management between USB actions take time (~27 ms)


# Set maximum delay for first frame according to product line
dev = test.find_first_device_or_exit()

def verify_frame_received(config):
    pipe = rs.pipeline()
    start_call_stopwatch = Stopwatch()
    pipe.start(config)
    # wait_for_frames will through if no frames received so no assert is needed
    f = pipe.wait_for_frames()
    delay = start_call_stopwatch.get_elapsed()
    log.out("After ", delay, " [sec] got first frame of ", f)
    pipe.stop()


################################################################################################
test.start("Testing pipeline start/stop stress test")
for i in range(10):
    log.out("starting iteration #", i + 1, "/", 10)
    cfg = rs.config()
    cfg.enable_all_streams()
    verify_frame_received(cfg)
test.finish()

################################################################################################
test.print_results_and_exit()
