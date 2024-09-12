# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as client
from rspy import log, test


import dds
client.debug( log.is_debug_on(), 'C  ' )
log.nested = 'C  '


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

dds.run_server( 'participant-server.py', nested_indent="  S" )

test.check( server_added )
test.check( server_removed )

listener = None
participant = None
test.finish()
#
#############################################################################################
test.print_results_and_exit()
