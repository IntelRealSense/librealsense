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


def ns_as_ms( ns ):
    return str(ns / 1000000.) + "ms"


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
        self.n_writers += d_writers
        log.d( "on_subscription_matched", reader.topic().name(), d_writers, '=', self.n_writers )

    def wait_for_writers( self, n_writers = 1, timeout = 3.0 ):
        """
        Wait until a writer/publisher has our topic open
        """
        while self.n_writers != n_writers:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( f"timed out waiting for {n_writers} writers" )

    def _on_data_available( self, reader ):
        topic_name = reader.topic().name()
        log.d( "on_data_available", topic_name )
        sample = dds.sample_info()
        got_something = False
        while True:
            msg = dds.flexible_msg.take_next( reader, sample )
            if not msg:
                if not got_something:
                    log.e( "expected message not received!" )
                break
            log.d( "received", '@T+' + ns_as_ms(sample.reception_timestamp()-sample.source_timestamp()), msg )
            self.data += [msg]
            got_something = True

    def read( self, timeout = 3.0 ):
        """
        :param timeout: if empty, wait for this many seconds then raise an error
        :return: the next sample read
        """
        self.wait_for_data( timeout=timeout )
        return self.data.pop(0)

    def wait_for_data( self, timeout = 3.0 ):
        """
        Wait until data is available or timeout seconds. If still none, raises an error.
        """
        while not self.data:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( "timed out waiting for data" )

    def empty( self ):
        """
        :return: True if no data is available
        """
        return not self.data


sample1 = { 'id' : 'test-message', 'n' : 1, 'content' : { 'int' : 1, 'float' : 2.0, 'array' : [ 1, 2.0, 'hello' ] } }
sample1_json = json.dumps( sample1 )
sample2 = { 'id' : 'test-message-2', 'n' : 2 }
sample2_json = json.dumps( sample2 )
sample3 = { 'id' : 'test-message', 'n' : 3, 'hmm' : [] }
sample3_json = json.dumps( sample3 )


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'streaming-server.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Test 1W:1R, 1 message..." )
    try:
        remote.run( 'topic = flexible_topic( "1w1r" )', timeout=5 )
        # NOTE: technically, we could combine everything into a single command:
        #      Topic("blah").write( ... )
        # and it would do everything, including destroy the Topic instance with the writer etc.
        # This doesn't work! If the writer is destroyed before we've had a chance to get the message,
        # the message will get lost...
        topic = flexible_topic( "1w1r" )
        # We may wait for the data before we even see the writer... so wait for the writer first!
        topic.wait_for_writers()
        # Same for the writer:
        remote.run( 'topic.wait_for_reader()', timeout=5 )
        remote.run( 'topic.write( """' + sample1_json + '""" )', timeout=5 )
        msg = topic.read()
        test.check( topic.empty() )
        test.check_equal( json.loads( msg.json_data() ), sample1 )
    except:
        test.unexpected_exception()
    remote.run( 'topic.stop()', timeout=5 )
    topic.wait_for_writers( 0 )
    topic = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test 1W:1R, N messages..." )
    try:
        remote.run( 'topic = flexible_topic( "1w1rNm" )', timeout=5 )
        topic = flexible_topic( "1w1rNm" )
        topic.wait_for_writers()
        remote.run( 'topic.wait_for_reader()', timeout=5 )
        remote.run( 'topic.write( """' + sample1_json + '""" )', timeout=5 )
        remote.run( 'topic.write( """' + sample2_json + '""" )', timeout=5 )
        remote.run( 'topic.write( """' + sample3_json + '""" )', timeout=5 )
        msg1 = topic.read()
        msg2 = topic.read()
        msg3 = topic.read()
        test.check( topic.empty() )
        test.check_equal( json.loads( msg1.json_data() ), sample1 )
        test.check_equal( json.loads( msg2.json_data() ), sample2 )
        test.check_equal( json.loads( msg3.json_data() ), sample3 )
    except:
        test.unexpected_exception()
    remote.run( 'topic.stop()', timeout=5 )
    topic.wait_for_writers( 0 )
    topic = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test 1W:2R, one message..." )
    try:
        remote.run( 'topic = flexible_topic( "1w2r" )', timeout=5 )
        r1 = flexible_topic( "1w2r" )
        r2 = flexible_topic( r1.handle )
        r1.wait_for_writers()
        r2.wait_for_writers()
        remote.run( 'topic.wait_for_reader( 2 )', timeout=5 )
        remote.run( 'topic.write( """' + sample1_json + '""" )', timeout=5 )
        msg1 = r1.read()
        msg2 = r2.read()
        test.check( r1.empty() )
        test.check( r2.empty() )
        test.check_equal( json.loads( msg1.json_data() ), sample1 )
        test.check_equal( json.loads( msg2.json_data() ), sample1 )
    except:
        test.unexpected_exception()
    remote.run( 'topic.stop()', timeout=5 )
    r1.wait_for_writers( 0 )
    r2.wait_for_writers( 0 )
    r1 = r2 = None
    test.finish()
    #
    #############################################################################################


test.print_results_and_exit()
