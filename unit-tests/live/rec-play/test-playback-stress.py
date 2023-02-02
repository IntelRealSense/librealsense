# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:timeout 1500
#test:donotrun:!nightly

import pyrealsense2 as rs, os
from rspy import log, test, repo
from playback_helper import PlaybackStatusVerifier

file_name = os.path.join( repo.build, 'unit-tests', 'recordings', 'all_combinations_depth_color.bag' )
log.d( 'deadlock file:', file_name )
frames_in_bag_file = 64

frames_count = 0

def frame_callback( f ):
    global frames_count
    frames_count += 1

################################################################################################
test.start( "Playback stress test" )

log.d( "Playing back: " + file_name )
for i in range(250):
    try:
        log.d("Test - Starting iteration # " , i)
        ctx = rs.context()
        dev = ctx.load_device( file_name )
        psv = PlaybackStatusVerifier( dev );
        dev.set_real_time( False )
        sensors = dev.query_sensors()
        frames_count = 0
        log.d("Opening Sensors")
        for sensor in sensors:
            sensor.open( sensor.get_stream_profiles() )
        log.d("Starting Sensors")
        for sensor in sensors:
            sensor.start( frame_callback )

        psv.wait_for_status( 5, rs.playback_status.playing )

        # We allow 10 seconds to each iteration to verify the playback_stopped event.
        psv.wait_for_status( 10,  rs.playback_status.stopped )

        log.d("Stopping Sensors")
        for sensor in sensors:
            sensor.stop()

        log.d("Closing Sensors")
        for sensor in sensors:
            #log.d("Test Closing Sensor ", sensor)
            sensor.close()
        log.d("Test - Loop ended")
    except Exception:
        test.unexpected_exception()
    finally:
        test.check_equal(frames_count, frames_in_bag_file)
        dev = None
test.finish()
#############################################################################################

test.print_results_and_exit()
 