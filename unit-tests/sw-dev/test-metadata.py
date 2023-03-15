# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test
import sw


def frame_metadata_values():
    return [rs.frame_metadata_value.__getattribute__( rs.frame_metadata_value, k )
            for k, v in rs.frame_metadata_value.__dict__.items()
            if str(v).startswith('frame_metadata_value.')]


#############################################################################################
#
test.start( "Nothing supported by default" )
try:
    with sw.sensor( "Stereo Module" ) as sensor:
        depth = sensor.video_stream( "Depth", rs.stream.depth, rs.format.z16 )
        #ir = sensor.video_stream( "Infrared", rs.stream.infrared, rs.format.y8 )
        sensor.start( depth )

        # Publish a frame
        f = sensor.publish( depth.frame() )

        for md in frame_metadata_values():
            test.check_false( f.supports_frame_metadata( md ))
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Set one value" )
try:
    with sw.sensor( "Stereo Module" ) as sensor:
        depth = sensor.video_stream( "Depth", rs.stream.depth, rs.format.z16 )
        #ir = sensor.video_stream( "Infrared", rs.stream.infrared, rs.format.y8 )
        sensor.start( depth )

        # Metadata is set on the sensor, not the software frame
        sensor._handle.set_metadata( rs.frame_metadata_value.white_balance, 0xbaad )
    
        # Publish the frame
        f = sensor.publish( depth.frame() )

        # Frame should have received the metadata from the sensor
        test.check( f.supports_frame_metadata( rs.frame_metadata_value.white_balance ))
        test.check_equal( f.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Post-frame metadata does not affect frame" )
try:
    with sw.sensor( "Stereo Module" ) as sensor:
        depth = sensor.video_stream( "Depth", rs.stream.depth, rs.format.z16 )
        sensor.start( depth )

        sensor._handle.set_metadata( rs.frame_metadata_value.white_balance, 0xbaad )
    
        f = sensor.publish( depth.frame() )

        sensor._handle.set_metadata( rs.frame_metadata_value.white_balance, 0xf00d )

        test.check_equal( f.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Metadata is kept from frame to frame" )
try:
    with sw.sensor( "Stereo Module" ) as sensor:
        depth = sensor.video_stream( "Depth", rs.stream.depth, rs.format.z16 )
        sensor.start( depth )

        sensor._handle.set_metadata( rs.frame_metadata_value.white_balance, 0xbaad )
        f1 = sensor.publish( depth.frame() )
        test.check_equal( f1.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )

        f2 = sensor.publish( depth.frame() )
        test.check_equal( f2.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )

except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Prev frame does not pick up new data from new frame" )
try:
    with sw.sensor( "Stereo Module" ) as sensor:
        depth = sensor.video_stream( "Depth", rs.stream.depth, rs.format.z16 )
        sensor.start( depth )

        sensor._handle.set_metadata( rs.frame_metadata_value.white_balance, 0xbaad )
        f1 = sensor.publish( depth.frame() )

        sensor._handle.set_metadata( rs.frame_metadata_value.actual_fps, 0xf00d )
        f2 = sensor.publish( depth.frame() )

        test.check_equal( f2.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )
        test.check_equal( f2.get_frame_metadata( rs.frame_metadata_value.actual_fps ), 0xf00d )

        test.check_equal( f1.supports_frame_metadata( rs.frame_metadata_value.actual_fps ), False )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
test.print_results_and_exit()

