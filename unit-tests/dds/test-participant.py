# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrs_dds_server as client
from rspy import log, test


client.debug( True, 'cli' )
log.nested = 'cli'


def run_server( server_script ):
    import sys
    cmd = [sys.executable]
    #
    # PYTHON FLAGS
    #
    #     -u     : force the stdout and stderr streams to be unbuffered; same as PYTHONUNBUFFERED=1
    # With buffering we may end up losing output in case of crashes! (in Python 3.7 the text layer of the
    # streams is unbuffered, but we assume 3.6)
    cmd += ['-u']
    #
    if sys.flags.verbose:
        cmd += ["-v"]
    #
    import os.path
    if not os.path.isabs( server_script ):
        thisdir = os.path.dirname( os.path.realpath( __file__ ))
        server_script = os.path.join( thisdir, server_script )
    cmd += [server_script]
    #
    if log.is_debug_on():
        cmd += ['--debug']
    #
    if log.is_color_on():
        cmd += ['--color']
    #
    cmd += ['--nested', 'svr']
    #
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



#############################################################################################
#
test.start( "Checking we can detect another participant..." )

server_added = False
def on_participant_added( guid, name ):
    global server_added
    if name == 'test-participant-server':
        server_added = True
server_removed = False
def on_participant_removed( guid, name ):
    global server_removed
    if name == 'test-participant-server':
        server_removed = True

participant = client.participant()
participant.init( 123, "test-participant-client" )
listener = participant.create_listener()
listener.on_participant_added( on_participant_added )
listener.on_participant_removed( on_participant_removed )

run_server( 'participant-server.py' )

test.check( server_added )
test.check( server_removed )

listener = None
participant = None
test.finish()
#
#############################################################################################
test.print_results_and_exit()
