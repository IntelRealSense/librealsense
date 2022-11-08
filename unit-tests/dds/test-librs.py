# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
log.nested = 'C  '

import d435i
import pyrealsense2 as rs
rs.log_to_console( rs.log_severity.debug )
from time import sleep

context = rs.context( '{"dds-domain":123}' )
only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)


def wait_for_devices( tries = 3 ):
    """
    Since DDS devices may take time to be recognized and then initialized, we try over time:
    """
    while tries:
        devices = context.query_devices( only_sw_devices )
        if len(devices) > 0:
            return devices
        tries -= 1
        sleep( 1 )
    


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'device-broadcaster.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Test D435i..." )
    try:
        remote.run( 'instance = broadcast_device( d435i, d435i.device_info )', timeout=5 )
        n_devs = 0
        for dev in wait_for_devices():
            n_devs += 1
        test.check_equal( n_devs, 1 )
        test.check_equal( dev.get_info( rs.camera_info.name ), d435i.device_info.name )
        test.check_equal( dev.get_info( rs.camera_info.serial_number ), d435i.device_info.serial )
        test.check_equal( dev.get_info( rs.camera_info.physical_port ), d435i.device_info.topic_root )
        sensors = {sensor.get_info( rs.camera_info.name ) : sensor for sensor in dev.query_sensors()}
        test.check_equal( len(sensors), 3 )
        if test.check( 'Stereo Module' in sensors ):
            sensor = sensors.get('Stereo Module')
            test.check_equal( len(sensor.get_stream_profiles()), len(d435i.depth_stream_profiles())+2*len(d435i.ir_stream_profiles()) )
        if test.check( 'RGB Camera' in sensors ):
            sensor = sensors['RGB Camera']
            test.check_equal( len(sensor.get_stream_profiles()), len(d435i.color_stream_profiles()) )
        if test.check( 'Motion Module' in sensors ):
            sensor = sensors['Motion Module']
            test.check_equal( len(sensor.get_stream_profiles()), len(d435i.accel_stream_profiles())+len(d435i.gyro_stream_profiles()) )
        remote.run( 'close_server( instance )', timeout=5 )
    except:
        test.unexpected_exception()
    dev = None
    test.finish()
    #
    #############################################################################################


context = None
test.print_results_and_exit()
