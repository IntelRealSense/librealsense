# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import pyrealsense2 as rs, os, time, tempfile, sys
from rspy import devices, log, test

cp = dp = None
previous_depth_frame_number = -1
previous_color_frame_number = -1
got_frames = False

def got_frame():
    global got_frames
    got_frames = True

def color_frame_call_back( frame ):
    global previous_color_frame_number
    got_frame()
    test.check_frame_drops( frame, previous_color_frame_number )
    previous_color_frame_number = frame.get_frame_number()

def depth_frame_call_back( frame ):
    global previous_depth_frame_number
    got_frame()
    test.check_frame_drops( frame, previous_depth_frame_number )
    previous_depth_frame_number = frame.get_frame_number()

def restart_profiles():
    """
    You can't use the same profile twice, but we need the same profile several times. So this function resets the
    profiles with the given parameters to allow quick profile creation
    """
    global cp, dp, color_sensor, depth_sensor
    for p in color_sensor.profiles:
        if p.is_default() and p.stream_type() == rs.stream.color:
            cp = p
            break
    for p in depth_sensor.profiles:
        if p.is_default() and p.stream_type() == rs.stream.depth:
            dp = p
            break
    

# create temporary folder to record to that will be deleted automatically at the end of the script
temp_dir = tempfile.TemporaryDirectory(prefix='recordings_')
file_name = temp_dir.name + os.sep + 'rec.bag'

################################################################################################
test.start("Trying to record and playback using pipeline interface")

cfg = pipeline = None
try:
    # creating a pipeline and recording to a file
    pipeline = rs.pipeline()
    cfg = rs.config()
    cfg.enable_record_to_file( file_name )
    pipeline.start( cfg )
    time.sleep(3)
    pipeline.stop()
    # we create a new pipeline and use it to playback from the file we just recoded to
    pipeline = rs.pipeline()
    cfg = rs.config()
    cfg.enable_device_from_file(file_name)
    pipeline.start(cfg)
    # if the record-playback worked we will get frames, otherwise the next line will timeout and throw
    pipeline.wait_for_frames()
except Exception:
    test.unexpected_exception()
finally: # we must remove all references to the file so we can use it again in the next test
    cfg = None
    if pipeline:
        try:
            pipeline.stop()
        except RuntimeError as rte:
            # if the error Occurred because the pipeline wasn't started we ignore it
            if str( rte ) != "stop() cannot be called before start()":
                test.unexpected_exception()
        except Exception:
            test.unexpected_exception()

test.finish()

################################################################################################
test.start("Trying to record and playback using sensor interface")

recorder = depth_sensor = color_sensor = playback = None
try:
    dev = test.find_first_device_or_exit()
    recorder = rs.recorder( file_name, dev )
    depth_sensor = dev.first_depth_sensor()
    color_sensor = dev.first_color_sensor()

    restart_profiles()

    depth_sensor.open( dp )
    depth_sensor.start( lambda f: None )
    color_sensor.open( cp )
    color_sensor.start( lambda f: None )

    time.sleep(3)

    recorder.pause()
    recorder = None
    color_sensor.stop()
    color_sensor.close()
    depth_sensor.stop()
    depth_sensor.close()

    ctx = rs.context()
    playback = ctx.load_device( file_name )

    depth_sensor = playback.first_depth_sensor()
    color_sensor = playback.first_color_sensor()

    restart_profiles()

    depth_sensor.open( dp )
    depth_sensor.start( depth_frame_call_back )
    color_sensor.open( cp )
    color_sensor.start( color_frame_call_back )

    time.sleep(3)

    # if record and playback worked we will receive frames, the callback functions will be called and got-frames
    # will be True. If the record and playback failed it will be false
    test.check( got_frames )
except Exception:
    test.unexpected_exception()
finally: # we must remove all references to the file so we can use it again in the next test
    if recorder:
        recorder.pause()
        recorder = None
    if playback:
        playback.pause()
        playback = None
    # if the sensor is already closed get_active_streams returns an empty list
    if depth_sensor:
        if depth_sensor.get_active_streams():
            try:
                depth_sensor.stop()
            except RuntimeError as rte:
                if str( rte ) != "stop_streaming() failed. UVC device is not streaming!":
                    test.unexpected_exception()
            except Exception:
                test.unexpected_exception()
            depth_sensor.close()
        depth_sensor = None
    if color_sensor:
        if color_sensor.get_active_streams():
            try:
                color_sensor.stop()
            except RuntimeError as rte:
                if str(rte) != "stop_streaming() failed. UVC device is not streaming!":
                    test.unexpected_exception()
            except Exception:
                test.unexpected_exception()
            color_sensor.close()
        color_sensor = None

test.finish()

#####################################################################################################
test.start("Trying to record and playback using sensor interface with syncer")

try:
    sync = rs.syncer()
    dev = test.find_first_device_or_exit()
    recorder = rs.recorder(file_name, dev)
    depth_sensor = dev.first_depth_sensor()
    color_sensor = dev.first_color_sensor()

    restart_profiles()

    depth_sensor.open(dp)
    depth_sensor.start(sync)
    color_sensor.open(cp)
    color_sensor.start(sync)

    time.sleep(3)

    recorder.pause()
    recorder = None
    color_sensor.stop()
    color_sensor.close()
    depth_sensor.stop()
    depth_sensor.close()

    ctx = rs.context()
    playback = ctx.load_device(file_name)

    depth_sensor = playback.first_depth_sensor()
    color_sensor = playback.first_color_sensor()

    restart_profiles()

    depth_sensor.open(dp)
    depth_sensor.start(sync)
    color_sensor.open(cp)
    color_sensor.start(sync)

    # if the record-playback worked we will get frames, otherwise the next line will timeout and throw
    sync.wait_for_frames()
except Exception:
    test.unexpected_exception()
finally: # we must remove all references to the file so the temporary folder can be deleted
    if recorder:
        recorder.pause()
        recorder = None
    if playback:
        playback.pause()
        playback = None
    # if the sensor is already closed get_active_streams returns an empty list
    if depth_sensor:
        if depth_sensor.get_active_streams():
            try:
                depth_sensor.stop()
            except RuntimeError as rte:
                if str( rte ) != "stop_streaming() failed. UVC device is not streaming!":
                    test.unexpected_exception()
            except Exception:
                test.unexpected_exception()
            depth_sensor.close()
        depth_sensor = None
    if color_sensor:
        if color_sensor.get_active_streams():
            try:
                color_sensor.stop()
            except RuntimeError as rte:
                if str(rte) != "stop_streaming() failed. UVC device is not streaming!":
                    test.unexpected_exception()
            except Exception:
                test.unexpected_exception()
            color_sensor.close()
        color_sensor = None

test.finish()

#############################################################################################
test.print_results_and_exit()
