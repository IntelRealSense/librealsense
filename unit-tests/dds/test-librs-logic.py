# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

from rspy import log, test
log.nested = 'C  '

import d435i
import d405
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
    test.start( "Multiple cameras disconnection/reconnection" )
    try:
        remote.run( 'instance = broadcast_device( d435i, d435i.device_info )', timeout=5 )
        n_devs = wait_for_devices()
        test.check_equal( len(n_devs), 1 )
        
        remote.run( 'instance2 = broadcast_device( d405, d405.device_info )', timeout=5 )
        sleep( 2 ) # Wait for device to be received and built, otherwise wait_for_devices will return immediately with d435i only
        n_devs = wait_for_devices()
        test.check_equal( len(n_devs), 2 )
        
        remote.run( 'close_server( instance )', timeout=5 )
        remote.run( 'instance = None', timeout=1 )
        sleep( 1 ) # Give client device time to close
        n_devs = wait_for_devices()
        test.check_equal( len(n_devs), 1 )
        
        remote.run( 'instance3 = broadcast_device( d435i, d435i.device_info )', timeout=5 )
        sleep( 2 ) # Wait for device to be received and built, otherwise wait_for_devices will return immediately with d405 only
        n_devs = wait_for_devices()
        test.check_equal( len(n_devs), 2 )
        
        remote.run( 'close_server( instance2 )', timeout=5 )
        remote.run( 'instance2 = None', timeout=1 )
        sleep( 1 ) # Give client device time to close
        n_devs = wait_for_devices()
        test.check_equal( len(n_devs), 1 )
        
        remote.run( 'close_server( instance3 )', timeout=5 )
        sleep( 1 ) # Give client device time to close
        n_devs = wait_for_devices()
        test.check_equal( n_devs, None )
    except:
        test.unexpected_exception()
    dev = None
    test.finish()
    #
    #############################################################################################

context = None
test.print_results_and_exit()
