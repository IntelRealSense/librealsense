# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries 2

from rspy import log, test

import pyrealdds as dds
dds.debug( log.is_debug_on(), 'C  ' )
log.nested = 'C  '

import threading


participant = dds.participant()
participant.init( 123, "client" )


# We listen directly on the device-info topic
device_info_topic = dds.message.device_info.create_topic( participant, dds.topics.device_info )
device_info = dds.topic_reader( device_info_topic )
broadcast_received = threading.Event()
broadcast_devices = []
def on_device_info_available( reader ):
    while True:
        msg = dds.message.flexible.take_next( reader )
        if not msg:
            break
        j = msg.json_data()
        log.d( f'on_device_info_available {j}' )
        di = dds.message.device_info.from_json( j )
        global broadcast_devices
        broadcast_devices.append( di )
    broadcast_received.set()
device_info.on_data_available( on_device_info_available )
device_info.run( dds.topic_reader.qos() )

def detect_broadcast():
    global broadcast_received, broadcast_devices
    broadcast_received.clear()
    broadcast_devices = []

from rspy.stopwatch import Stopwatch
def wait_for_broadcast( count=1, timeout=1 ):
    while timeout > 0:
        sw = Stopwatch()
        if not broadcast_received.wait( timeout ):
            raise TimeoutError( 'timeout waiting for broadcast' )
        if count <= len(broadcast_devices):
            return
        broadcast_received.clear()
        timeout -= sw.get_elapsed()
    if count is None:
        raise TimeoutError( 'timeout waiting broadcast' )
    raise TimeoutError( f'timeout waiting for {count} broadcasts; {len(broadcast_devices)} received' )

class broadcast_expected:
    def __init__( self, n_expected=1, timeout=1 ):
        self._timeout = timeout
        self._n_expected = n_expected
    def __enter__( self ):
        detect_broadcast()
    def __exit__( self, type, value, traceback ):
        if type is None:  # If an exception is thrown, don't do anything
            wait_for_broadcast( count=self._n_expected, timeout=self._timeout )


# Start a watcher, too...
devices_added = 0
def on_device_added( watcher, dev ):
    global devices_added
    devices_added += 1
    log.d( 'watcher detected device added', dev )
    watcher.foreach_device( lambda dev: log.d( dev ) )

devices_removed = 0
def on_device_removed( watcher, dev ):
    global devices_removed
    devices_removed += 1
    log.d( 'watcher detected device removed', dev )

watcher = dds.device_watcher( participant )
watcher.on_device_added( on_device_added )
watcher.on_device_removed( on_device_removed )
watcher.start()


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'watcher-server.py' )
with test.remote( remote_script, nested_indent='  S' ) as remote:
    #############################################################################################
    #
    test.start( "Broadcast first; expect 1" )
    with broadcast_expected():
        remote.run( 'broadcast({ "serial" : "123" })', timeout=5 )
    test.check_equal( len(broadcast_devices), 1 )
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Broadcast second; expect 1" )
    with broadcast_expected():
        remote.run( 'broadcast({ "serial" : "456" })', timeout=5 )
    test.check_equal( len(broadcast_devices), 1 )
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Add another client; expect 2!" )
    with broadcast_expected( 2 ):
        reader_2 = dds.topic_reader( device_info_topic )
        reader_2.run( dds.topic_reader.qos() )
    test.check_equal( len(broadcast_devices), 2 )
    del reader_2
    test.finish()
    #
    #############################################################################################
    #
    test.start( "We should see both in the watcher" )
    test.check_equal( devices_added, 2 )
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Get both removals" )
    remote.run( 'unbroadcast_all()', timeout=5 )
    from time import sleep
    sleep(1)
    test.check_equal( devices_removed, 2 )
    test.finish()
    #
    #############################################################################################

del watcher
del device_info
del participant
test.print_results_and_exit()
