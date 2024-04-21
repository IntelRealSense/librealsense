# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( log.is_debug_on(), log.nested )

import d435i


participant = dds.participant()
participant.init( 123, "server" )


# set up a server device with a single color stream
device_server = dds.device_server( participant, d435i.device_info.topic_root )

color_stream = dds.color_stream_server( "Color",  "RGB Camera" )
color_stream.enable_metadata()  # not there in d435i by default
color_stream.init_profiles( d435i.color_stream_profiles(), 0 )
color_stream.init_options( [] )
color_stream.set_intrinsics( d435i.color_stream_intrinsics() )

def on_control( server, id, control, reply ):
    # the control has already been output to debug by the calling code, as will the reply
    return True  # otherwise the control will be flagged as error

device_server.on_control( on_control )
device_server.init( [color_stream], [], {} )


def broadcast():
    global device_server
    device_server.broadcast( d435i.device_info )


def new_image( width, height, bpp, timestamp_as_ns = None ):
    i = dds.message.image()
    i.width = width
    i.height = height
    i.data = bytearray( width * height * bpp )
    if timestamp_as_ns is not None:
        i.timestamp = dds.time.from_ns( timestamp_as_ns )
    return i


def publish_image( img, timestamp ):
    img.timestamp = timestamp
    color_stream.publish_image( img )


# From here down, we're in "interactive" mode (see test-metadata.py)
# ...
