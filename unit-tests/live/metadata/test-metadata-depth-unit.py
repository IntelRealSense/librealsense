# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*
#test:device SR300*

import pyrealsense2 as rs
from rspy import test

#############################################################################################
test.start("checking depth units on metadata")

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()

try:
    cfg = pipeline = None
    pipeline = rs.pipeline()
    cfg = rs.config()
    pipeline.start(cfg)

    frame_set = pipeline.wait_for_frames()
    depth_frame = frame_set.get_depth_frame()
    test.check(depth_frame != None, abort_if_failed=True)
    test.check(depth_frame.get_units() != 0)

    pipeline.stop()

except Exception:
    test.unexpected_exception()

test.finish()
test.print_results_and_exit()
