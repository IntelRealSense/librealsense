# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrs_dds_server as client
from rspy import log, test


import dds
client.debug( True, 'cli' )
log.nested = 'cli'


#############################################################################################
#
test.start( "Checking we can detect devices..." )

devices_added = 0
def on_device_added( dev ):
    global devices_added
    devices_added += 1
    log.d( 'added', dev )
    global watcher
    watcher.foreach_device( lambda dev: log.d( dev ) )
devices_removed = 0
def on_device_removed( dev ):
    global devices_removed
    devices_removed += 1
    log.d( 'removed', dev )

participant = client.participant()
participant.init( 123, "test-watcher-client" )

watcher = client.device_watcher( participant )
watcher.on_device_added( on_device_added )
watcher.on_device_removed( on_device_removed )
watcher.start()

dds.run_server( 'watcher-server.py' )

test.check_equal( devices_added, 2 )
test.check_equal( devices_removed, 2 )

watcher = None
participant = None
test.finish()
#
#############################################################################################
test.print_results_and_exit()
