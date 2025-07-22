# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device D400*

import pyrealsense2 as rs
from rspy import test

#############################################################################################
# get metadata depth units value and make sure it's non zero and equal to the depth sensor matching option value
test.start("checking depth units on metadata")

dev, ctx = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()

try:
    cfg = pipeline = None
    pipeline = rs.pipeline(ctx)
    cfg = rs.config()
    pipeline_profile = pipeline.start(cfg)

    # Check that depth units on meta data is non zero
    frame_set = pipeline.wait_for_frames()
    depth_frame = frame_set.get_depth_frame()
    depth_units_from_metadata = depth_frame.get_units()
    test.check(depth_units_from_metadata > 0)

    # Check metadata depth unit value match option value
    dev = pipeline_profile.get_device()
    ds = dev.first_depth_sensor()
    test.check(ds.supports(rs.option.depth_units))
    test.check_equal(ds.get_option(rs.option.depth_units), depth_units_from_metadata)

    pipeline.stop()

except Exception:
    test.unexpected_exception()

test.finish()
test.print_results_and_exit()
