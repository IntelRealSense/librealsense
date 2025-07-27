# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test, log
import json


def to_signed(value):
    """Convert an unsigned integer to a signed integer."""
    value = int(value)
    if value & 0x80000000:
        value -= 0x100000000
    return str(value)


MANUAL_HDR_CONFIG = {
    "hdr-preset": {
        "id": "0",
        "iterations": "0",
        "items": [
            {"iterations": "1", "controls": {"depth-gain": "16", "depth-exposure": "1"}},
            {"iterations": "2", "controls": {"depth-gain": "61", "depth-exposure": "10"}},
            {"iterations": "1", "controls": {"depth-gain": "116", "depth-exposure": "100"}},
            {"iterations": "3", "controls": {"depth-gain": "161", "depth-exposure": "1000"}},
            {"iterations": "1", "controls": {"depth-gain": "22", "depth-exposure": "10000"}},
            {"iterations": "2", "controls": {"depth-gain": "222", "depth-exposure": "4444"}},
        ]
    }
}

AUTO_HDR_CONFIG = {
    "hdr-preset": {
        "id": "0",
        "iterations": "0",
        "items": [
            {"iterations": "1", "controls": {"depth-ae": "1"}},
            {"iterations": "2", "controls": {"depth-ae-exp": "2000", "depth-ae-gain": "30"}},
            {"iterations": "2", "controls": {"depth-ae-exp": "-2000", "depth-ae-gain": "20"}},
            {"iterations": "3", "controls": {"depth-ae-exp": "3000", "depth-ae-gain": "10"}},
            {"iterations": "3", "controls": {"depth-ae-exp": "-3000", "depth-ae-gain": "40"}},
        ]
    }
}

device, ctx = test.find_first_device_or_exit()
am = rs.rs400_advanced_mode(device)
sensor = device.first_depth_sensor()
batch_size = 0  # updated on test_json_load

cfg = rs.config()
cfg.enable_stream(rs.stream.depth)
pipe = rs.pipeline(ctx)


def test_json_load(config, test_title):
    global batch_size
    with test.closure(test_title):
        prior_config = json.loads(am.serialize_json())
        am.load_json(json.dumps(config))  # json dumps just stringify the object
        hdr_config_read = json.loads(am.serialize_json())  # jsonify, note this JSON contains also other keys (params)

        # go over the read data and convert all values to signed integers
        batch_size = 0
        for key in hdr_config_read["hdr-preset"]["items"]:
            batch_size += int(key["iterations"])
            for control_key in key["controls"].keys():
                key["controls"][control_key] = to_signed(key["controls"][control_key])
        test.check(hdr_config_read.get("hdr-preset") == config.get("hdr-preset"))

        # check that the other keys are unchanged
        for key in hdr_config_read.keys():
            if key != "hdr-preset":
                test.check(hdr_config_read[key] == prior_config[key])

        test.check(sensor.get_option(rs.option.hdr_enabled) == 1)


test_json_load(MANUAL_HDR_CONFIG, "Manual mode: Loading HDR preset config")

