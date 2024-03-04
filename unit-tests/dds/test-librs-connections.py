# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries:gha 2

from rspy import log, test
log.nested = 'C  '

import d435i
import d405
import dds
import pyrealsense2 as rs
if log.is_debug_on():
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
    with test.closure( "Start two devices", on_fail=test.ABORT ):
        remote.run( 'instance = broadcast_device( d435i, d435i.device_info )' )
        remote.run( 'instance2 = broadcast_device( d405, d405.device_info )' )

        # Create context after remote device is ready to test discovery on start-up
        context = rs.context( { 'dds': { 'enabled': True, 'domain': 123, 'participant': 'librs' }} )
        # The DDS devices take time to be recognized and we just created the context; we
        # should not see them yet!
        #test.check( len( context.query_devices( only_sw_devices )) != 2 )
        # Wait for them
        dds.wait_for_devices( context, only_sw_devices, n=2. )
    #
    #############################################################################################
    #
    with test.closure( "Start a third", on_fail=test.ABORT ):
        remote.run( 'instance3 = broadcast_device( d455, d455.device_info )' )
        dds.wait_for_devices( context, only_sw_devices, n=3. )
    #
    #############################################################################################
    #
    with test.closure( "Close the first", on_fail=test.ABORT ):
        dds.devices_updated.clear()
        remote.run( 'close_server( instance )' )
        remote.run( 'instance = None', timeout=1 )
        dds.wait_for_devices( context, only_sw_devices, n=2. )
    #
    #############################################################################################
    #
    with test.closure( "Add a fourth", on_fail=test.ABORT ):
        remote.run( 'instance4 = broadcast_device( d435i, d435i.device_info )' )
        dds.wait_for_devices( context, only_sw_devices, n=3. )
    #
    #############################################################################################
    #
    with test.closure( "Close the second", on_fail=test.ABORT ):
        remote.run( 'close_server( instance2 )' )
        remote.run( 'instance2 = None', timeout=1 )
        dds.wait_for_devices( context, only_sw_devices, n=2. )
    #
    #############################################################################################
    #
    with test.closure( "Close the third", on_fail=test.ABORT ):
        remote.run( 'close_server( instance3 )' )
        remote.run( 'instance2 = None', timeout=1 )
        dds.wait_for_devices( context, only_sw_devices, n=1. )
    #
    #############################################################################################
    #
    with test.closure( "Close the last", on_fail=test.ABORT ):
        remote.run( 'close_server( instance4 )' )
        dds.wait_for_devices( context, only_sw_devices, n=0. )
    #
    #############################################################################################

context = None
test.print_results_and_exit()
