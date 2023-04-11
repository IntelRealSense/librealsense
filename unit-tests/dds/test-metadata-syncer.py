# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test
from time import sleep
from rspy.stopwatch import Stopwatch

dds.debug( True, 'C  ' )
log.nested = 'C  '


participant = dds.participant()
participant.init( 123, "client" )


last_image = None
last_metadata = None
def on_frame_ready( image, metadata ):
    log.d( f'----> frame ready: {image=} {metadata=}' )
    global last_image, last_metadata
    last_image = image
    last_metadata = metadata

dropped_metadata = []
def on_metadata_dropped( key, metadata ):
    log.d( f'----X metadata dropped: {key=} {metadata=}' )
    dropped_metadata.append( key )


def new_syncer( on_frame_ready=on_frame_ready, on_metadata_dropped=on_metadata_dropped ):
    syncer = dds.metadata_syncer()
    syncer.on_frame_ready( on_frame_ready )
    syncer.on_metadata_dropped( on_metadata_dropped )
    global last_image, last_metadata, dropped_metadata
    last_image = last_metadata = None
    dropped_metadata = []
    return syncer


def new_image_( id, width, height, bpp, timestamp=None ):
    i = dds.image_msg()
    i.frame_id = str(id)
    i.width = width
    i.height = height
    i.data = bytearray( width * height * bpp )
    if timestamp is not None:
        i.timestamp = timestamp
    return i
def new_image( i ):
    return new_image_( i, 2, 2, 3 )


def new_metadata( id, timestamp=None ):
    timestamp = timestamp or dds.now()
    md = {
        'frame-id' : str(id),
        'timestamp' : timestamp.to_double(),
    }
    return md


with test.closure( "Enqueue 1 image -> nothing should come out" ):
    syncer = new_syncer()
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue 1 metadata -> nothing should come out" ):
    syncer = new_syncer()
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue many metadata -> nothing out; should get drops" ):
    syncer = new_syncer()
    for i in range( dds.metadata_syncer.max_md_queue_size*2 ):
        syncer.enqueue_metadata( i, new_metadata( i ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), dds.metadata_syncer.max_md_queue_size )

with test.closure( "Enqueue many images -> some out (no md); no drops" ):
    syncer = new_syncer()
    for i in range( dds.metadata_syncer.max_frame_queue_size*3 ):
        syncer.enqueue_frame( i, new_image( i ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, str(i-dds.metadata_syncer.max_frame_queue_size) )
    test.check_equal( last_metadata, None )  # Note: not {}, but None!

with test.closure( "Enqueue image+metadata -> out" ):
    syncer = new_syncer()
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '1' )
    test.check( last_metadata ) and test.check_equal( last_metadata['frame-id'], '1' )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue metadata+image -> out" ):
    syncer = new_syncer()
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '1' )
    test.check( last_metadata ) and test.check_equal( last_metadata['frame-id'], '1' )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue 1 image after some metadata -> drops + match" ):
    syncer = new_syncer()
    for i in range(4):
        syncer.enqueue_metadata( i, new_metadata( i ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_frame( 2, new_image( 2 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '2' )
    test.check( last_metadata ) and test.check_equal( last_metadata['frame-id'], '2' )
    test.check_equal( len(dropped_metadata), 2 )  # 0 and 1

with test.closure( "Enqueue 1 image then earlier metadata -> nothing out; metadata is dropped" ):
    syncer = new_syncer()
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 0, new_metadata( 0 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 1 )  # not going to be any frame for it

with test.closure( "Enqueue 1 image then later metadata -> image out" ):
    syncer = new_syncer()
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 2, new_metadata( 2 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '1' )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue 2 images then matching metadata for the second -> both out" ):
    syncer = new_syncer()
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_frame( 2, new_image( 2 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 2, new_metadata( 2 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '2' )
    test.check( last_metadata ) and test.check_equal( last_metadata['frame-id'], '2' )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue during callback" ):
    def enqueue_during_callback( image, metadata ):
        on_frame_ready( image, metadata )
        id = int(image.frame_id)+1
        syncer.enqueue_frame( id, new_image( id ))
        # Can generate a 'RuntimeError: device or resource busy' and then the 2nd frame won't come out
    syncer = new_syncer( on_frame_ready=enqueue_during_callback )
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '1' )
    test.check( last_metadata ) and test.check_equal( last_metadata['frame-id'], '1' )
    test.check_equal( len(dropped_metadata), 0 )
    # If the enqueue succeeded, a matching metadata will generate a callback
    syncer.enqueue_metadata( 2, new_metadata( 2 ) )
    test.check( last_image ) and test.check_equal( last_image.frame_id, '2' )
    test.check( last_metadata ) and test.check_equal( last_metadata['frame-id'], '2' )
    test.check_equal( len(dropped_metadata), 0 )


test.print_results_and_exit()
