# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "options-server" )


def test_no_options():
    # Create one stream with one profile so device init won't fail
    # No device options, no stream options
    s1p1 = dds.video_stream_profile( 9, dds.stream_format("RGB8"), 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    dev_opts = []
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1], dev_opts )

def test_device_options_discovery():
    # Create one stream with one profile so device init won't fail, no stream options
    s1p1 = dds.video_stream_profile( 9, dds.stream_format("RGB8"), 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    do1 = dds.dds_option( "opt1", "device" )
    do1.set_value( 1 )
    do2 = dds.dds_option( "opt2", "device" )
    do2.set_value( 2 )
    do3 = dds.dds_option( "opt3", "device" )
    do3.set_value( 3 )
    dev_opts = [do1, do2, do3]
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1], dev_opts )

def test_stream_options_discovery():
    s1p1 = dds.video_stream_profile( 9, dds.stream_format("RGB8"), 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    so1 = dds.dds_option( "opt1", "s1" )
    so1.set_value( 1 )
    so2 = dds.dds_option( "opt2", "s1" )
    range = dds.dds_option_range()
    range.min = 0 # Using int values because can't test.check_equal floats
    range.max = 123456
    range.step = 123
    range.default_value = 12
    so2.set_range( range )
    so3 = dds.dds_option( "opt3", "s1" )
    so3.set_description( "opt3 of s1")
    s1.init_options( [so1, so2, so3] )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1], [] )


def close_server():
    global server
    server = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
