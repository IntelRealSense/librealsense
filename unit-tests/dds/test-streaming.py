# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test
log.nested = 'C  '

from time import sleep
import json

participant = dds.participant()
participant.init( 123, "test-streaming" )


data = {}
def on_data_available( reader ):
    global data
    topic_name = reader.topic().name()
    if topic_name not in data:
        data[topic_name] = []
    msg = dds.flexible_msg.take_next( reader )
    if not msg:
        log.e( "expected message not received!" )
    else:
        log.d( "received:", msg )
        data[topic_name] += [msg]

def wait_for_data( timeout = 3.0 ):
    global data
    while not data:
        timeout -= 0.25
        if timeout > 0:
            sleep( 0.25 )
        else:
            raise RuntimeError( "timed out waiting for data" )


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
        remote.run( 'topic = Topic( "blah" )', timeout=5 )
        # NOTE: technically, we could combine everything into a single command:
        #      Topic("blah").write( ... )
        # and it would do everything, including destroy the Topic instance with the writer etc.
        # This doesn't work! If the writer is destroyed before we've had a chance to get the message,
        # the message will get lost...
        topic = dds.flexible_msg.create_topic( participant, "blah" )
        reader = dds.topic_reader( topic )
        reader.on_data_available( on_data_available )
        reader.run( dds.topic_reader.qos() )
        assert( not data )
        remote.run( 'topic.write( """' + sample1_json + '""" )', timeout=5 )
        wait_for_data()
        if test.check_equal( len(data), 1 ):
            if test.check_equal( next( iter( data )), 'blah' ):
                test.check_equal( len( data['blah'] ), 1 )
                test.check_equal( json.loads( data['blah'][0].json_data() ), sample1 )
    except:
        test.unexpected_exception()
    remote.run( 'topic = None', timeout=5 )
    data = []
    test.finish()
    #
    #############################################################################################


test.print_results_and_exit()
