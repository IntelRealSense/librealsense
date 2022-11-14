# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test
import json

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "stream-server" )



class Topic:
    """
    Just to enable simple one-line syntax:
        Topic( "blah" ).write( '{"field" : 1}' )
    """

    def __init__( self, name, qos = None ):
        if type(name) is str:
            self.handle = dds.flexible_msg.create_topic( participant, name )
        elif type(name) is dds.topic:
            self.handle = name
        else:
            raise RuntimeError( "invalid 'name' argument: " + type(name) )
        self.writer = dds.topic_writer( self.handle )
        self.writer.run( qos or dds.topic_writer.qos( dds.reliability.reliable, dds.durability.transient_local ) )

    def write( self, json_string ):
        dds.flexible_msg( json_string ).write_to( self.writer )



# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
