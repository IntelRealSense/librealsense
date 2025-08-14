# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*
# test:donotrun:!nightly

import pyrealsense2 as rs
from rspy import test, log
import json, time
from hdr_helper import test_json_load, HDR_CONFIGURATIONS


device, ctx = test.find_first_device_or_exit()
am = rs.rs400_advanced_mode(device)
sensor = device.first_depth_sensor()

count = 0
count_frames = False

EXPECTED_FPS = 30
ACCEPTABLE_FPS = EXPECTED_FPS * 0.9
TIME_FOR_STEADY_STATE = 3
TIME_TO_COUNT_FRAMES = 5


def frame_counter_callback(frame):
    global count, count_frames
    if not count_frames:
        return  # Skip counting if not enabled
    count += 1
    log.d("Frame callback called, frame number:", frame.get_frame_number())


def test_hdr_performance():
    """
    Test HDR performance with various configurations
    """
    for i, config in enumerate(HDR_CONFIGURATIONS):
        config_type = "Auto" if "depth-ae" in json.dumps(config) else "Manual"
        num_items = len(config["hdr-preset"]["items"])
        test_name = f"Config {i + 1} ({config_type}, {num_items} items)"
        test_json_load(config, test_name)

        global count, count_frames
        count = 0
        depth_profile = next(p for p in sensor.get_stream_profiles() if p.stream_type() == rs.stream.depth)
        sensor.open(depth_profile)
        sensor.start(frame_counter_callback)

        time.sleep(TIME_FOR_STEADY_STATE)
        count_frames = True  # Start counting frames
        time.sleep(TIME_TO_COUNT_FRAMES)
        count_frames = False  # Stop counting

        sensor.stop()
        sensor.close()

        measured_fps = count / TIME_TO_COUNT_FRAMES
        log.d(f"Test {test_name}: Counted frames = {count}, Measured FPS = {measured_fps:.2f}")
        test.check(measured_fps > ACCEPTABLE_FPS, f"Measured FPS {measured_fps:.2f}")


test_hdr_performance()

test.print_results_and_exit()
