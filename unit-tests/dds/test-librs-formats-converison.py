# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
import pyrealsense2 as rs
import dds

rs.log_to_console( rs.log_severity.debug )
log.nested = 'C  '

context = rs.context( '{"dds-domain":123,"dds-participant-name":"test-formats-conversion"}' )
only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)

import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'formats-conversion-server.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()

    # Tests setup
    remote.run( 'create_server()' )
    n_devs = 0
    for dev in dds.wait_for_devices( context, only_sw_devices ):
        n_devs += 1
    test.check_equal( n_devs, 1 )
    sensors = {sensor.get_info( rs.camera_info.name ) : sensor for sensor in dev.query_sensors()}
    #
    #############################################################################################
    #
    with test.closure( "Test Y8 conversion", on_fail=test.ABORT ):
        if test.check( 'Y8-sensor' in sensors ):
            sensor = sensors.get('Y8-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 ) # Y8 stays Y8, for infrared stream indexed 1
            test.check_equal( profiles[0].format(), rs.format.y8 )
            test.check_equal( profiles[0].stream_index(), 1 )
    #
    #############################################################################################
    #
    with test.closure( "Test Y8I conversion", on_fail=test.ABORT ):
        if test.check( 'Y8I-sensor' in sensors ):
            sensor = sensors.get('Y8I-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 2 ) # Y8I is converted into two Y8 streams
            test.check_equal( profiles[0].format(), rs.format.y8 )
            test.check_equal( profiles[0].stream_index(), 1 )
            test.check_equal( profiles[1].format(), rs.format.y8 )
            test.check_equal( profiles[1].stream_index(), 2 )
    #
    #############################################################################################
    #
    with test.closure( "Test Y12I conversion", on_fail=test.ABORT ):
        if test.check( 'Y12I-sensor' in sensors ):
            sensor = sensors.get('Y12I-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 2 ) # Y12I is converted into two Y162 streams
            test.check_equal( profiles[0].format(), rs.format.y16 )
            test.check_equal( profiles[0].stream_index(), 1 )
            test.check_equal( profiles[1].format(), rs.format.y16 )
            test.check_equal( profiles[1].stream_index(), 2 )
    #
    #############################################################################################
    #
    with test.closure( "Test YUYV conversion", on_fail=test.ABORT ):
        if test.check( 'YUYV-sensor' in sensors ):
            sensor = sensors.get('YUYV-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 7 ) # YUYV is converted into 7 streams - RGB8, RGBA8, BGR8, BGRA8, YUYV, Y16, Y8
            test.check_equal( profiles[0].format(), rs.format.rgb8 )
            test.check_equal( profiles[1].format(), rs.format.rgba8 )
            test.check_equal( profiles[2].format(), rs.format.bgr8 )
            test.check_equal( profiles[3].format(), rs.format.bgra8 )
            test.check_equal( profiles[4].format(), rs.format.yuyv )
            test.check_equal( profiles[5].format(), rs.format.y16 )
            test.check_equal( profiles[6].format(), rs.format.y8 )
    #
    #############################################################################################
    #
    with test.closure( "Test UYVY conversion", on_fail=test.ABORT ):
        if test.check( 'UYVY-sensor' in sensors ):
            sensor = sensors.get('UYVY-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 5 ) # UYVY is converted into 5 streams - RGB8, RGBA8, BGR8, BGRA8, UYVY
            test.check_equal( profiles[0].format(), rs.format.rgb8 )
            test.check_equal( profiles[1].format(), rs.format.rgba8 )
            test.check_equal( profiles[2].format(), rs.format.bgr8 )
            test.check_equal( profiles[3].format(), rs.format.bgra8 )
            test.check_equal( profiles[4].format(), rs.format.uyvy )
    #
    #############################################################################################
    #
    with test.closure( "Test Z16 conversion", on_fail=test.ABORT ):
        if test.check( 'Z16-sensor' in sensors ):
            sensor = sensors.get('Z16-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 ) # Z16 stays Z16, for depth stream type
            test.check_equal( profiles[0].format(), rs.format.z16 )
            test.check_equal( profiles[0].stream_type(), rs.stream.depth )
    #
    #############################################################################################
    #
    with test.closure( "Test accel conversion", on_fail=test.ABORT ):
        if test.check( 'accel-sensor' in sensors ):
            sensor = sensors.get('accel-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 ) # MXYZ stays MXYZ with type based on the dds_stream type
            test.check_equal( profiles[0].format(), rs.format.motion_xyz32f )
            test.check_equal( profiles[0].stream_type(), rs.stream.accel )
    #
    #############################################################################################
    #
    with test.closure( "Test gyro conversion", on_fail=test.ABORT ):
        if test.check( 'gyro-sensor' in sensors ):
            sensor = sensors.get('gyro-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 ) # MXYZ stays MXYZ with type based on the dds_stream type
            test.check_equal( profiles[0].format(), rs.format.motion_xyz32f )
            test.check_equal( profiles[0].stream_type(), rs.stream.gyro )
    #
    #############################################################################################
    #
    with test.closure( "Test multiple accel profiles one stream", on_fail=test.ABORT ):
        if test.check( 'multiple-accel-sensor' in sensors ):
            sensor = sensors.get('multiple-accel-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 4 )
            for i in range( len( profiles ) ):
                test.check_equal( profiles[i].format(), rs.format.motion_xyz32f )
                test.check_equal( profiles[i].stream_type(), rs.stream.accel )
    #
    #############################################################################################
    #
    with test.closure( "Test multiple color profiles one stream", on_fail=test.ABORT ):
        if test.check( 'multiple-color-sensor' in sensors ):
            sensor = sensors.get('multiple-color-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 21 )
            for i in range(3):
                test.check_equal( profiles[i * 7 + 0].format(), rs.format.rgb8 )
                test.check_equal( profiles[i * 7 + 1].format(), rs.format.rgba8 )
                test.check_equal( profiles[i * 7 + 2].format(), rs.format.bgr8 )
                test.check_equal( profiles[i * 7 + 3].format(), rs.format.bgra8 )
                test.check_equal( profiles[i * 7 + 4].format(), rs.format.yuyv )
                test.check_equal( profiles[i * 7 + 5].format(), rs.format.y16 )
                test.check_equal( profiles[i * 7 + 6].format(), rs.format.y8 )
    #
    #############################################################################################
    #
    with test.closure( "Test multiple depth profiles one stream", on_fail=test.ABORT ):
        if test.check( 'multiple-depth-sensor' in sensors ):
            sensor = sensors.get('multiple-depth-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 5 )
            for i in range(5):
                test.check_equal( profiles[i].format(), rs.format.z16 )
    #
    #############################################################################################
    #
    # Tests tear down
    remote.run( 'close_server()' )
    dev = None

context = None

test.print_results_and_exit()
