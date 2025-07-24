# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy import test, log

# This test is checking that timestamps of depth and infrared frames are consistent

# Tolerance for gap between depth and color
GLOBAL_TS_TOLERANCE = 1  # in ms
FRAME_TS_TOLERANCE = 20  # in microseconds
with test.closure("Test Timestamps Consistency"):
    device, ctx = test.find_first_device_or_exit()
    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth)
    cfg.enable_stream(rs.stream.infrared, 1)
    cfg.enable_stream(rs.stream.infrared, 2)
    cfg.enable_stream(rs.stream.color)

    pipe = rs.pipeline(ctx)
    pipe.start(cfg)

    try:
        for _ in range(50):
            frames = pipe.wait_for_frames()
            depth_frame = frames.get_depth_frame()
            ir1_frame = frames.get_infrared_frame(1)
            ir2_frame = frames.get_infrared_frame(2)
            color_frame = frames.get_color_frame()

            if not (depth_frame and ir1_frame and ir2_frame):
                log.e("One or more frames are missing")
                continue

            # Global timestamps
            depth_ts = depth_frame.timestamp
            ir1_ts = ir1_frame.timestamp
            ir2_ts = ir2_frame.timestamp
            color_ts = color_frame.timestamp

            log.d(f"Depth Global TS: {depth_ts}, IR1 Global TS: {ir1_ts}, IR2 Global TS: {ir2_ts}")
            log.d(f"Color Global TS: {color_ts}")

            # Check global timestamps
            test.check(depth_ts == ir1_ts)
            test.check(depth_ts == ir2_ts)
            test.check_approx_abs(depth_ts, color_ts, GLOBAL_TS_TOLERANCE)

            # Frame metadata timestamps
            if all(frame.supports_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                   for frame in [depth_frame, ir1_frame, ir2_frame, color_frame]):
                # if some frame does not support frame timestamp metadata, skip the check

                depth_frame_ts = depth_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                ir1_frame_ts = ir1_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                ir2_frame_ts = ir2_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                color_frame_ts = color_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)

                log.d(f"Depth Frame TS: {depth_frame_ts}, IR1 Frame TS: {ir1_frame_ts}, IR2 Frame TS: {ir2_frame_ts}")
                log.d(f"Color Frame TS: {color_frame_ts}")

                # Check frame timestamps
                test.check(depth_frame_ts == ir1_frame_ts)
                test.check(depth_frame_ts == ir2_frame_ts)
                test.check_approx_abs(depth_frame_ts, color_frame_ts, FRAME_TS_TOLERANCE)
            else:
                log.d("One or more frames do not support frame timestamp metadata")

    finally:
        pipe.stop()

test.print_results_and_exit()