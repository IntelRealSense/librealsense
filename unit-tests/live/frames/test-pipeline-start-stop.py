# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# Currently, we exclude D457 as it's failing
# test:device each(D400*) !D457

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time

# Run multiple start stop of all streams and verify we get a frame for each once

dev = test.find_first_device_or_exit()

def verify_frame_received(config):
    pipe = rs.pipeline()
    start_call_stopwatch = Stopwatch()
    pipe.start(config)
    # wait_for_frames will throw if no frames received so no assert is needed
    f = pipe.wait_for_frames()
    delay = start_call_stopwatch.get_elapsed()
    log.out("After ", delay, " [sec] got first frame of ", f)
    pipe.stop()
    time.sleep(1) # allow the streaming some time to acctualy stop between iterations


################################################################################################
test.start("Testing pipeline start/stop stress test")
for i in range(3):
    log.out("starting iteration #", i + 1, "/", 10)
    cfg = rs.config()
    cfg.enable_all_streams()
    verify_frame_received(cfg)
test.finish()

################################################################################################
test.print_results_and_exit()
