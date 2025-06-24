# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import log, test
import numpy as np
import time

NUM_FRAMES = 10 # Number of frames to check
DEPTH_TOLERANCE = 0.05  # Acceptable deviation from expected depth in meters
FRAMES_PASS_THRESHOLD =0.8 # Percentage of frames that needs to pass

# Known pixel positions and expected depth values (in meters)
depth_points = {
    "front": ((538, 315), 0.524 ),
    "middle": ((437, 252), 0.655),
    "back": ((750, 63), 1.058 )
}

test.start("Basic Depth Image Quality Test")

try:
    dev, ctx = test.find_first_device_or_exit()

    pipeline = rs.pipeline(ctx)
    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth, 848, 480, rs.format.z16, 30)
    profile = pipeline.start(cfg)
    frames = pipeline.wait_for_frames()
    time.sleep(2)

    depth_sensor = profile.get_device().first_depth_sensor()
    depth_scale = depth_sensor.get_depth_scale()

    depth_passes = {name: 0 for name in depth_points}

    for i in range(NUM_FRAMES):
        frames = pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        if not depth_frame:
            continue

        depth_image = np.asanyarray(depth_frame.get_data())

        for point_name, ((x, y), expected_depth) in depth_points.items():
            raw_depth = depth_image[y, x]
            depth_value = raw_depth * depth_scale  # Convert to meters

            if abs(depth_value - expected_depth) <= DEPTH_TOLERANCE:
                depth_passes[point_name] += 1
            else:
                log.d(f"Frame {i} - {point_name} at ({x},{y}): {depth_value:.3f}m ≠ {expected_depth:.3f}m")

    # Check that each point passed the threshold
    min_passes = int(NUM_FRAMES * FRAMES_PASS_THRESHOLD)
    for point_name, count in depth_passes.items():
        log.i(f"{point_name.title()} passed in {count}/{NUM_FRAMES} frames")
        test.check(count >= min_passes)

except Exception as e:
    test.unexpected_exception(e)

pipeline.stop()
test.finish()
test.print_results_and_exit()
