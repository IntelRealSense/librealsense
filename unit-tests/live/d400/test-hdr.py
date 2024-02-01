# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

######################################
# This set of tests is valid for any device that supports the HDR feature #
######################################

# test:device D400* !D457

import pyrealsense2 as rs
from rspy import test, log
import time

# HDR CONFIGURATION TESTS
with test.closure("HDR Config - default config", "[hdr][live]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx is not None:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if device_count != 0:
            depth_sensor = device.first_depth_sensor()

            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                exposure_range = depth_sensor.get_option_range(rs.option.exposure)
                gain_range = depth_sensor.get_option_range(rs.option.gain)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                depth_sensor.set_option(rs.option.sequence_id, 1)
                test.check(depth_sensor.get_option(rs.option.sequence_id) == 1)
                exp = depth_sensor.get_option(rs.option.exposure)
                test.check(depth_sensor.get_option(rs.option.exposure) == exposure_range.default - 1000)  # w/a
                test.check(depth_sensor.get_option(rs.option.gain) == gain_range.default)

                depth_sensor.set_option(rs.option.sequence_id, 2)
                test.check(depth_sensor.get_option(rs.option.sequence_id) == 2)
                test.check(depth_sensor.get_option(rs.option.exposure) == exposure_range.min)
                test.check(depth_sensor.get_option(rs.option.gain) == gain_range.min)

                depth_sensor.set_option(rs.option.hdr_enabled, 0)
                enabled = depth_sensor.get_option(rs.option.hdr_enabled)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 0)
        else:
            log.e("No device was found - skipping test")

with test.closure("HDR Config - custom config", "[hdr][live]"):
    # Require at least one device to be plugged in
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx is not None:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if device_count != 0:
            depth_sensor = device.first_depth_sensor()

            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                depth_sensor.set_option(rs.option.sequence_size, 2)
                test.check(depth_sensor.get_option(rs.option.sequence_size) == 2)

                depth_sensor.set_option(rs.option.sequence_id, 1)
                test.check(depth_sensor.get_option(rs.option.sequence_id) == 1)
                depth_sensor.set_option(rs.option.exposure, 120)
                test.check(depth_sensor.get_option(rs.option.exposure) == 120)
                depth_sensor.set_option(rs.option.gain, 90)
                test.check(depth_sensor.get_option(rs.option.gain) == 90)

                depth_sensor.set_option(rs.option.sequence_id, 2)
                test.check(depth_sensor.get_option(rs.option.sequence_id) == 2)
                depth_sensor.set_option(rs.option.exposure, 1200)
                test.check(depth_sensor.get_option(rs.option.exposure) == 1200)
                depth_sensor.set_option(rs.option.gain, 20)
                test.check(depth_sensor.get_option(rs.option.gain) == 20)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                depth_sensor.set_option(rs.option.hdr_enabled, 0)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 0)
        else:
            log.e("No device was found - skipping test")

# HDR STREAMING TESTS
with test.closure("HDR Streaming - default config", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx is not None:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if device_count != 0:
            depth_sensor = device.first_depth_sensor()

            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                exposure_range = depth_sensor.get_option_range(rs.option.exposure)
                gain_range = depth_sensor.get_option_range(rs.option.gain)

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
                            test.check(frame_exposure == exposure_range.default - 1000)  # w/a
                            test.check(frame_gain == gain_range.default)
                        else:
                            test.check(frame_exposure == exposure_range.min)
                            test.check(frame_gain == gain_range.min)
                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")


with test.closure("HDR Streaming - custom config", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()

            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
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

                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")

# CHECKING HDR AFTER PIPE RESTART

with test.closure("HDR Running - restart hdr at restream", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()
            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):

                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                pipe = rs.pipeline()
                pipe.start(cfg)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                for i in range(10):
                    data = pipe.wait_for_frames()

                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)
                pipe.stop()

                pipe.start(cfg)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)
                pipe.stop()

                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")


