# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*
# test:donotrun:!nightly

import pyrealsense2 as rs
from rspy import test, log
import json
from hdr_helper import load_and_perform_test, HDR_CONFIGURATIONS, MANUAL_HDR_CONFIG_1

# Different depth resolutions to test
DEPTH_RESOLUTIONS = [
    (640, 480),
    (848, 480),
    (1280, 720),
]

# Test each configuration with different resolutions
for i, config in enumerate(HDR_CONFIGURATIONS):
    config_type = "Auto" if "depth-ae" in json.dumps(config) else "Manual"
    num_items = len(config["hdr-preset"]["items"])
    for resolution in DEPTH_RESOLUTIONS:
        resolution_name = f"{resolution[0]}x{resolution[1]}"
        test_name = f"Config {i + 1} ({config_type}, {num_items} items) @ {resolution_name}"
        load_and_perform_test(config, test_name, resolution)


def test_disable_auto_hdr():
    """
    Test disabling Auto-HDR and returning to default behavior
    """
    device, ctx = test.find_first_device_or_exit()
    am = rs.rs400_advanced_mode(device)
    sensor = device.first_depth_sensor()

    cfg = rs.config()
    pipe = rs.pipeline(ctx)
    with test.closure("Disable Auto-HDR - Return to default behavior"):
        # First enable HDR
        am.load_json(json.dumps(MANUAL_HDR_CONFIG_1))
        test.check(sensor.get_option(rs.option.hdr_enabled) == 1)

        # Disable HDR
        sensor.set_option(rs.option.hdr_enabled, 0)
        test.check(sensor.get_option(rs.option.hdr_enabled) == 0)

        # Verify we're back to default single-frame behavior
        cfg.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
        pipe.start(cfg)

        for _ in range(30):
            data = pipe.wait_for_frames()
            depth_frame = data.get_depth_frame()

            # In default mode, sequence size should be 0
            seq_size = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_size)
            test.check(seq_size == 0, f"Expected sequence size 0 in default mode, got {seq_size}")

            # Sequence ID should always be 0 in single-frame mode
            seq_id = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
            test.check(seq_id == 0, f"Expected sequence ID 0 in default mode, got {seq_id}")

        pipe.stop()
        cfg.disable_all_streams()


# Test disabling HDR
test_disable_auto_hdr()

test.print_results_and_exit()
