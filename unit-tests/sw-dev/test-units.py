# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test
import sw

    
with sw.sensor( "Stereo Module" ) as sensor:
    depth = sensor.video_stream( "Depth", rs.stream.depth, rs.format.z16 )
    sensor.start( depth )

    with test.closure( "By default, frames do not have units" ):
        f = depth.frame()
        test.check_equal( f.depth_units, 0. )

    # Publish it
    f = sensor.publish( f )

    with test.closure( "rs.stream.depth should generate depth-frames", on_fail=test.ABORT ):
        df = rs.depth_frame( f )
        test.check( df )

    with test.closure( "No DEPTH_UNITS; Units should be 0" ):
        test.check_false( sensor.supports( rs.option.depth_units ) )
        test.check_equal( df.get_units(), 0. )
        test.check_equal( df.get_distance( 0, 0 ), 0. )

    with test.closure( "Set the sensor DEPTH_UNITS" ):
        sensor.add_option( rs.option.depth_units, rs.option_range( 0, 1, 0.000001, 0.001 ), True )
        #test.check_equal( sensor.get_option( rs.option.depth_units ), 0.001 )   it's not set yet
        sensor.set_option( rs.option.depth_units, 0.001 )

    with test.closure( "Frame units should not change after the fact" ):
        sensor.set_option( rs.option.depth_units, 0.001 )
        test.check_equal( df.get_units(), 0. )
        test.check_equal( df.get_distance( 0, 0 ), 0. )

    with test.closure( "But if we generate a new frame..." ):
        f = depth.frame()
        test.check_equal( f.depth_units, 0. )

    # Publish it
    f = sensor.publish( f )
    df = rs.depth_frame( f )

    with test.closure( "New frame should pick up DEPTH_UNITS" ):
        test.check_approx_abs( df.get_units(), 0.001, 0.00000001 )
        # sw.py uses 0x69 to fill the buffer, and Z16 is 16-bit so the pixel value should be 0x6969
        # and the units are 0.001, so distance (pixel*units) should be 26.985:
        test.check_approx_abs( df.get_distance( 0, 0 ), 26.985, 0.000001 )


#
#############################################################################################
test.print_results_and_exit()

