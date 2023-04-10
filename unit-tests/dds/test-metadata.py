# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

# We disable under Linux for now, pending feedback from FastDDS team:
# Having two participants in the same process ("client" and "librs" below) usually works, but in this case the former
# is from pyrealdds and the latter from pyrealsense2. The two somehow interfere so the server doesn't even see the
# latter and we have a problem where the broadcaster does not work.
#test:donotrun:linux


import pyrealdds as dds
from rspy import log, test
from time import sleep
from rspy.stopwatch import Stopwatch

dds.debug( True, 'C  ' )
log.nested = 'C  '

import d435i


participant = dds.participant()
participant.init( 123, "client" )


# run the server in another process: GHA has some issue with it in the same process...
import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'metadata-server.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()


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
    with test.closure( "No librs syncer; direct from server" ):
        md = { 'stream-name' : 'Color', 'invalid-metadata' : True }
        with metadata_expected( md ):
            remote.run( f'device_server.publish_metadata( {md} )' )
    #
    #############################################################################################
    #
    with test.closure( "Broadcast the device" ):  # otherwise librs won't see it
        remote.run( 'broadcast()' )
    #
    #############################################################################################
    #
    with test.closure( "Initialize librs device", abort_if_failed=True ):
        import pyrealsense2 as rs
        rs.log_to_console( rs.log_severity.debug )
        from dds import wait_for_devices
        context = rs.context( '{"dds-domain":123,"dds-participant-name":"librs"}' )
        only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any_intel)
        device = wait_for_devices( context, only_sw_devices, n=1. )[0]
        sensors = device.sensors
        test.check_equal( len(sensors), 1, abort_if_failed=True )
        sensor = sensors[0]
        test.check_equal( sensor.get_info( rs.camera_info.name ), 'RGB Camera', abort_if_failed=True )
        del sensors
        profile = rs.video_stream_profile( sensor.get_stream_profiles()[0] )  # take the first one
        log.d( f'using profile {profile}')
        encoding = dds.stream_format.from_rs2( profile.format() )
        remote.run( f'img = new_image( {profile.width()}, {profile.height()}, {profile.bytes_per_pixel()} )', on_fail='abort' )
        sensor.open( [profile] )
        queue = rs.frame_queue( 100 )
        sensor.start( queue )
    #
    #############################################################################################
    #
    with test.closure( "Metadata alone should not come out" ):
        with metadata_expected( count=20 ):
            for i in range(20):
                md = { 'stream-name' : 'Color', 'header' : { 'frame-id' : str(i) }, 'metadata' : {} }
                remote.run( f'device_server.publish_metadata( {md} )' )
        sleep( 0.25 )  # plus some extra for librs...
        test.check_false( queue.poll_for_frame() )  # we didn't send any images, shouldn't get any frames!
    #
    #############################################################################################
    #
    with test.closure( "Metadata after an image" ):
        i += 1
        timestamp = dds.now()
        remote.run( f'img.timestamp = dds.time.from_ns( {timestamp.to_ns()} )' )
        remote.run( f'color_stream.start_streaming( dds.stream_format( "{encoding}" ), img.width, img.height )' )
        # It will take the image a lot longer to get anywhere than the metadata
        with image_expected():
            remote.run( f'publish_image( img, {i} )' )
        sleep( 0.25 )  # plus some extra for librs...
        test.check_false( queue.poll_for_frame() )  # the image should still be pending in the syncer
        with metadata_expected():
            md = {
                'stream-name' : 'Color',
                'header' : {'frame-id':str(i), 'timestamp':timestamp.to_double(), 'timestamp-domain':int(rs.timestamp_domain.system_time) },
                'metadata': {
                    "White Balance" : 0xbaad
                    }
            }
            remote.run( f'device_server.publish_metadata( {md} )' )
        f = queue.wait_for_frame( 250 )  # A frame should now be available
        log.d( '---->', f )
        if test.check( f ) and test.check_equal( f.get_frame_number(), 20 ):
            test.check_equal( f.get_timestamp(), timestamp.to_double() )
            test.check_false( f.supports_frame_metadata( rs.frame_metadata_value.actual_fps ) )
            if test.check( f.supports_frame_metadata( rs.frame_metadata_value.white_balance ) ):
                test.check_equal( f.get_frame_metadata( rs.frame_metadata_value.white_balance ), 0xbaad )
        test.check_false( queue.poll_for_frame() )  # the image should still be pending in the syncer
    remote.run( 'color_stream.stop_streaming()', on_fail='catch' )
    #
    #############################################################################################
    #
    with test.closure( "Metadata without a stream name is ignored" ):
        pass
    #
    #############################################################################################
    #
test.print_results_and_exit()
