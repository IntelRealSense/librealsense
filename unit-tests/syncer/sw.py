# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test


# Constants
#
domain = rs.timestamp_domain.hardware_clock       # For either depth/color
#
# To be set before init()
#
fps_c = fps_d = 60
w = 640
h = 480
bpp = 2  # bytes


def init():
    """
    """
    global gap_c, gap_d
    gap_d = 1000 / fps_d
    gap_c = 1000 / fps_c
    #
    global pixels, w, h
    pixels = bytearray( b'\x00' * ( w * h * 2 ))  # Dummy data
    #
    global device
    device = rs.software_device()
    device.create_matcher( rs.matchers.default )  # Compare all streams according to timestamp
    #
    global depth_sensor, color_sensor
    depth_sensor = device.add_sensor( "Depth" )
    color_sensor = device.add_sensor( "Color" )
    #
    depth_stream = rs.video_stream()
    depth_stream.type = rs.stream.depth
    depth_stream.uid = 0
    depth_stream.width = 640
    depth_stream.height = 480
    depth_stream.bpp = 2
    depth_stream.fmt = rs.format.z16
    depth_stream.fps = fps_d
    #
    global depth_profile
    depth_profile = rs.video_stream_profile( depth_sensor.add_video_stream( depth_stream ))
    #
    color_stream = rs.video_stream()
    color_stream.type = rs.stream.color
    color_stream.uid = 1
    color_stream.width = 640
    color_stream.height = 480
    color_stream.bpp = 2
    color_stream.fmt = rs.format.rgba8
    color_stream.fps = fps_c
    #
    global color_profile
    color_profile = rs.video_stream_profile( color_sensor.add_video_stream( color_stream ))
    #
    global syncer
    syncer = rs.syncer( 100 )  # We don't want to lose any frames so uses a big queue size (default is 1)


def start():
    """
    """
    global depth_profile, color_profile, depth_sensor, color_sensor, syncer
    depth_sensor.open( depth_profile )
    color_sensor.open( color_profile )
    depth_sensor.start( syncer )
    color_sensor.start( syncer )


def generate_depth_frame( frame_number, timestamp ):
    """
    """
    global depth_profile, domain, pixels, depth_sensor, w, bpp
    #
    depth_frame = rs.software_video_frame()
    depth_frame.pixels = pixels
    depth_frame.stride = w * bpp
    depth_frame.bpp = bpp
    depth_frame.frame_number = frame_number
    depth_frame.timestamp = timestamp
    depth_frame.domain = domain
    depth_frame.profile = depth_profile
    #
    log.d( "-->", depth_frame )
    depth_sensor.on_video_frame( depth_frame )

def generate_color_frame( frame_number, timestamp ):
    """
    """
    global color_profile, domain, pixels, color_sensor, w, bpp
    #
    color_frame = rs.software_video_frame()
    color_frame.pixels = pixels
    color_frame.stride = w * bpp
    color_frame.bpp = bpp
    color_frame.frame_number = frame_number
    color_frame.timestamp = timestamp
    color_frame.domain = domain
    color_frame.profile = color_profile
    #
    log.d( "-->", color_frame )
    color_sensor.on_video_frame( color_frame )

def generate_depth_and_color( frame_number, timestamp ):
    generate_depth_frame( frame_number, timestamp )
    generate_color_frame( frame_number, timestamp )

def expect( depth_frame = None, color_frame = None, nothing_else = False ):
    """
    Looks at the syncer queue and gets the next frame from it if available, checking its contents
    against the expected frame numbers.
    """
    global syncer
    fs = syncer.poll_for_frames()  # NOTE: will never be None
    if not fs:
        test.check( depth_frame is None, "expected a depth frame" )
        test.check( color_frame is None, "expected a color frame" )
        return False

    log.d( "Got", fs )

    depth = fs.get_depth_frame()
    test.info( "actual depth", depth )
    test.check_equal( depth_frame is None, not depth )
    if depth_frame is not None and depth:
        test.check_equal( depth.get_frame_number(), depth_frame )
    
    color = fs.get_color_frame()
    test.info( "actual color", color )
    test.check_equal( color_frame is None, not color )
    if color_frame is not None and color:
        test.check_equal( color.get_frame_number(), color_frame )

    if nothing_else:
        fs = syncer.poll_for_frames()
        test.info( "Expected nothing else; actual", fs )
        test.check( not fs )

    return True

def expect_nothing():
    expect( nothing_else = True )

