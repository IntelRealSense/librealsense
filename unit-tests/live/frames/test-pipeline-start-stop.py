# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# Currently, we exclude D457 as it's failing
# test:device each(D400*) !D457

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time

# Run multiple start stop of all streams and verify we get a frame for each once
ITERATIONS_COUNT = 2

dev = test.find_first_device_or_exit()

def run_and_verify_frame_received():
    pipe = rs.pipeline()
    start_call_stopwatch = Stopwatch()
    pipe.start()
    # wait_for_frames will throw if no frames received so no assert is needed
    f = pipe.wait_for_frames()
    delay = start_call_stopwatch.get_elapsed()
    log.out("After ", delay, " [sec] got first frame of ", f)
    pipe.stop()


################################################################################################
test.start("Testing pipeline start/stop")
for i in range( ITERATIONS_COUNT ):
    log.out("starting iteration #", i + 1, "/", ITERATIONS_COUNT)
    # When we had this line enabled and we used it on "run_and_verify_frame_received" the pipeline failed on second iteration on D455
    # IR frames do not arrive on 2-nd iteration
    # This is investigated in LRS-972 

    #cfg.enable_all_streams()
    run_and_verify_frame_received()
test.finish()

################################################################################################
test.print_results_and_exit()
