# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#temporary fix to prevent the test from running on Win_SH_Py_DDS_CI 
#test:donotrun:dds

import pyrealsense2 as rs
from rspy import test
import numpy as np
import tempfile, os.path
import threading
import time
################################################################################################

frame_processor_lock = threading.Lock()
frames_lock = threading.Lock()
completion_event = threading.Event()
all_pairs = True
composite_frames = []
sensor_to_frames = {}
sensor_to_framesets = {}
sensor_to_frame = {}


class AlignRecordBlock:

    def __init__(self, align_to, align_from):
        self._align_from = rs.stream(align_from)
        self._align = rs.align(align_to)

    def record(self, sensor_name, frame, sctx):
        ss = sctx.sw_sensors[sensor_name]
        ss.on_video_frame(create_sw_frame(frame, sctx.sw_stream_profiles[sensor_name][self._align_from]))
        ss.stop()
        ss.close()

    def process(self, frame):
        fs = frame.as_frameset()
        return self._align.process(fs).first_or_default(self._align_from)


def create_sw_frame(video_frame, profile):
    new_video_frame = rs.software_video_frame()
    new_video_frame.data = bytearray(video_frame.get_data())
    new_video_frame.bpp = video_frame.get_bytes_per_pixel()
    new_video_frame.stride = video_frame.get_width() * video_frame.get_bytes_per_pixel()
    new_video_frame.timestamp = video_frame.get_timestamp()
    new_video_frame.domain = video_frame.get_frame_timestamp_domain()
    new_video_frame.frame_number = video_frame.get_frame_number()
    new_video_frame.profile = profile
    return new_video_frame


def validate_ppf_results(result_frame, reference_frame):
    result_profile = result_frame.get_profile().as_video_stream_profile()
    reference_profile = reference_frame.get_profile().as_video_stream_profile()

    test.check_equal(result_profile.width(), reference_profile.width())
    test.check_equal(result_profile.height(), reference_profile.height())

    v1 = bytearray(result_frame.get_data())
    v2 = bytearray(reference_frame.get_data())
    test.check_equal_lists(v1,v2)


def process_frame(frame, frame_source):
    sensor_name = rs.sensor.from_frame(frame).get_info(rs.camera_info.name)
    with frame_processor_lock:
        if sensor_name in sensor_to_frames:
            sensor_to_frames[sensor_name].append(frame)
        else:
            sensor_to_frames[sensor_name] = [frame]

        if len(sensor_to_frames[sensor_name]) == 2:
            frameset = frame_source.allocate_composite_frame(sensor_to_frames[sensor_name])
            frameset.keep()
            sensor_to_framesets[sensor_name] = frameset
            all_pairs = True
        else:
            all_pairs = False


def check_condition(sensors):
    with frames_lock:
        return len(sensor_to_frame) >= len(sensors)


def wait_for_condition(sensors):
    while not check_condition(sensors):
        time.sleep(0.0001)
    completion_event.set()


def callback_for_get_frames(frame, sensor_to_frame, sensors):
    with frames_lock:
        if len(sensor_to_frame) < len(sensors):
            frame.keep()
            sensor_name = rs.sensor.from_frame(frame).get_info(rs.camera_info.name)
            sensor_to_frame[sensor_name] = frame


def get_frames(sensors):
    frames = []
    for s in sensors:
        s.open(s.get_stream_profiles())

    for s in sensors:
        s.start(lambda frame: callback_for_get_frames(frame, sensor_to_frame, sensors))

    timeout = 30
    start_time = time.time()

    condition_thread = threading.Thread(target=wait_for_condition(sensors))
    condition_thread.start()
    condition_thread.join(timeout)

    test.check(time.time() - start_time < timeout)

    for s in sensors:
        s.stop()

    for s in sensors:
        s.close()

    for sf in sensor_to_frame:
        frames.append(sensor_to_frame[sf])

    return frames


def get_composite_frames(sensors):
    frame_processor = rs.processing_block(process_frame)
    for sensor in sensors:
        sensor.open(sensor.get_stream_profiles())

    for sensor in sensors:
        sensor.start(lambda frame: (frame.keep(), frame_processor.invoke(frame)))

    while True:
        with frame_processor_lock:
            if len(sensor_to_framesets) >= len(sensors) and all_pairs is True:
                break
            time.sleep(0.0001)

    for sf in sensor_to_framesets:
        composite_frames.append(sensor_to_framesets[sf])

    for sensor in sensors:
        sensor.stop()

    for sensor in sensors:
        sensor.close()

    return composite_frames


def compare_processed_frames_vs_recorded_frames(record_block, file):
    temp_dir = os.path.join(tempfile.gettempdir())
    filename = os.path.join(temp_dir, "all_combinations_depth_color.bag")

    ctx = rs.context()
    dev = ctx.load_device(filename)
    dev.set_real_time(False)

    sensors = dev.query_sensors()
    frames = get_composite_frames(sensors)
    ref_dev = ctx.load_device(os.path.join(temp_dir, file))
    ref_dev.set_real_time(False)

    ref_sensors = ref_dev.query_sensors()
    ref_frames = get_frames(ref_sensors)

    test.check_equal(len(ref_frames), len(frames))

    for i in range(len(frames)):
        fs_res = record_block.process(frames[i])
        validate_ppf_results(fs_res, ref_frames[i])


################################################################################################
with test.closure("Test align depth to color from recording"):
    record_block = AlignRecordBlock(rs.stream.color, rs.stream.depth)
    compare_processed_frames_vs_recorded_frames(record_block, "[aligned_2c]_all_combinations_depth_color.bag")

################################################################################################
with test.closure("Test align color to depth from recording"):

    composite_frames = []
    sensor_to_frames = {}
    sensor_to_framesets = {}
    sensor_to_frame = {}
    all_pairs = True

    record_block = AlignRecordBlock(rs.stream.depth, rs.stream.color)
    compare_processed_frames_vs_recorded_frames(record_block, "[aligned_2d]_all_combinations_depth_color.bag")

################################################################################################
test.print_results_and_exit()
