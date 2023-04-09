# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
log.nested = 'C  '

import d435i
import d405
import dds
import pyrealsense2 as rs
rs.log_to_console( rs.log_severity.debug )
from time import sleep

only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)

import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'device-broadcaster.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Multiple cameras disconnection/reconnection" )
    try:
        remote.run( 'instance = broadcast_device( d435i, d435i.device_info )' )
        remote.run( 'instance2 = broadcast_device( d405, d405.device_info )' )

        #Create context after remote device is ready to test discovery on start-up
        #Later on, other connections will test discovery using the callback functions
        context = rs.context( '{"dds-domain":123}' )
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        test.check_equal( len(n_devs), 2 )

        remote.run( 'instance3 = broadcast_device( d455, d455.device_info )' )
        sleep( 1 ) # Wait for device to be received and built, otherwise wait_for_devices will return immediately with previous devices
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        test.check_equal( len(n_devs), 3 )

        remote.run( 'close_server( instance )' )
        remote.run( 'instance = None', timeout=1 )
        sleep( 0.5 ) # Give client device time to close
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        test.check_equal( len(n_devs), 2 )

        remote.run( 'instance4 = broadcast_device( d435i, d435i.device_info )' )
        sleep( 1 ) # Wait for device to be received and built, otherwise wait_for_devices will return immediately with previous devices
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        test.check_equal( len(n_devs), 3 )

        remote.run( 'close_server( instance2 )' )
        remote.run( 'instance2 = None', timeout=1 )
        sleep( 0.5 ) # Give client device time to close
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        test.check_equal( len(n_devs), 2 )

        remote.run( 'close_server( instance3 )' )
        remote.run( 'instance2 = None', timeout=1 )
        sleep( 0.5 ) # Give client device time to close
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        test.check_equal( len(n_devs), 1 )

        remote.run( 'close_server( instance4 )' )
        n_devs = dds.wait_for_devices( context, only_sw_devices )
        sleep( 0.5 ) # Give client device time to close
        test.check_equal( n_devs, None )
    except:
        test.unexpected_exception()
    dev = None
    test.finish()
    #
    #############################################################################################

context = None
test.print_results_and_exit()