# helper method
# checks that the frames resulting from the hdr_merge processing block are generated
# from frames streamed within this method (and not using old frames)
# returned value: False if some metadata could not be read - otherwise, True
def check_hdr_frame_counter(pipe, num_of_frames, merging_filter):
    min_counter = -1
    min_counter_set = False
    for i in range(num_of_frames):
        data = pipe.wait_for_frames()

        # get depth frame data
        depth_frame = data.get_depth_frame()
        if not depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
            log.e("Frame metadata not supported")
            return False

        depth_seq_id = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
        depth_counter = depth_frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
        depth_ts = depth_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)

        if not min_counter_set:
            min_counter = int(depth_counter)
            min_counter_set = True

        # apply HDR Merge process
        merged_frame = merging_filter.process(data)

        # get hdr frame data
        if not merged_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
            log.e("Frame metadata not supported")
            return False

        hdr_counter = merged_frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
        hdr_ts = merged_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
        test.check(hdr_counter >= min_counter)
        if min_counter > hdr_counter:
            log.e("min:",min_counter, "hdr:",hdr_counter)
    return True


# CHECKING HDR MERGE AFTER HDR RESTART
with test.closure("HDR Running - hdr merge after hdr restart", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()
            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                # initializing the merging filter
                merging_filter = rs.hdr_merge()

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                pipe = rs.pipeline()
                pipe.start(cfg)

                frame_metadata_enabled = True
                number_frames_streamed = 10

                if not check_hdr_frame_counter(pipe, number_frames_streamed, merging_filter):
                    frame_metadata_enabled = False

                depth_sensor.set_option(rs.option.hdr_enabled, 0)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 0)

                if frame_metadata_enabled:
                    if not check_hdr_frame_counter(pipe, number_frames_streamed, merging_filter):
                        frame_metadata_enabled = False

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                if frame_metadata_enabled:
                    check_hdr_frame_counter(pipe, number_frames_streamed, merging_filter)

                pipe.stop()
                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")


