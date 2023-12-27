# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:device D400*

#The test flow is a result of a fixed bug - viewer crashed when starting stream after finishing record session

import pyrealsense2 as rs, os, time, tempfile
from rspy import test

def find_default_profile():
    default_profile = next(p for p in depth_sensor.profiles if p.is_default() and p.stream_type() == rs.stream.depth)
    return default_profile


def restart_profile(default_profile):
    """
    You can't use the same profile twice, but we need the same profile several times. So this function resets the
    profiles with the given parameters to allow quick profile creation
    """
    depth_profile = next( p for p in depth_sensor.profiles if p.fps() == default_profile.fps()
               and p.stream_type() == rs.stream.depth
               and p.format() == default_profile.format()
               and p.as_video_stream_profile().width() == default_profile.as_video_stream_profile().width()
               and p.as_video_stream_profile().height() == default_profile.as_video_stream_profile().height())
    return depth_profile

def record(file_name, default_profile):
    global depth_sensor

    depth_profile = restart_profile(default_profile)
    depth_sensor.open(depth_profile)
    depth_sensor.start(sync)

    recorder = rs.recorder(file_name, dev)
    time.sleep(3)
    recorder.pause()
    recorder = None

    depth_sensor.stop()
    depth_sensor.close()


def try_streaming(default_profile):
    global depth_sensor

    depth_profile = restart_profile(default_profile)
    depth_sensor.open(depth_profile)
    depth_sensor.start(sync)
    time.sleep(3)
    depth_sensor.stop()
    depth_sensor.close()


def play_recording(default_profile):
    global depth_sensor

    ctx = rs.context()
    playback = ctx.load_device(file_name)
    depth_sensor = playback.first_depth_sensor()

    depth_profile = restart_profile(default_profile)
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
    default_profile = find_default_profile()
    record(file_name, default_profile)

    # after we finish recording we close the sensor and then open it again and try streaming
    try_streaming(default_profile)

    play_recording(default_profile)
################################################################################################
test.print_results_and_exit()
