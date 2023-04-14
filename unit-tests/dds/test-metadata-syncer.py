# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test
from time import sleep
from rspy.stopwatch import Stopwatch
import threading

dds.debug( True )


participant = dds.participant()
participant.init( 123, "client" )


def image_id( image ):
    """
    We use timestamps as keys for the syncer.
    To make them readable, approachable, and simple, we simply encode our "counter" (0-x) as the
    timestamp seconds.
    """
    return image.timestamp.seconds

def md_id( metadata ):
    """
    Same for metadata, which is expressed as a float X.Y; our Y is always 0 and X is the counter
    """
    return int(metadata['timestamp'])

def time_stamp( id ):
    """
    Opposite of image_id().
    """
    return dds.time.from_double( id )


last_image = None
last_metadata = None
def on_frame_ready( image, metadata ):
    global last_image, last_metadata
    log.d( f"{image_id(image):-<4}> {dds.now()} [{threading.get_native_id()}] frame ready: {image=} {metadata=}" )
    test.check( last_image is None  or  last_image.timestamp.seconds < image.timestamp.seconds )
    last_image = image
    last_metadata = metadata

dropped_metadata = []
def on_metadata_dropped( key, metadata ):
    log.d( f'Mdrop {dds.now()} [{threading.get_native_id()}] metadata dropped: {key=} {metadata=}' )
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
    i.width = width
    i.height = height
    i.data = bytearray( width * height * bpp )
    i.timestamp = timestamp or time_stamp( id )
    return i
def new_image( id, timestamp=None ):
    return new_image_( id, 2, 2, 3, timestamp )


def new_metadata( id, timestamp=None ):
    timestamp = timestamp or time_stamp( id )
    md = {
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
    test.check( last_image ) and test.check_equal( image_id( last_image ), i - dds.metadata_syncer.max_frame_queue_size )
    test.check_equal( last_metadata, None )  # Note: not {}, but None!

with test.closure( "Enqueue image+metadata -> out" ):
    syncer = new_syncer()
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check( last_image ) and test.check_equal( image_id( last_image ), 1 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 1 )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue metadata+image -> out" ):
    syncer = new_syncer()
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check( last_image ) and test.check_equal( image_id( last_image ), 1 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 1 )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue 1 image after some metadata -> drops + match" ):
    syncer = new_syncer()
    for i in range(4):
        syncer.enqueue_metadata( i, new_metadata( i ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_frame( 2, new_image( 2 ) )
    test.check( last_image ) and test.check_equal( image_id( last_image ), 2 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 2 )
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
    test.check( last_image ) and test.check_equal( image_id( last_image ), 1 )
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
    test.check( last_image ) and test.check_equal( image_id( last_image ), 2 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 2 )
    test.check_equal( len(dropped_metadata), 0 )

with test.closure( "Enqueue during callback" ):
    def enqueue_during_callback( image, metadata ):
        on_frame_ready( image, metadata )
        id = image_id( image ) + 1
        syncer.enqueue_frame( id, new_image( id ))
        # Can generate a 'RuntimeError: device or resource busy' and then the 2nd frame won't come out
    syncer = new_syncer( on_frame_ready=enqueue_during_callback )
    syncer.enqueue_frame( 1, new_image( 1 ) )
    test.check_equal( last_image, None )
    test.check_equal( last_metadata, None )
    test.check_equal( len(dropped_metadata), 0 )
    syncer.enqueue_metadata( 1, new_metadata( 1 ) )
    test.check( last_image ) and test.check_equal( image_id( last_image ), 1 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 1 )
    test.check_equal( len(dropped_metadata), 0 )
    # If the enqueue succeeded, a matching metadata will generate a callback
    syncer.enqueue_metadata( 2, new_metadata( 2 ) )
    test.check( last_image ) and test.check_equal( image_id( last_image ), 2 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 2 )
    test.check_equal( len(dropped_metadata), 0 )


def two_threads( callback_time, time_between_frames=0.033, n=10 ):
    """
    Generate frames periodically. Enqueue images and metadata from two separate threads just as in
    a DDS client (thread per reader).
    """
    have_frame = threading.Event()
    have_md = threading.Event()
    have_frame.clear()
    have_md.clear()
    frames = []
    mds = []

    def frame_handler( image, metadata ):
        # This is our callback
        nonlocal callback_time
        on_frame_ready( image, metadata )  # for reporting
        sleep( callback_time )
        log.d( f'<{image_id(image):->4} {dds.now()} [{threading.get_native_id()}]' )

    syncer = new_syncer( on_frame_ready=frame_handler )

    def generator_thread():
        # This thread generates the frames
        nonlocal time_between_frames, n
        threadid = threading.get_native_id()
        for i in range(n):
            timestamp = time_stamp( i )
            log.d( f'{i:>5} {dds.now()} [{threadid}] generate @ {timestamp}' )
            image = new_image( i, timestamp )
            md = new_metadata( i, timestamp )
            frames.append( image )
            mds.append( md )
            have_frame.set()
            have_md.set()
            sleep( time_between_frames )

    def frame_thread():
        # This thread enqueues the images
        threadid = threading.get_native_id()
        nonlocal syncer, n
        for i in range(n):
            have_frame.wait()
            image = frames.pop( 0 )
            if not frames:
                have_frame.clear()
            idstr = f'F{image_id(image)}'
            log.d( f'{idstr:>5} {dds.now()} [{threadid}] enqueue {image}' )
            syncer.enqueue_frame( i, image )

    def md_thread():
        # This thread enqueues the metadata
        threadid = threading.get_native_id()
        sleep( 0.005 )
        nonlocal syncer, n
        for i in range(n):
            have_md.wait()
            md = mds.pop( 0 )
            if not mds:
                have_md.clear()
            idstr = f'M{int(md["timestamp"])}'
            log.d( f'{idstr:>5} {dds.now()} [{threadid}] enqueue {md}' )
            syncer.enqueue_metadata( i, md )

    gen_th = threading.Thread( target=generator_thread )
    f_th = threading.Thread( target=frame_thread )
    m_th = threading.Thread( target=md_thread )

    gen_th.start()
    f_th.start()
    m_th.start()
    gen_th.join()
    f_th.join()
    m_th.join()


with test.closure( "Two threads, slow callback -> MD drops & parallel callbacks" ):

    two_threads( callback_time=0.1 )  # longer than time-between-frames, on purpose!

    test.check( last_image ) and test.check_equal( image_id( last_image ), 9 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 9 )
    # On GHA, threading is less "evolved"; on a real CPU we see actual dropped metadata
    #test.check_equal( len(dropped_metadata), 3 )

with test.closure( "Two threads, fast callback -> no drops, no parallel callbacks" ):

    two_threads( callback_time=0.01 )  # shorter than time-between-frames

    test.check( last_image ) and test.check_equal( image_id( last_image ), 9 )
    test.check( last_metadata ) and test.check_equal( md_id( last_metadata ), 9 )
    test.check_equal( len(dropped_metadata), 0 )


test.print_results_and_exit()
