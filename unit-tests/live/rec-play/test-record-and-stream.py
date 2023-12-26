# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:device D400*

import pyrealsense2 as rs, os, time, tempfile
from rspy import test

depth_format = None
depth_fps = None
depth_width = None
depth_height = None
depth_profile = None


def find_default_profile():
    global depth_format, depth_fps, depth_width, depth_height

    for p in depth_sensor.profiles:
        if p.is_default() and p.stream_type() == rs.stream.depth:
            depth_format = p.format()
            depth_fps = p.fps()
            depth_width = p.as_video_stream_profile().width()
            depth_height = p.as_video_stream_profile().height()
            break


def restart_profile():
    """
    You can't use the same profile twice, but we need the same profile several times. So this function resets the
    profiles with the given parameters to allow quick profile creation
    """
    global depth_profile

    depth_profile = next( p for p in depth_sensor.profiles if p.fps() == depth_fps
               and p.stream_type() == rs.stream.depth
               and p.format() == p.format() == depth_format
               and p.as_video_stream_profile().width() == depth_width
               and p.as_video_stream_profile().height() == depth_height )


def record():
    global depth_sensor

    restart_profile()
    depth_sensor.open(depth_profile)
    depth_sensor.start(sync)

    recorder = rs.recorder(file_name, dev)
    time.sleep(3)
    recorder.pause()
    recorder = None

    depth_sensor.stop()
    depth_sensor.close()


def try_streaming():
    global depth_sensor

    restart_profile()
    depth_sensor.open(depth_profile)
    depth_sensor.start(sync)
    time.sleep(3)
    depth_sensor.stop()
    depth_sensor.close()


def play_recording():
    global depth_sensor

    ctx = rs.context()
    playback = ctx.load_device(file_name)
    depth_sensor = playback.first_depth_sensor()

    restart_profile()
    depth_sensor.open(depth_profile)
    depth_sensor.start(sync)

    # if the record-playback worked we will get frames, otherwise the next line will timeout and throw
    sync.wait_for_frames()

    depth_sensor.stop()
    depth_sensor.close()


################################################################################################
with test.closure("Record, stream and playback using sensor interface with syncer"):
    temp_dir = tempfile.mkdtemp()
    file_name = os.path.join(temp_dir, "recording.bag")

    sync = rs.syncer()
    dev = test.find_first_device_or_exit()
    depth_sensor = dev.first_depth_sensor()
    find_default_profile()
    record()

    # after we finish recording we close the sensor and then open it again and try streaming
    try_streaming()

    play_recording()
################################################################################################
test.print_results_and_exit()
