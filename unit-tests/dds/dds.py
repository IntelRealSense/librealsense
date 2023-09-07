# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds
from rspy import log, test
from time import sleep
import threading
from rspy.timer import Timer


def run_server( server_script, nested_indent = 'svr' ):
    import os.path
    cmd = test.nested_cmd( server_script, cwd = os.path.dirname(os.path.realpath(__file__)), nested_indent = nested_indent )
    import subprocess, time
    log.d( 'running:', cmd )
    start_time = time.time()
    try:
        log.debug_indent()
        rv = subprocess.run( cmd,
                             stdout=None,
                             stderr=subprocess.STDOUT,
                             universal_newlines=True,
                             timeout=5,
                             check=True )
        result = rv.stdout
        if not result:
            result = []
        else:
            result = result.split( '\n' )
        return result
    finally:
        log.debug_unindent()
        run_time = time.time() - start_time
        log.d( "server took", run_time, "seconds" )


devices_updated = threading.Event()
def _device_change_callback( info ):
    """
    Called when librealsense detects a device change
    """
    global devices_updated
    devices_updated.set()


def wait_for_devices( context, mask, n=1, timeout=3, throw=None ):
    """
    Since DDS devices may take time to be recognized and then initialized, we try over time:

    :param n:
        If a float ('2.') then an exact match is expected (3 devices will not fit 2.) and throw
        is on by default; otherwise, the minimum number of devices acceptable (throw is off)
    """
    if 'gha' in test.context:
        timeout *= 3  # GHA VMs have only 2 cores
    exact = isinstance( n, float )
    if exact:
        n = int(n)
        if throw is None:
            throw = True
    #
    timer = Timer( timeout )
    devices = context.query_devices( mask )
    if len(devices) >= n:
        if not exact or len(devices) == n:
            return devices
    #
    context.set_devices_changed_callback( _device_change_callback )
    global devices_updated
    #
    while not timer.has_expired():
        #
        log.d( f'{len(devices)} device(s) detected; waiting...' )
        devices_updated.clear()
        if not devices_updated.wait( timer.time_left() ):
            break
        #
        devices = context.query_devices( mask )
        if len(devices) >= n:
            if not exact:
                return devices
            if len(devices) == n:
                if n == 1:
                    return devices[0]
                return devices
    if throw:
        raise TimeoutError( f'timeout waiting for {n} device(s)' )
    log.d( f'timeout waiting for {n} device(s)' )
    if exact and n == 1:
        if devices:
            return device[0]
        return None
    return devices
