# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries:gha 2

from rspy import log, test
log.nested = 'C  '

import dds
import pyrealsense2 as rs
if log.is_debug_on():
    rs.log_to_console( rs.log_severity.debug )
from time import sleep

context = rs.context( { 'dds': { 'enabled': True, 'domain': 123 }} )
only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)

import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'device-broadcaster.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "D435i intrinsics" )
    try:
        remote.run( 'instance = broadcast_device( d435i, d435i.device_info )' )
        n_devs = 0
        for dev in dds.wait_for_devices( context, only_sw_devices ):
            n_devs += 1
        test.check_equal( n_devs, 1 )

        sensors = {sensor.get_info( rs.camera_info.name ) : sensor for sensor in dev.query_sensors()}

        sensor = sensors['Stereo Module']
        for profile in sensor.get_stream_profiles() :
            if profile.stream_type() == rs.stream.depth :
                depth_profile = profile.as_video_stream_profile()
                if depth_profile.width() == 256 and depth_profile.height() == 144 :
                    test.check_equal( depth_profile.get_intrinsics().ppx, 128.2379150390625 )
                    test.check_equal( depth_profile.get_intrinsics().ppy, 69.3431396484375 )
                    test.check_equal( depth_profile.get_intrinsics().fx, 631.3428955078125 )
                    test.check_equal( depth_profile.get_intrinsics().fy, 631.3428955078125 )

                if depth_profile.width() == 848 and depth_profile.height() == 100 :
                    test.check_equal( depth_profile.get_intrinsics().ppx, 424.1576232910156 )
                    test.check_equal( depth_profile.get_intrinsics().ppy, 48.239837646484375 )
                    test.check_equal( depth_profile.get_intrinsics().fx, 418.2646789550781 )
                    test.check_equal( depth_profile.get_intrinsics().fy, 418.2646789550781 )

            if profile.stream_type() == rs.stream.infrared and profile.stream_index() == 1 :
                ir1_profile = profile.as_video_stream_profile()
                if ir1_profile.width() == 1280 and ir1_profile.height() == 720 :
                    test.check_equal( ir1_profile.get_intrinsics().ppx, 640.2379150390625 )
                    test.check_equal( ir1_profile.get_intrinsics().ppy, 357.3431396484375 )
                    test.check_equal( ir1_profile.get_intrinsics().fx, 631.3428955078125 )
                    test.check_equal( ir1_profile.get_intrinsics().fy, 631.3428955078125 )
                    test.check_equal( ir1_profile.get_intrinsics().model, rs.distortion.brown_conrady )
                    test.check_equal_lists( ir1_profile.get_intrinsics().coeffs, [0.0,0.0,0.0,0.0,0.0] )

                if ir1_profile.width() == 1280 and ir1_profile.height() == 800 :
                    test.check_equal( ir1_profile.get_intrinsics().ppx, 640.2379150390625 )
                    test.check_equal( ir1_profile.get_intrinsics().ppy, 397.3431396484375 )
                    test.check_equal( ir1_profile.get_intrinsics().fx, 631.3428955078125 )
                    test.check_equal( ir1_profile.get_intrinsics().fy, 631.3428955078125 )

                if ir1_profile.width() == 424 and ir1_profile.height() == 240 :
                    test.check_equal( ir1_profile.get_intrinsics().ppx, 212.0788116455078 )
                    test.check_equal( ir1_profile.get_intrinsics().ppy, 119.07991790771484 )
                    test.check_equal( ir1_profile.get_intrinsics().fx, 209.13233947753906 )
                    test.check_equal( ir1_profile.get_intrinsics().fy, 209.13233947753906 )

            if profile.stream_type() == rs.stream.infrared and profile.stream_index() == 2 :
                ir2_profile = profile.as_video_stream_profile()
                if ir1_profile.width() == 480 and ir2_profile.height() == 270 :
                    test.check_equal( ir2_profile.get_intrinsics().ppx, 240.08921813964844 )
                    test.check_equal( ir2_profile.get_intrinsics().ppy, 134.00367736816406 )
                    test.check_equal( ir2_profile.get_intrinsics().fx, 236.7535858154297 )
                    test.check_equal( ir2_profile.get_intrinsics().fy, 236.7535858154297 )

        sensor = sensors['RGB Camera']
        for profile in sensor.get_stream_profiles() :
            if profile.stream_type() == rs.stream.color :
                color_profile = profile.as_video_stream_profile()
                if color_profile.width() == 320 and color_profile.height() == 180 :
                    test.check_equal( color_profile.get_intrinsics().ppx, 161.7417755126953 )
                    test.check_equal( color_profile.get_intrinsics().ppy, 90.47455596923828 )
                    test.check_equal( color_profile.get_intrinsics().fx, 227.0221710205078 )
                    test.check_equal( color_profile.get_intrinsics().fy, 227.1049346923828 )
                    test.check_equal( color_profile.get_intrinsics().model, rs.distortion.inverse_brown_conrady )
                    test.check_equal_lists( color_profile.get_intrinsics().coeffs, [0.0,0.0,0.0,0.0,0.0] )

                if color_profile.width() == 960 and color_profile.height() == 540 :
                    test.check_equal( color_profile.get_intrinsics().ppx, 485.2253112792969 )
                    test.check_equal( color_profile.get_intrinsics().ppy, 271.4236755371094 )
                    test.check_equal( color_profile.get_intrinsics().fx, 681.0665283203125 )
                    test.check_equal( color_profile.get_intrinsics().fy, 681.3148193359375 )

                if color_profile.width() == 1920 and color_profile.height() == 1080:
                    test.check_equal( color_profile.get_intrinsics().ppx, 970.4506225585938 )
                    test.check_equal( color_profile.get_intrinsics().ppy, 542.8473510742188 )
                    test.check_equal( color_profile.get_intrinsics().fx, 1362.133056640625 )
                    test.check_equal( color_profile.get_intrinsics().fy, 1362.629638671875 )

        sensor = sensors['Motion Module']
        for profile in sensor.get_stream_profiles():
            if test.check_equal( profile.stream_type(), rs.stream.motion ):
                gyro_profile = profile.as_motion_stream_profile()
                test.check_equal_lists( gyro_profile.get_motion_intrinsics().data, [[1.0,0.0,0.0,0.0],[0.0,1.0,0.0,0.0],[0.0,0.0,1.0,0.0]] )
                test.check_equal_lists( gyro_profile.get_motion_intrinsics().noise_variances, [0.0,0.0,0.0] )
                test.check_equal_lists( gyro_profile.get_motion_intrinsics().bias_variances, [0.0,0.0,0.0] )
                # There's currently no way to get the accelerometer intrinsics (which are set the same anyway)

        remote.run( 'close_server( instance )' )
    except:
        test.unexpected_exception()
    dev = None
    test.finish()
    #
    #############################################################################################

context = None
test.print_results_and_exit()
