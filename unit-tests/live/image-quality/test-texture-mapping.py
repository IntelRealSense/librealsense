# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*
# test:donotrun
import pyrealsense2 as rs
from rspy import log, test
import numpy as np
import time


NUM_FRAMES = 10 # Number of frames to check
COLOR_TOLERANCE = 20  # Acceptable per-channel deviation in RGB values
FRAMES_PASS_THRESHOLD =0.8 # Percentage of frames that needs to pass

# Pixel locations in the depth frame and their expected RGB color after texture mapping
expected_mappings = {
    "orange": ((878, 444), (107, 22, 4)),
    "red":    ((766, 476), (84, 8, 5)),
    "green":  ((646,  437), (13, 26, 11))
}

def is_color_close(actual_rgb, expected_rgb, tolerance):
    return all(abs(int(a) - int(e)) <= tolerance for a, e in zip(actual_rgb, expected_rgb))


test.start("Texture Mapping Test")

try:
    dev, ctx = test.find_first_device_or_exit()
    pipeline = rs.pipeline(ctx)
    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
    cfg.enable_stream(rs.stream.color, 1280, 720, rs.format.rgb8, 30)
    profile = pipeline.start(cfg)
    frames = pipeline.wait_for_frames()
    time.sleep(2)

    align = rs.align(rs.stream.color)
    color_pass_count = {name: 0 for name in expected_mappings}

    for frame_index in range(NUM_FRAMES):
        frames = pipeline.wait_for_frames()
        aligned_frames = align.process(frames)
        depth_frame = aligned_frames.get_depth_frame()
        color_frame = aligned_frames.get_color_frame()

        if not depth_frame or not color_frame:
            continue

        color_image = np.asanyarray(color_frame.get_data())

        for name, ((x, y), expected_rgb) in expected_mappings.items():
            depth = depth_frame.get_distance(x, y)
            if depth == 0:
                continue

            if 0 <= x < color_image.shape[1] and 0 <= y < color_image.shape[0]:
                pixel_rgb = color_image[y, x]
                if is_color_close(pixel_rgb, expected_rgb, COLOR_TOLERANCE):
                    color_pass_count[name] += 1
                else:
                    log.d(f"Frame {frame_index} - {name}: Color {pixel_rgb} ≠ expected {expected_rgb} at ({x},{y})")
            else:
                log.w(f"Frame {frame_index} - {name}: Projected pixel ({x},{y}) is out of bounds")

    # Check per-color pass threshold
    min_passes = int(NUM_FRAMES * FRAMES_PASS_THRESHOLD)
    for name, count in color_pass_count.items():
        log.i(f"{name.title()} passed in {count}/{NUM_FRAMES} frames")
        test.check(count >= min_passes, f"{name.title()} failed in too many frames")

except Exception as e:
    test.unexpected_exception(e)

pipeline.stop()
test.finish()
test.print_results_and_exit()