# CHECKING SEQUENCE ID WHILE STREAMING
with test.closure("HDR Streaming - checking sequence id", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()
            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                cfg.enable_stream(rs.stream.infrared, 1)
                pipe = rs.pipeline()
                pipe.start(cfg)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                sequence_id = -1
                iterations_for_preparation = 6
                for iteration in range(1, 50):  # Application still alive?
                    data = pipe.wait_for_frames()

                    if iteration < iterations_for_preparation:
                        continue

                    depth_frame = data.get_depth_frame()
                    if depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
                        depth_seq_id = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
                        ir_frame = data.get_infrared_frame(1)
                        ir_seq_id = ir_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
                        if iteration == iterations_for_preparation:
                            test.check(depth_seq_id == ir_seq_id)
                            sequence_id = int(depth_seq_id)
                        else:
                            sequence_id = 1 if sequence_id == 0 else 0
                            if sequence_id != depth_seq_id:
                                log.e("sequence_id != depth_seq_id")
                                log.e(f"iteration = {iteration}, iterations_for_preparation = {iterations_for_preparation}")

                            test.check(sequence_id == depth_seq_id)
                            test.check(sequence_id == ir_seq_id)
                pipe.stop()
                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")


with test.closure("Emitter on/off - checking sequence id", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()

            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                cfg.enable_stream(rs.stream.infrared, 1)
                pipe = rs.pipeline()
                pipe.start(cfg)

                if depth_sensor.supports(rs.option.emitter_on_off):
                    depth_sensor.set_option(rs.option.emitter_on_off, 1)
                    test.check(depth_sensor.get_option(rs.option.emitter_on_off) == 1)

                sequence_id = -1

                # emitter on/off works with PWM (pulse with modulation) in the hardware
                # this takes some time to configure it
                iterations_for_preparation = 10
                for iteration in range(1, 50):  # Application still alive?
                    data = pipe.wait_for_frames()

                    if iteration < iterations_for_preparation:
                        continue

                    depth_frame = data.get_depth_frame()
                    if depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
                        depth_seq_id = depth_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)
                        ir_frame = data.get_infrared_frame(1)
                        ir_seq_id = ir_frame.get_frame_metadata(rs.frame_metadata_value.sequence_id)

                        if iteration == iterations_for_preparation:
                            test.check(depth_seq_id == ir_seq_id)
                            sequence_id = int(depth_seq_id)
                        else:
                            sequence_id = 1 if sequence_id == 0 else 0
                            test.check(sequence_id == depth_seq_id)
                            test.check(sequence_id == ir_seq_id)
                pipe.stop()
                depth_sensor.set_option(rs.option.emitter_on_off, 0)
        else:
            log.e("No device was found - skipping test")


# This test is removed until its failure is solved
# It does not fail locally but it fails on our internal validation environment
# This tests checks that the previously saved merged frame is discarded after a pipe restart
with test.closure("HDR Merge - discard merged frame", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()
            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                pipe = rs.pipeline()
                pipe.start(cfg)

                # initializing the merging filter
                merging_filter = rs.hdr_merge()

                num_of_iterations_in_serie = 10
                first_series_last_merged_ts = -1
                at_least_one_frame_supported_seq_id = False
                for i in range(0, num_of_iterations_in_serie):
                    data = pipe.wait_for_frames()
                    out_depth_frame = data.get_depth_frame()

                    if out_depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
                        at_least_one_frame_supported_seq_id = True
                        # merging the frames from the different HDR sequence IDs
                        merged_frameset = merging_filter.process(data)
                        merged_depth_frame = merged_frameset.as_frameset().get_depth_frame()

                        frame_ts = merged_depth_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)

                        if i == (num_of_iterations_in_serie - 1):
                            first_series_last_merged_ts = frame_ts

                pipe.stop()

                if at_least_one_frame_supported_seq_id:
                    test.check(first_series_last_merged_ts != -1)
                    test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)
                    pipe.start(cfg)

                    for i in range(0,10):
                        data = pipe.wait_for_frames()
                        out_depth_frame = data.get_depth_frame()

                        if out_depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):
                            # merging the frames from the different HDR sequence IDs
                            merged_frameset = merging_filter.process(data)
                            merged_depth_frame = merged_frameset.as_frameset().get_depth_frame()

                            frame_ts = merged_depth_frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                            test.check(frame_ts > first_series_last_merged_ts)
                    pipe.stop()
                else:
                    log.e("sequence_id metadata was not supported - skipping test")

                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")


with test.closure("HDR Start Stop - recover manual exposure and gain", "[HDR]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()
            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):

                gain_before_hdr = 50
                depth_sensor.set_option(rs.option.gain, gain_before_hdr)
                test.check(depth_sensor.get_option(rs.option.gain) == gain_before_hdr)

                exposure_before_hdr = 5000
                depth_sensor.set_option(rs.option.exposure, exposure_before_hdr)
                test.check(depth_sensor.get_option(rs.option.exposure) == exposure_before_hdr)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                pipe = rs.pipeline()
                pipe.start(cfg)

                iteration_for_disable = 50
                iteration_to_check_after_disable = iteration_for_disable + 5  # Was 2, aligned to validation KPI's [DSO-18682]
                for iteration in range(1, 70):

                    data = pipe.wait_for_frames()

                    out_depth_frame = data.get_depth_frame()

                    if out_depth_frame.supports_frame_metadata(rs.frame_metadata_value.sequence_id):

                        frame_gain = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.gain_level)
                        frame_exposure = out_depth_frame.get_frame_metadata(rs.frame_metadata_value.actual_exposure)

                        if iteration > iteration_for_disable and iteration < iteration_to_check_after_disable:
                            continue

                        if iteration == iteration_for_disable:
                            depth_sensor.set_option(rs.option.hdr_enabled, 0)
                            test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 0)

                        elif iteration >= iteration_to_check_after_disable:
                            test.check(frame_gain == gain_before_hdr)
                            if frame_exposure != exposure_before_hdr:
                                log.e("frame_exposure != exposure_before_hdr")
                                log.e(f"iteration = {iteration}, iteration_to_check_after_disable = {iteration_to_check_after_disable}")
                            test.check(frame_exposure == exposure_before_hdr)
        else:
            log.e("No device was found - skipping test")

