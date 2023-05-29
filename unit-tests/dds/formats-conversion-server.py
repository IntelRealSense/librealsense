# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test
import pyrealsense2 as rs

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "formats-conversion-server" )

device_info = dds.device_info()
device_info.name = "formats-coversion-device"
device_info.serial = "123"
device_info.product_line = "D400"
device_info.topic_root = "root_" + device_info.serial


def test_one_video_profile_per_stream( format_strings ):
    profiles = []
    stream_servers = []
    counter = 0
    for format_string in format_strings:
        profile = dds.video_stream_profile( 30, dds.stream_format( format_string ), 1280, 720 )
        profiles.append( profile )
        if format_string == "16UC1":
            stream_server = dds.depth_stream_server( "server" + str( counter ), "sensor" )
        elif format_string == "yuv422_yuy2" or format_string == "UYVY":
            stream_server = dds.color_stream_server( "server" + str( counter ), "sensor" )
        else:
            stream_server = dds.ir_stream_server( "server" + str( counter ), "sensor" )
        stream_server.init_profiles( profiles, 0 )
        log.d( stream_server.profiles() )
        stream_servers.append( stream_server )
        counter = counter + 1
    
    global dev_server
    dev_server = dds.device_server( participant, device_info.topic_root )
    dev_server.init( stream_servers, [], {} )
    dev_server.broadcast( device_info )


def test_multiple_video_profiles_per_stream( format_strings, frequencies, stream_type = "depth" ):
    profiles = []
    stream_servers = []
    counter = 0
    for format_string in format_strings:
        profile = dds.video_stream_profile( frequencies[counter], dds.stream_format( format_string ), 1280, 720 )
        profiles.append( profile )
        counter = counter + 1

    if stream_type == "depth":
        stream_server = dds.depth_stream_server( "server", "sensor" )
    elif stream_type == "color":
        stream_server = dds.color_stream_server( "server", "sensor" )
    else:
        stream_server = dds.ir_stream_server( "server", "sensor" )

    stream_server.init_profiles( profiles, 0 )
    log.d( stream_server.profiles() )
    stream_servers.append( stream_server )

    global dev_server
    dev_server = dds.device_server( participant, device_info.topic_root )
    dev_server.init( stream_servers, [], {} )
    dev_server.broadcast( device_info )


def test_one_motion_profile_per_stream( format_strings, stream_type = "accel" ):
    profiles = []
    stream_servers = []
    counter = 0
    for format_string in format_strings:
        profile = dds.motion_stream_profile( 30, dds.stream_format( format_string ) )
        profiles.append( profile )
        if stream_type == "accel":
            stream_server = dds.accel_stream_server( "server" + str( counter ), "sensor" )
        else:
            stream_server = dds.gyro_stream_server( "server" + str( counter ), "sensor" )
        stream_server.init_profiles( profiles, 0 )
        log.d( stream_server.profiles() )
        stream_servers.append( stream_server )
        counter = counter + 1

    global dev_server
    dev_server = dds.device_server( participant, device_info.topic_root )
    dev_server.init( stream_servers, [], {} )
    dev_server.broadcast( device_info )


def test_multiple_motion_profiles_per_stream( format_strings, frequencies, stream_type = "accel" ):
    profiles = []
    stream_servers = []
    counter = 0
    for format_string in format_strings:
        profile = dds.motion_stream_profile( frequencies[counter], dds.stream_format( format_string ) )
        profiles.append( profile )
        counter = counter + 1

    if stream_type == "accel":
        stream_server = dds.accel_stream_server( "server", "sensor" )
    else:
        stream_server = dds.gyro_stream_server( "server", "sensor" )
    stream_server.init_profiles( profiles, 0 )
    log.d( stream_server.profiles() )
    stream_servers.append( stream_server )

    global dev_server
    dev_server = dds.device_server( participant, device_info.topic_root )
    dev_server.init( stream_servers, [], {} )
    dev_server.broadcast( device_info )

    
def test_Y8I_stream():
    test_one_video_profile_per_stream( { "Y8I" } )


def test_Y12I_stream():
    test_one_video_profile_per_stream( { "Y12I" } )
 
 
def test_Y8_stream():
    test_one_video_profile_per_stream( { "mono8" } )


def test_YUYV_stream():
    test_one_video_profile_per_stream( { "yuv422_yuy2" } )


def test_UYVY_stream():
    test_one_video_profile_per_stream( { "UYVY" } )
    
    
def test_Z16_stream():
    test_one_video_profile_per_stream( { "16UC1" } )
    
    
def test_accel_stream():
    test_one_motion_profile_per_stream( { "MXYZ" }, "accel" )


def test_gyro_stream():
    test_one_motion_profile_per_stream( { "MXYZ" }, "gyro" )


def test_multiple_accel_profiles():
    test_multiple_motion_profiles_per_stream( [ "MXYZ", "MXYZ", "MXYZ", "MXYZ" ], [ 63, 200, 250, 400 ], "accel" )


def test_multiple_color_profiles():
    test_multiple_video_profiles_per_stream( [ "yuv422_yuy2", "yuv422_yuy2", "yuv422_yuy2" ], [ 5, 15, 30 ], "color" )


def test_multiple_depth_profiles():
    test_multiple_video_profiles_per_stream( [ "16UC1", "16UC1", "16UC1", "16UC1", "16UC1" ], [ 5, 10, 15, 20, 30 ], "depth" )     

    
def close_server():
    global dev_server
    dev_server = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
