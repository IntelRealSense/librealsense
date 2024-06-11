# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test, log

with test.closure("HDR Streaming - custom config"):
    device = test.find_first_device_or_exit()
    depth_sensor = device.first_depth_sensor()

    if test.check(depth_sensor and depth_sensor.supports(rs.option.hdr_enabled)):
        depth_sensor.set_option(rs.option.sequence_size, 2)
        test.check(depth_sensor.get_option(rs.option.sequence_size) == 2)
        first_exposure = 120
        first_gain = 90
        depth_sensor.set_option(rs.option.sequence_id, 1)
        test.check(depth_sensor.get_option(rs.option.sequence_id) == 1)
        depth_sensor.set_option(rs.option.exposure, first_exposure)
        test.check(depth_sensor.get_option(rs.option.exposure) == first_exposure)
        depth_sensor.set_option(rs.option.gain, first_gain)
        test.check(depth_sensor.get_option(rs.option.gain) == first_gain)

        second_exposure = 1200
        second_gain = 20
        depth_sensor.set_option(rs.option.sequence_id, 2)
        test.check(depth_sensor.get_option(rs.option.sequence_id) == 2)
        depth_sensor.set_option(rs.option.exposure, second_exposure)
        test.check(depth_sensor.get_option(rs.option.exposure) == second_exposure)
        depth_sensor.set_option(rs.option.gain, second_gain)
        test.check(depth_sensor.get_option(rs.option.gain) == second_gain)

        depth_sensor.set_option(rs.option.hdr_enabled, 1)
        test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

        cfg = rs.config()
        cfg.enable_stream(rs.stream.depth)
        cfg.enable_stream(rs.stream.infrared, 1)
        pipe = rs.pipeline()
        pipe.start(cfg)

        for iteration in range(1, 100):
            data = pipe.wait_for_frames()

            out_depth_frame = data.get_depth_frame()
            if iteration < 3:
                continue

            if out_depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
                frame_exposure = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.actual_exposure)
                frame_gain = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.gain_level)
                seq_id = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)

                if seq_id == 0:
                    test.check(frame_exposure == first_exposure)
                    test.check(frame_gain == first_gain)
                else:
                    test.check(frame_exposure == second_exposure)
                    test.check(frame_gain == second_gain)

        pipe.stop()
        depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests

test.print_results_and_exit()
