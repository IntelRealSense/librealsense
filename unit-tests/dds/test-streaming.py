# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test

dds.debug( True, 'C  ' )
log.nested = 'C  '

from time import sleep
import json

participant = dds.participant()
participant.init( 123, "test-streaming" )


class flexible_topic:
    """
    Just to enable simple one-line syntax:
        msg = flexible_topic( "blah" ).read()
    """

    def __init__( self, name, qos = None ):
        if type(name) is str:
            self.handle = dds.flexible_msg.create_topic( participant, name )
        elif type(name) is dds.topic:
            self.handle = name
        else:
            raise RuntimeError( "invalid 'name' argument: " + type(name) )
        self.n_writers = 0
        self.data = []
        self.reader = dds.topic_reader( self.handle )
        self.reader.on_data_available( self._on_data_available )
        self.reader.on_subscription_matched( self._on_subscription_matched )
        self.reader.run( qos or dds.topic_reader.qos() )

    def _on_subscription_matched( self, reader, d_writers ):
        log.d( "on_subscription_matched", d_writers )
        self.n_writers += d_writers

    def wait_for_writer( self, timeout = 3.0 ):
        while self.n_writers <= 0:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( "timed out waiting for writer" )

    def _on_data_available( self, reader ):
        topic_name = reader.topic().name()
        log.d( "on_data_available", topic_name )
        msg = dds.flexible_msg.take_next( reader )
        if not msg:
            log.e( "expected message not received!" )
        else:
            log.d( "received:", msg )
            self.data += [msg]

    def read( self, timeout = 3.0 ):
        self.wait_for_data( timeout=timeout )
        return self.data.pop(0)

    def wait_for_data( self, timeout = 3.0 ):
        while not self.data:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( "timed out waiting for data" )

    def empty( self ):
        return not self.data


sample1 = { 'id' : 'test-message', 'content' : { 'int' : 1, 'float' : 2.0, 'array' : [ 1, 2.0, 'hello' ] } }
sample1_json = json.dumps( sample1 )


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'streaming-server.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Test 1..." )
    try:
        remote.run( 'topic = flexible_topic( "blah" )', timeout=5 )
        # NOTE: technically, we could combine everything into a single command:
        #      Topic("blah").write( ... )
        # and it would do everything, including destroy the Topic instance with the writer etc.
        # This doesn't work! If the writer is destroyed before we've had a chance to get the message,
        # the message will get lost...
        topic = flexible_topic( "blah" )
        # We may wait for the data before we even see the writer... so wait for the writer first!
        topic.wait_for_writer()
        # Same for the writer:
        remote.run( 'topic.wait_for_reader()', timeout=5 )
        remote.run( 'topic.write( """' + sample1_json + '""" )', timeout=5 )
        msg = topic.read()
        test.check( topic.empty() )
        test.check_equal( json.loads( msg.json_data() ), sample1 )
    except:
        test.unexpected_exception()
    remote.run( 'topic = None', timeout=5 )
    data = []
    test.finish()
    #
    #############################################################################################


test.print_results_and_exit()
