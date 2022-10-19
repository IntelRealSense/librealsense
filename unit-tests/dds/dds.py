# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds
from rspy import log, test


def run_server( server_script ):
    import os.path
    cmd = test.nested_cmd( server_script, cwd = os.path.dirname(os.path.realpath(__file__)) )
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

