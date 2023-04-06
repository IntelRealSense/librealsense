# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds
from rspy import log, test
from time import sleep

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


def wait_for_devices( context, mask, timeout=3 ):
    """
    Since DDS devices may take time to be recognized and then initialized, we try over time:
    """
    if 'gha' in test.context:
        timeout *= 3  # GHA VMs have only 2 cores
    while timeout:
        devices = context.query_devices( mask )
        if len(devices) > 0:
            return devices
        timeout -= 1
        log.d( 'waiting for device...' )
        sleep( 1 )
    log.d( 'timed out' )
