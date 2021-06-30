# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:timeout 1500
#test:donotrun:!nightly

import pyrealsense2 as rs, os, time
from rspy import log, test, repo
from rspy.timer import Timer

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
wait_for_stop_timer = Timer(10)
for i in range(250):
    try:
        log.d("Starting iteration # " , i)
        ctx = rs.context()
        dev = ctx.load_device( file_name )
        dev.set_real_time( False )
        sensors = dev.query_sensors()
        frames_count = 0
        for sensor in sensors:
            sensor.open( sensor.get_stream_profiles() )
            
        for sensor in sensors:
            sensor.start( frame_callback )
    
        while( True ):
            if dev.current_status() != rs.playback_status.playing:
                time.sleep(0.05)
                continue
            else:
                break
        
        test.check_equal( dev.current_status(), rs.playback_status.playing )
        
        wait_for_stop_timer.start()
        
        # We allow 10 seconds to each iteration to verify the playback_stopped event.
        while( not wait_for_stop_timer.has_expired() ):
        
            status = dev.current_status()
            
            if status == rs.playback_status.stopped:
                log.d( 'stopped!' )
                break
            else:
                log.d( "status =", status )
                
            time.sleep( 1 )
            
        test.check(not wait_for_stop_timer.has_expired())
        
        for sensor in sensors:
            sensor.stop()
            sensor.close()
    except Exception:
        test.unexpected_exception()
    finally:
        test.check_equal(frames_count, frames_in_bag_file)
        dev = None
test.finish()
#############################################################################################

test.print_results_and_exit()
 