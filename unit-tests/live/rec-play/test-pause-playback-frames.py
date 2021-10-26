# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*


# Tests objective - Verify that pause & resume did not mess up the recorded timestamps and the sleep time between
# each 2 frame is reasonable. We had a BUG with calculating the sleep time between each 2 frames when the pause
# action occurred before the recording base time was set (first frame arrival time). Here we test multiple flows on
# pause & resume actions and verify that the whole file will be be played until a stop event (EOF) at a reasonable
# time , see [DSO-14342]

import pyrealsense2 as rs, os, time, tempfile
from rspy import log, test
from rspy.timer import Timer

# create temporary folder to record to that will be deleted automatically at the end of the script
# (requires that no files are being held open inside this directory. Important to not keep any handle open to a file
# in this directory, any handle as such must be set to None)
temp_dir = tempfile.TemporaryDirectory( prefix='recordings_' )
file_name = temp_dir.name + os.sep + 'rec.bag'

stopped_detected = False


def record_with_pause( file_name, iterations, pause_delay=0, resume_delay=0 ):
    # creating a pipeline and recording to a file
    pipeline = rs.pipeline()
    cfg = rs.config()
    cfg.enable_record_to_file( file_name )
    pipeline_record_profile = pipeline.start( cfg )
    device = pipeline_record_profile.get_device()
    device_recorder = device.as_recorder()

    for i in range( iterations ):
        if pause_delay > 0:
            time.sleep( pause_delay )
        log.d( 'Pausing...' )
        rs.recorder.pause(device_recorder)

        if resume_delay > 0:
            time.sleep( resume_delay )
        log.d('Resumed...')
        rs.recorder.resume( device_recorder )
        time.sleep( 3 )

    pipeline.stop()


def playback( pipeline, file_name, signal_on_stop ):
    cfg = rs.config()
    cfg.enable_device_from_file( file_name, repeat_playback=False )
    log.d( 'Playing...' )
    pipeline_playback_profile = pipeline.start( cfg )
    device = pipeline_playback_profile.get_device()
    playback_dev = device.as_playback()
    playback_dev.set_real_time( True )
    playback_dev.set_status_changed_callback( signal_on_stop )
    pipeline.wait_for_frames()
    test.check_equal( playback_dev.current_status(), rs.playback_status.playing )
    return playback_dev


def verify_stop_when_eof( timeout ):
    global stopped_detected
    stopped_detected = False

    wait_for_stop_timer = Timer( timeout )
    wait_for_stop_timer.start()

    while not wait_for_stop_timer.has_expired():
        if stopped_detected:
            log.d( 'stopped!' )
            break
        else:
            log.d( 'waiting for playback status -> stop' )

        time.sleep( 1 )

    test.check( stopped_detected )


def signal_on_stop( playback_status ):
    log.d( 'playback status callback invoked with ', playback_status )
    if playback_status == rs.playback_status.stopped:
        global stopped_detected
        stopped_detected = True


################################################################################################

test.start("Immediate pause & test")
# probably pause & resume will occur before recording base time is set.

try:
    record_with_pause( file_name, iterations=1, pause_delay=0, resume_delay=0 )
    pipeline = rs.pipeline()
    device_playback = playback( pipeline, file_name, signal_on_stop )
    verify_stop_when_eof( timeout=10 )

except Exception:
    test.unexpected_exception()
finally:  # remove all references to the file and dereference the pipeline
    device_playback = None
    pipeline = None

test.finish()

################################################################################################
test.start("Immediate pause & delayed resume test")
# Pause time should be lower than recording base time and resume time higher

try:

    record_with_pause( file_name, iterations=1, pause_delay=0, resume_delay=5 )
    pipeline = rs.pipeline()
    device_playback = playback( pipeline, file_name, signal_on_stop )
    verify_stop_when_eof( timeout=10 )

except Exception:
    test.unexpected_exception()
finally:   # remove all references to the file and dereference the pipeline
    device_playback = None
    pipeline = None

test.finish()

################################################################################################
test.start("delayed pause & delayed resume test")
# Pause & resume will occur after recording base time is set
try:

    record_with_pause( file_name, iterations=1, pause_delay=3, resume_delay=2 )
    pipeline = rs.pipeline()
    device_playback = playback( pipeline, file_name, signal_on_stop )
    verify_stop_when_eof( timeout=15 )

except Exception:
    test.unexpected_exception()
finally:   # remove all references to the file and dereference the pipeline
    device_playback = None
    pipeline = None

test.finish()

################################################################################################
test.start("multiple delay & pause test")
# Combination of some of the previous tests, testing accumulated recording capture time

try:

    record_with_pause( file_name, iterations=2, pause_delay=0, resume_delay=2 )
    pipeline = rs.pipeline()
    device_playback = playback( pipeline, file_name, signal_on_stop )
    verify_stop_when_eof( timeout=20 )

except Exception:
    test.unexpected_exception()
finally:   # remove all references to the file and dereference the pipeline
    device_playback = None
    pipeline = None

test.finish()

#############################################################################################
test.print_results_and_exit()