# expected that i == sequence id
with test.closure("Manual mode: Checking streaming data matches config"):
    expected_iterations = {}
    for idx, item in enumerate(MANUAL_HDR_CONFIG["hdr-preset"]["items"]):
        expected_iterations[idx] = int(item["iterations"])

    seq_id_counts = {}
    pipe.start(cfg)

    log.d(f"Batch size: {batch_size}")
    for i in range(0, batch_size * 5):
        data = pipe.wait_for_frames()
        depth_frame = data.get_depth_frame()
        frame_number = depth_frame.get_frame_number()

        if i < batch_size:  # skip the first batch of frames
            continue

        seq_id         = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
        frame_exposure = depth_frame.get_frame_metadata(rs.frame_metadata_value.actual_exposure)
        frame_gain     = depth_frame.get_frame_metadata(rs.frame_metadata_value.gain_level)
        seq_size       = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_size)

        seq_id_counts[seq_id] = seq_id_counts.get(seq_id, 0) + 1
        log.d(f"Frame {frame_number} - Sequence ID: {seq_id}, Exposure: {frame_exposure}, Gain: {frame_gain}")

        current_controls = MANUAL_HDR_CONFIG["hdr-preset"]["items"][seq_id]["controls"]
        expected_exposure = int(current_controls["depth-exposure"])
        expected_gain = int(current_controls["depth-gain"])

        test.check(frame_exposure == expected_exposure, f"Exposure - Expected: {expected_exposure}, Actual: {frame_exposure}")
        test.check(frame_gain == expected_gain, f"Gain - Expected: {expected_gain}, Actual: {frame_gain}")
        test.check(seq_size == len(MANUAL_HDR_CONFIG["hdr-preset"]["items"]))

        if i % batch_size == batch_size - 1:
            test.check(seq_id_counts == expected_iterations,
                       f"Sequence ID counts do not match expected: {seq_id_counts} != {expected_iterations}")
            seq_id_counts.clear()

            log.d("----")

    pipe.stop()

test_json_load(AUTO_HDR_CONFIG, "Auto mode: Loading HDR preset config")

seq_id_0_exp = 0
seq_id_0_gain = 0
# TODO: take claude's expected_iterations thingy and make it on manual AND auto, to make sure it works right
with test.closure("Auto mode: Checking streaming data matches config"):
    # after loading config for Auto mode, we should have AE enabled and be in RS2_DEPTH_AUTO_EXPOSURE_ACCELERATED mode
    test.check(sensor.get_option(rs.option.enable_auto_exposure) == 1)
    if sensor.supports(rs.option.auto_exposure_mode):
        test.check(sensor.get_option(rs.option.auto_exposure_mode) == 1)

    expected_iterations = {}
    for idx, item in enumerate(AUTO_HDR_CONFIG["hdr-preset"]["items"]):
        expected_iterations[idx] = int(item["iterations"])

    seq_id_counts = {}

    pipe.start(cfg)
    log.d(f"Batch size: {batch_size}")
    i = 0
    while i < batch_size * 5:
        data = pipe.wait_for_frames()
        depth_frame = data.get_depth_frame()
        frame_number = depth_frame.get_frame_number()

        seq_id         = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
        frame_exposure = depth_frame.get_frame_metadata(rs.frame_metadata_value.actual_exposure)
        frame_gain     = depth_frame.get_frame_metadata(rs.frame_metadata_value.gain_level)
        seq_size       = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_size)

        if i < batch_size:
            i += 1
            continue
        elif seq_id_0_exp == 0 and seq_id_0_gain == 0 and seq_id != 0:
            # skip until we see seq_id 0
            continue

        seq_id_counts[seq_id] = seq_id_counts.get(seq_id, 0) + 1
        log.d(f"Frame {frame_number} - Sequence ID: {seq_id}, Exposure: {frame_exposure}, Gain: {frame_gain}")
        if seq_id == 0:
            seq_id_0_exp = frame_exposure
            seq_id_0_gain = frame_gain
            i += 1
            continue

        current_controls = AUTO_HDR_CONFIG["hdr-preset"]["items"][seq_id]["controls"]
        expected_exposure = seq_id_0_exp + int(current_controls["depth-ae-exp"])
        expected_gain = seq_id_0_gain + int(current_controls["depth-ae-gain"])

        test.check(frame_exposure == expected_exposure, f"Exposure - Expected: {expected_exposure}, Actual: {frame_exposure}")
        test.check(frame_gain == expected_gain, f"Gain - Expected: {expected_gain}, Actual: {frame_gain}")
        test.check(seq_size == len(AUTO_HDR_CONFIG["hdr-preset"]["items"]))

        if i % batch_size == batch_size - 1:
            test.check(seq_id_counts == expected_iterations,
                       f"Sequence ID counts do not match expected: {seq_id_counts} != {expected_iterations}")
            seq_id_counts.clear()

            log.d("----")
        i += 1

    pipe.stop()

test.print_results_and_exit()
