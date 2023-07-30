# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
import pyrealsense2 as rs
import dds

if log.is_debug_on():
    rs.log_to_console( rs.log_severity.debug )
log.nested = 'C  '

context = rs.context( { 'dds': { 'domain': 123, 'participant': 'test-formats-conversion' }} )
only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)

import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'formats-conversion-server.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()

    #############################################################################################
    #
    with test.closure( "Test setup", on_fail=test.ABORT ):
        remote.run( 'create_server()' )
        n_devs = 0
        for dev in dds.wait_for_devices( context, only_sw_devices ):
            n_devs += 1
        test.check_equal( n_devs, 1 )
        sensors = {sensor.get_info( rs.camera_info.name ) : sensor for sensor in dev.query_sensors()}
    #
    #############################################################################################
    #
    with test.closure( "Test Y8 conversion"):
        if test.check( 'Y8-sensor' in sensors ):
            sensor = sensors.get('Y8-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 )
            test.check_equal( profiles[0].format(), rs.format.y8 )
            test.check_equal( profiles[0].stream_index(), 0 )
    #
    #############################################################################################
    #
    with test.closure( "Test YUYV conversion"):
        if test.check( 'YUYV-sensor' in sensors ):
            sensor = sensors.get('YUYV-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 2 ) # YUYV can stay YUYV or be converted into RGB8
            test.check_equal( profiles[0].format(), rs.format.yuyv )
            test.check_equal( profiles[1].format(), rs.format.rgb8 )
    #
    #############################################################################################
    #
    with test.closure( "Test UYVY conversion"):
        if test.check( 'UYVY-sensor' in sensors ):
            sensor = sensors.get('UYVY-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 2 ) # UYVY can stay UYVY or be converted into RGB8
            test.check_equal( profiles[0].format(), rs.format.uyvy )
            test.check_equal( profiles[1].format(), rs.format.rgb8 )
    #
    #############################################################################################
    #
    with test.closure( "Test Z16 conversion"):
        if test.check( 'Z16-sensor' in sensors ):
            sensor = sensors.get('Z16-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 ) # Z16 stays Z16, for depth stream type
            test.check_equal( profiles[0].format(), rs.format.z16 )
            test.check_equal( profiles[0].stream_type(), rs.stream.depth )
    #
    #############################################################################################
    #
    with test.closure( "Test motion conversion" ):
        if test.check( 'motion-sensor' in sensors ):
            sensor = sensors.get('motion-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 1 ) # MXYZ stays MXYZ with type based on the dds_stream type
            test.check_equal( profiles[0].stream_type(), rs.stream.motion )
    #
    #############################################################################################
    #
    with test.closure( "Test multiple motion profiles one stream" ):
        if test.check( 'multiple-motion-sensor' in sensors ):
            sensor = sensors.get('multiple-motion-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 4 )
            for i in range( len( profiles ) ):
                test.check_equal( profiles[i].stream_type(), rs.stream.motion )
    #
    #############################################################################################
    #
    with test.closure( "Test multiple color profiles one stream"):
        if test.check( 'multiple-color-sensor' in sensors ):
            sensor = sensors.get('multiple-color-sensor')
            profiles = sensor.get_stream_profiles()

            test.check_equal( len( profiles ), 6 )
            for i in range(3):
                test.check_equal( profiles[i * 2 + 0].format(), rs.format.yuyv )
                test.check_equal( profiles[i * 2 + 1].format(), rs.format.rgb8 )
    #
    #############################################################################################
    #
    with test.closure( "Test multiple depth profiles one stream"):
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
