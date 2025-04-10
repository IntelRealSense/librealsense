# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022-4 Intel Corporation. All Rights Reserved.

from rspy import log as _log
from rspy import test as _test
import threading as _threading
from rspy.timer import Timer as _Timer
from pyrealsense2 import *


only_sw_devices = int(product_line.sw_only) | int(product_line.any)


_devices_updated = _threading.Event()
def _device_change_callback( info ):
    """
    Called when librealsense detects a device change
    """
    global _devices_updated
    _devices_updated.set()


def wait_for_devices( context, mask=product_line.any, n=1, timeout=3, throw=None ):
    """
    Since DDS devices may take time to be recognized and then initialized, we try over time:

    :param n:
        If a float ('2.') then an exact match is expected (3 devices will not fit 2.) and throw
        is on by default; otherwise, the minimum number of devices acceptable (throw is off)
    """
    if 'gha' in _test.context:
        timeout *= 3  # GHA VMs have only 2 cores
    exact = isinstance( n, float )
    if exact:
        n = int(n)
        if throw is None:
            throw = True
    #
    timer = _Timer( timeout )
    devices = context.query_devices( mask )
    if len(devices) >= n:
        if not exact:
            return devices
        if len(devices) == n:
            if n == 1:
                return devices[0]
            return devices
    #
    context.set_devices_changed_callback( _device_change_callback )
    global _devices_updated
    #
    while not timer.has_expired():
        #
        _log.d( f'{len(devices)} device(s) detected; waiting...' )
        _devices_updated.clear()
        if not _devices_updated.wait( timer.time_left() ):
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
    _log.d( f'timeout waiting for {n} device(s)' )
    if exact and n == 1:
        if devices:
            return device[0]
        return None
    return devices
