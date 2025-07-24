# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test, log
import json

hdr_config = {
    "hdr-preset": {
        "id": "0",
        "iterations": "0",
        "items": [
            {
                "iterations": "1",
                "controls": {
                    "depth-gain": "16",
                    "depth-exposure": "1"
                }
            },
            {
                "iterations": "2",
                "controls": {
                    "depth-gain": "61",
                    "depth-exposure": "10"
                }
            },
            {
                "iterations": "1",
                "controls": {
                    "depth-gain": "116",
                    "depth-exposure": "100"
                }
            },
            {
                "iterations": "3",
                "controls": {
                    "depth-gain": "161",
                    "depth-exposure": "1000"
                }
            },
            {
                "iterations": "1",
                "controls": {
                    "depth-gain": "22",
                    "depth-exposure": "10000"
                }
            },
            {
                "iterations": "2",
                "controls": {
                    "depth-gain": "222",
                    "depth-exposure": "4444"
                }
            }
        ]
    }
}

device, ctx = test.find_first_device_or_exit()
am = rs.rs400_advanced_mode(device)

with test.closure("Comparing auto-hdr loading and reading"):
    prior_config = json.loads(am.serialize_json())

    am.load_json(json.dumps(hdr_config))  # json dumps just stringify the object

    hdr_config_read = json.loads(am.serialize_json())  # jsonify, note this JSON contains also other keys (params)
    test.check(hdr_config_read.get("hdr-preset") == hdr_config.get("hdr-preset"))

    # check that the other keys are unchanged
    for key in hdr_config_read.keys():
        if key != "hdr-preset":
            test.check(hdr_config_read[key] == prior_config[key])

with test.closure("Checking streaming data matches config"):
    depth_sensor = device.first_depth_sensor()
    test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth)
    pipe = rs.pipeline(ctx)
    pipe.start(cfg)

    expected_data = {}
    for i, item in enumerate(hdr_config["hdr-preset"]["items"]):
        iterations = int(item["iterations"])
        for _ in range(iterations):
            controls = item["controls"]
            # expected that i == sequence id
            expected_data[i] = {key: value for key, value in controls.items()}
    batch_size = len(expected_data)

    for _ in range(0, batch_size * 5 + 1):
        data = pipe.wait_for_frames()

        out_depth_frame = data.get_depth_frame()
        frame_number = out_depth_frame.get_frame_number()

        if frame_number <= batch_size:  # TODO: add a check that the sequence number is as expected too
            continue

        seq_id = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)

        frame_exposure = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.actual_exposure)
        test.check(str(frame_exposure) == expected_data[seq_id]["depth-exposure"])

        frame_gain = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.gain_level)
        test.check(str(frame_gain) == expected_data[seq_id]["depth-gain"])

        seq_size = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_size)
        test.check(seq_size == len(hdr_config["hdr-preset"]["items"]))

        log.d(f"frame number: {frame_number}, exposure: {frame_exposure}, gain: {frame_gain}, seq_id: {seq_id}, seq_size: {seq_size}")
        log.d(f"expected data: {expected_data[seq_id]}")

    pipe.stop()

test.print_results_and_exit()
