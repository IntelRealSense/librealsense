# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*
# test:donotrun
import pyrealsense2 as rs
from rspy import log, test
import numpy as np
import time

NUM_FRAMES = 10 # Number of frames to check
COLOR_TOLERANCE = 20 # Acceptable per-channel deviation in RGB values
FRAMES_PASS_THRESHOLD =0.8 # Percentage of frames that needs to pass

# Known pixel positions and expected RGB values
color_points = {
    "orange": ((920, 400), (116, 38, 15)),
    "red":    ((800, 500), (105, 13, 9)),
    "green":  ((700, 400), (14, 21, 9))
}

def is_color_close(actual, expected, tolerance):
    return all(abs(int(a) - int(e)) <= tolerance for a, e in zip(actual, expected))

test.start("Basic Color Image Quality Test")

try:
    dev, ctx = test.find_first_device_or_exit()

    pipeline = rs.pipeline(ctx)
    cfg = rs.config()
    cfg.enable_stream(rs.stream.color, 1280, 720, rs.format.rgb8, 30)
    pipeline_profile = pipeline.start(cfg)
    frames = pipeline.wait_for_frames()
    time.sleep(2)

    color_match_count = {name: 0 for name in color_points}

    for i in range(NUM_FRAMES):
        frames = pipeline.wait_for_frames()
        color_frame = frames.get_color_frame()
        image = np.asanyarray(color_frame.get_data())

        for color, (pos, expected_rgb) in color_points.items():
            x, y = pos
            pixel = image[y, x]
            if is_color_close(pixel, expected_rgb, COLOR_TOLERANCE):
                color_match_count[color] += 1
            else:
                log.d(f"Frame {i} - {color} at ({x},{y}): {pixel} ≠ {expected_rgb}")

    # Check per-color pass threshold
    min_passes = int(NUM_FRAMES * FRAMES_PASS_THRESHOLD)
    for color_name, count in color_match_count.items():
        log.i(f"{color_name.title()} passed in {count}/{NUM_FRAMES} frames")
        test.check(count >= min_passes)

except Exception as e:
    test.unexpected_exception(e)

pipeline.stop()
test.finish()
test.print_results_and_exit()