# CONTROLS STABILITY WHILE HDR ACTIVE
with test.closure("HDR Active - set locked options", "[hdr][live]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()

            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                # setting laser ON
                if depth_sensor.supports(rs.option.emitter_enabled):
                    depth_sensor.set_option(rs.option.emitter_enabled, 1)

                test.check(depth_sensor.supports(rs.option.laser_power))
                laser_power_before_hdr = depth_sensor.get_option(rs.option.laser_power)

                time.sleep(1.5)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                # the following calls should not be performed and should send a LOG_WARNING
                if depth_sensor.supports(rs.option.enable_auto_exposure):
                    depth_sensor.set_option(rs.option.enable_auto_exposure, 1)
                    test.check(depth_sensor.get_option(rs.option.enable_auto_exposure) == 0)

                if depth_sensor.supports(rs.option.emitter_enabled):
                    depth_sensor.set_option(rs.option.emitter_enabled, 0)
                    test.check(depth_sensor.get_option(rs.option.emitter_enabled) == 1)

                if depth_sensor.supports(rs.option.emitter_on_off):
                    depth_sensor.set_option(rs.option.emitter_on_off, 1)
                    test.check(depth_sensor.get_option(rs.option.emitter_on_off) == 0)

                # Control's presence verified in the beginning of the block
                depth_sensor.set_option(rs.option.laser_power, laser_power_before_hdr - 30)
                test.check(depth_sensor.get_option(rs.option.laser_power) == laser_power_before_hdr)

                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")


with test.closure("HDR Streaming - set locked options", "[hdr][live][using_pipeline]"):
    ctx = rs.context()
    device = test.find_first_device_or_exit()
    if ctx:
        devices_list = ctx.query_devices()
        device_count = devices_list.size()
        if devices_list.size() != 0:
            depth_sensor = device.first_depth_sensor()
            if depth_sensor and depth_sensor.supports(rs.option.hdr_enabled):
                # setting laser ON
                depth_sensor.set_option(rs.option.emitter_enabled, 1)
                laser_power_before_hdr = depth_sensor.get_option(rs.option.laser_power)

                time.sleep(1.5)

                depth_sensor.set_option(rs.option.hdr_enabled, 1)
                test.check(depth_sensor.get_option(rs.option.hdr_enabled) == 1)

                cfg = rs.config()
                cfg.enable_stream(rs.stream.depth)
                pipe = rs.pipeline()
                pipe.start(cfg)

                for iteration in range(1, 50):
                    data = pipe.wait_for_frames()

                    if iteration == 20:
                        # the following calls should not be performed and should send a LOG_WARNING
                        depth_sensor.set_option(rs.option.enable_auto_exposure, 1)
                        test.check(depth_sensor.get_option(rs.option.enable_auto_exposure) == 0)

                        depth_sensor.set_option(rs.option.emitter_enabled, 0)
                        test.check(depth_sensor.get_option(rs.option.emitter_enabled) == 1)

                        depth_sensor.set_option(rs.option.emitter_on_off, 1)
                        test.check(depth_sensor.get_option(rs.option.emitter_on_off) == 0)

                        depth_sensor.set_option(rs.option.laser_power, laser_power_before_hdr - 30)
                        test.check(depth_sensor.get_option(rs.option.laser_power) == laser_power_before_hdr)
                depth_sensor.set_option(rs.option.hdr_enabled, 0)  # disable hdr before next tests
        else:
            log.e("No device was found - skipping test")
