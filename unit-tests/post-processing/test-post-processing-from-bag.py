# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#temporary fix to prevent the test from running on Win_SH_Py_DDS_CI 
#test:donotrun:dds

import pyrealsense2 as rs
from rspy import test, repo
import os.path
import time
################################################################################################
pre_aligned_frames_map ={}
frames_data_map = {}
playback_status = None
sensors = []
align_from = rs.stream(rs.stream.depth)
align = rs.align(rs.stream.color)
callback = None


def playback_callback(status):
    global playback_status
    playback_status = status


def validate_ppf_results(result_frame_data, reference_frame_data):
    result_bytearray_frame_data, result_profile = result_frame_data
    reference_bytearray_frame_data, reference_profile = reference_frame_data

    test.check_equal(result_profile.width(), reference_profile.width())
    test.check_equal(result_profile.height(), reference_profile.height())
    test.check_equal_lists(result_bytearray_frame_data,reference_bytearray_frame_data)


def process_frame(frame, frame_source):
    sensor_name = rs.sensor.from_frame(frame).get_info(rs.camera_info.name)
    if sensor_name in pre_aligned_frames_map:
        pre_aligned_frames_map[sensor_name].append(frame)
    else:
        pre_aligned_frames_map[sensor_name] = [frame]

    if len(pre_aligned_frames_map[sensor_name]) == 2:
        frameset = frame_source.allocate_composite_frame(pre_aligned_frames_map[sensor_name])
        fs_align = callback(frameset.as_frameset())
        fs_align_data = bytearray(fs_align.get_data())
        fs_align_profile = fs_align.get_profile().as_video_stream_profile()
        frames_data_map[sensor_name] = (fs_align_data, fs_align_profile)


def load_ref_frames_to_map(frame):
    if len(frames_data_map) < len(sensors):
        sensor_name = rs.sensor.from_frame(frame).get_info(rs.camera_info.name)
        frames_data_map[sensor_name] = (bytearray(frame.get_data()), frame.get_profile().as_video_stream_profile())


def get_frames(callback):
    for s in sensors:
        s.open(s.get_stream_profiles())

    for s in sensors:
        s.start(callback)

    while playback_status != rs.playback_status.stopped:
        time.sleep(0.25)

    for s in sensors:
        s.stop()

    for s in sensors:
        s.close()


def playback_file(file, callback):
    global playback_status
    playback_status = None
    filename = os.path.join(repo.build, 'unit-tests', 'recordings', file)
    ctx = rs.context()

    dev = ctx.load_device(filename)
    dev.set_real_time(False)
    dev.set_status_changed_callback(playback_callback)

    global sensors
    sensors = dev.query_sensors()

    global frames_data_map
    frames_data_map = {}
    get_frames(callback)

    frames_data_list = []
    for sf in frames_data_map:
        frames_data_list.append(frames_data_map[sf])

    return frames_data_list


def compare_processed_frames_vs_recorded_frames(file):
    # we need processing_block in order to have frame_source.
    # frame_source is used to composite frames (by calling allocate_composite_frames function).
    frame_processor = rs.processing_block(process_frame)

    aligned_frames_data_list = playback_file('all_combinations_depth_color.bag', lambda frame: (frame_processor.invoke(frame)))
    ref_frame_data_list = playback_file(file, lambda frame: load_ref_frames_to_map(frame))
    test.check_equal(len(aligned_frames_data_list),len(ref_frame_data_list))

    for i in range(len(aligned_frames_data_list)):
        validate_ppf_results(aligned_frames_data_list[i], ref_frame_data_list[i])

################################################################################################
with test.closure("Test align depth to color from recording"):
    align_from = rs.stream(rs.stream.depth)
    align = rs.align(rs.stream.color)
    callback = lambda fs: align.process(fs).first_or_default(align_from)

    compare_processed_frames_vs_recorded_frames("[aligned_2c]_all_combinations_depth_color.bag")
################################################################################################
with test.closure("Test align color to depth from recording"):
    align_from = rs.stream(rs.stream.color)
    align = rs.align(rs.stream.depth)
    callback = lambda fs: align.process(fs).first_or_default(align_from)
    pre_aligned_frames_map = {}

    compare_processed_frames_vs_recorded_frames("[aligned_2d]_all_combinations_depth_color.bag")
################################################################################################
test.print_results_and_exit()
