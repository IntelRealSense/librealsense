# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import pyrealsense2 as rs, os, time, tempfile
from rspy import log, test
from rspy.timer import Timer

# create temporary folder to record to that will be deleted automatically at the end of the script
# (requires that no files are being held open inside this directory. Important to not keep any handle open to a file
# in this directory, any handle as such must be set to None)
temp_dir = tempfile.TemporaryDirectory( prefix='recordings_' )
file_name = temp_dir.name + os.sep + 'rec.bag'

def record_with_pause( file_name ):
    # creating a pipeline and recording to a file
    pipeline = rs.pipeline()
    cfg = rs.config()
    cfg.enable_record_to_file(file_name)
    pipeline_record_profile = pipeline.start(cfg)
    device = pipeline_record_profile.get_device()
    device_recorder = device.as_recorder()

    log.d('Pausing...')
    rs.recorder.pause(device_recorder)
    time.sleep(5)

    log.d('Resumed...')
    rs.recorder.resume(device_recorder)
    time.sleep(5)
    pipeline.stop()

################################################################################################
test.start( "Trying to record using pause and resume and verify playback" )

cfg = pipeline = None
try:

    record_with_pause( file_name )

    # we create a new pipeline and use it to playback from the file we just recoded to
    pipeline = rs.pipeline()
    cfg = rs.config()
    cfg.enable_device_from_file( file_name, repeat_playback = False )
    pipeline_playback_profile = pipeline.start( cfg )
    device = pipeline_playback_profile.get_device()
    device_playback = device.as_playback()
    device_playback.set_real_time( True )
    # if the record-playback worked we will get frames, otherwise the next line will timeout and throw
    pipeline.wait_for_frames()
    test.check_equal(device_playback.current_status(), rs.playback_status.playing)

    # We allow 15 seconds to verify the playback_stopped event.
    # This will verify that pause & resume did not mess up the recorded timestamps and the sleep time between each 2 frame is reasonable. [DSO-14342]
    wait_for_stop_timer = Timer(15)
    wait_for_stop_timer.start()

    while (not wait_for_stop_timer.has_expired()):

        status = device_playback.current_status()

        if status == rs.playback_status.stopped:
            log.d('stopped!')
            break
        else:
            log.d("status =", status)

        time.sleep(1)

    test.check_equal(device_playback.current_status(), rs.playback_status.stopped)

except Exception:
    test.unexpected_exception()
finally: # remove all references to the file and stop the pipeline if exist
    # Cleanup
    cfg = device = device_playback = pipeline = pipeline_playback_profile= None
    if pipeline:
        pipeline.stop()


test.finish()

#############################################################################################
test.print_results_and_exit()
