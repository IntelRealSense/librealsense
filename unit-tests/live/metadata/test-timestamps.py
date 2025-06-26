# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy import test, log

# This test is checking that timestamps of depth and infrared frames are consistent

with test.closure("Test Timestamps Consistency"):
    device, ctx = test.find_first_device_or_exit()
    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth)
    cfg.enable_stream(rs.stream.infrared, 1)
    cfg.enable_stream(rs.stream.infrared, 2)

    pipe = rs.pipeline(ctx)
    pipe.start(cfg)

    try:
        for _ in range(50):
            frames = pipe.wait_for_frames()
            depth_frame = frames.get_depth_frame()
            ir1_frame = frames.get_infrared_frame(1)
            ir2_frame = frames.get_infrared_frame(2)

            if not (depth_frame and ir1_frame and ir2_frame):
                log.e("One or more frames are missing")
                continue

            # Global timestamps
            depth_ts = depth_frame.timestamp
            ir1_ts = ir1_frame.timestamp
            ir2_ts = ir2_frame.timestamp

            # Frame timestamps
            depth_frame_ts = depth_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
            ir1_frame_ts = ir1_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
            ir2_frame_ts = ir2_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)

            log.d(f"Depth TS: {depth_ts}, IR1 TS: {ir1_ts}, IR2 TS: {ir2_ts}")
            log.d(f"Depth Frame TS: {depth_frame_ts}, IR1 Frame TS: {ir1_frame_ts}, IR2 Frame TS: {ir2_frame_ts}")

            # Check global timestamps
            test.check(depth_ts == ir1_ts)
            test.check(depth_ts == ir2_ts)

            # Check frame timestamps
            test.check(depth_frame_ts == ir1_frame_ts)
            test.check(depth_frame_ts == ir2_frame_ts)

    finally:
        pipe.stop()

test.print_results_and_exit()