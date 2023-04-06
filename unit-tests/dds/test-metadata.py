# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test
dds.debug( True )
from time import sleep

participant = dds.participant()
participant.init( 123, "test-metadata" )

# set up a server device
import d435i
device_server = dds.device_server( participant, d435i.device_info.topic_root )
color_stream = dds.color_stream_server( "Color",  "RGB Camera" )
color_stream.enable_metadata()  # not there in d435i by default
color_stream.init_profiles( d435i.color_stream_profiles(), 0 )
color_stream.init_options( [] )
color_stream.set_intrinsics( d435i.color_stream_intrinsics() )
device_server.init( [color_stream], [], {} )

# set up the client device and keep all its streams - this is connected directly and we can get notifications on it!
device_direct = dds.device( participant, participant.create_guid(), d435i.device_info )
device_direct.run( 1000 )  # this will throw if something's wrong
test.check( device_direct.is_running(), abort_if_failed=True )
for stream_direct in device_direct.streams():
    pass  # should be only one
topic_name = 'rt/' + d435i.device_info.topic_root + '_' + stream_direct.name()

import threading
image_received = threading.Event()
image_content = []
def on_image_available( stream, image ):
    log.d( f'-----> image {image}')
    image_content.append( image )
    image_received.set()

stream_direct.on_data_available( on_image_available )
stream_direct.open( topic_name, dds.subscriber( participant ) )
stream_direct.start_streaming()

def detect_image():
    image_content.clear()
    image_received.clear()

def wait_for_image( timeout=1, count=None ):
    while timeout > 0:
        sw = Stopwatch()
        if not image_received.wait( timeout ):
            raise TimeoutError( 'timeout waiting for image' )
        if count is None  or  count <= len(image_content):
            return
        image_received.clear()
        timeout -= sw.get_elapsed()
    if count is None:
        raise TimeoutError( 'timeout waiting for image' )
    raise TimeoutError( f'timeout waiting for {count} images; {len(image_content)} received' )

class image_expected:
    def __init__( self, timeout=1, count=None ):
        self._timeout = timeout
        self._count = count
    def __enter__( self ):
        detect_image()
    def __exit__( self, type, value, traceback ):
        if type is None:  # If an exception is thrown, don't do anything
            wait_for_image( timeout=self._timeout, count=self._count )


from rspy.stopwatch import Stopwatch
metadata_received = threading.Event()
metadata_content = []

def on_metadata_available( device, md ):
    log.d( f'-----> metadata {md}')
    metadata_content.append( md )
    metadata_received.set()

device_direct.on_metadata_available( on_metadata_available )

def detect_metadata():
    metadata_content.clear()
    metadata_received.clear()

def wait_for_metadata( timeout=1, count=None ):
    while timeout > 0:
        sw = Stopwatch()
        if not metadata_received.wait( timeout ):
            raise TimeoutError( 'timeout waiting for metadata' )
        if count is None  or  count <= len(metadata_content):
            return
        metadata_received.clear()
        timeout -= sw.get_elapsed()
    if count is None:
        raise TimeoutError( 'timeout waiting for metadata' )
    raise TimeoutError( f'timeout waiting for {count} metadata; {len(metadata_content)} received' )

class metadata_expected:
    def __init__( self, expected_md=None, timeout=1, count=None ):
        self._md = expected_md
        self._timeout = timeout
        self._count = count
    def __enter__( self ):
        detect_metadata()
    def __exit__( self, type, value, traceback ):
        if type is None:  # If an exception is thrown, don't do anything
            wait_for_metadata( timeout=self._timeout, count=self._count )
            if self._md is not None:
                if test.check( len(metadata_content), description = 'Expected metadata but got none' ):
                    test.check_equal( metadata_content[0], self._md )

#############################################################################################
#
test.start( "No librs syncer; direct from server" )
try:
    md = { 'stream-name' : 'Color', 'invalid-metadata' : True }
    with metadata_expected( md ):
        device_server.publish_metadata( md )
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Broadcast the device" )  # otherwise librs won't see it
broadcaster = dds.device_broadcaster( participant )
broadcaster.run()
broadcaster.add_device( d435i.device_info )
test.finish()
#
#############################################################################################
#
test.start( "Initialize librs device" )
import pyrealsense2 as rs
rs.log_to_console( rs.log_severity.debug )
from dds import wait_for_devices
context = rs.context( '{"dds-domain":123,"dds-participant-name":"librs"}' )
only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)
devices = wait_for_devices( context, only_sw_devices )
test.check( devices is not None, abort_if_failed=True )
test.check_equal( len(devices), 1, abort_if_failed=True )
device = devices[0]
del devices
sensors = device.sensors
test.check_equal( len(sensors), 1, abort_if_failed=True )
sensor = sensors[0]
test.check_equal( sensor.get_info( rs.camera_info.name ), 'RGB Camera', abort_if_failed=True )
del sensors
profile = rs.video_stream_profile( sensor.get_stream_profiles()[0] )  # take the first one
log.d( f'using profile {profile}')
sensor.open( [profile] )
queue = rs.frame_queue( 100 )
sensor.start( queue )
test.finish()
#
#############################################################################################
#
test.start( "Metadata alone should not come out" )
try:
    with metadata_expected( count=20 ):
        for i in range(20):
            device_server.publish_metadata({ 'stream-name' : 'Color', 'header' : { 'frame-id' : str(i) }, 'metadata' : {} })
    sleep( 0.25 )  # plus some extra for librs...
    test.check_false( queue.poll_for_frame() )  # we didn't send any images, shouldn't get any frames!
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.start( "Metadata after an image" )
try:
    timestamp = dds.now()
    img = dds.image_msg()
    img.width = profile.width()
    img.height = profile.height()
    img.data = bytearray( img.width * img.height * profile.bytes_per_pixel() )
    img.timestamp = timestamp
    i += 1
    color_stream.start_streaming( dds.stream_format.from_rs2( profile.format() ), img.width, img.height )
    # It will take the image a lot longer to get anywhere than the metadata
    with image_expected():
        color_stream.publish_image( img, i )
    sleep( 0.25 )  # plus some extra for librs...
    test.check_false( queue.poll_for_frame() )  # the image should still be pending in the syncer
    with metadata_expected():
        device_server.publish_metadata({
            'stream-name' : 'Color',
            'header' : {'frame-id':str(i), 'timestamp':timestamp.to_double(), 'timestamp-domain':int(rs.timestamp_domain.system_time) },
            'metadata': {
                "White Balance" : 0xbaad
                }
            })
    f = queue.wait_for_frame( 250 )  # A frame should now be available
    log.d( '---->', f )
    if test.check( f ) and test.check_equal( f.get_frame_number(), 20 ):
        test.check_equal( f.get_timestamp(), timestamp.to_double() )
        test.check_false( f.supports_frame_metadata( rs.frame_metadata_value.actual_fps ) )
        if test.check( f.supports_frame_metadata( rs.frame_metadata_value.white_balance ) ):
            test.check_equal( f.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )
    test.check_false( queue.poll_for_frame() )  # the image should still be pending in the syncer
except:
    test.unexpected_exception()
finally:
    color_stream.stop_streaming()
test.finish()
#
#############################################################################################
#
test.start( "Metadata without a stream name is ignored" )
try:
    pass
except:
    test.unexpected_exception()
test.finish()
#
#############################################################################################
#
test.print_results_and_exit()
