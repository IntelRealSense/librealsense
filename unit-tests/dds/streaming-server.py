# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test
import json
from time import sleep

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "streaming-server" )



class flexible_topic:
    """
    Just to enable simple one-line syntax:
        flexible_topic( "blah" ).write( '{"field" : 1}' )
    """

    def __init__( self, name, qos = None ):
        if type(name) is str:
            self.handle = dds.flexible_msg.create_topic( participant, name )
        elif type(name) is dds.topic:
            self.handle = name
        else:
            raise RuntimeError( "invalid 'name' argument: " + type(name) )
        self.n_readers = 0
        self.writer = dds.topic_writer( self.handle )
        self.writer.on_publication_matched( self._on_publication_matched )
        self.writer.run( qos or dds.topic_writer.qos() )

    def _on_publication_matched( self, writer, d_readers ):
        log.d( "on_publication_matched", d_readers )
        self.n_readers += d_readers

    def wait_for_reader( self, n_readers = 1, timeout = 3.0 ):
        while self.n_readers < n_readers:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( "timed out waiting for reader" )

    def write( self, json_string ):
        msg = dds.flexible_msg( json_string )
        log.d( "writing", msg )
        msg.write_to( self.writer )

    def stop( self ):
        self.writer = self.handle = None



# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
