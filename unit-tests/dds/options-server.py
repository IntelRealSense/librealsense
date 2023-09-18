# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( log.is_debug_on(), log.nested )


participant = dds.participant()
participant.init( 123, "options-server" )


def test_no_options():
    # Create one stream with one profile so device init won't fail
    # No device options, no stream options
    s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    dev_opts = []
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1], dev_opts, {} )

def test_device_options_discovery( values ):
    # Create one stream with one profile so device init won't fail, no stream options
    s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    dev_opts = []
    for index, value in enumerate( values ):
        option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
        dev_opts.append( option )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1], dev_opts, {})

def test_stream_options_discovery( value, min, max, step, default, description ):
    s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    so1 = dds.option( "opt1", dds.option_range( value, value, 0, value ), "opt1 is const" )
    so1.set_value( value )
    so2 = dds.option( "opt2", dds.option_range( min, max, step, default ), "opt2 with range" )
    so3 = dds.option( "opt3", dds.option_range( 0, 1, 0.05, 0.15 ), description )
    s1.init_options( [so1, so2, so3] )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1], [], {} )

def test_device_and_multiple_stream_options_discovery( dev_values, stream_values ):
    dev_options = []
    for index, value in enumerate( dev_values ):
        option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
        dev_options.append( option )

    s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.depth_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    stream_options = []
    for index, value in enumerate( stream_values ):
        option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
        stream_options.append( option )
    s1.init_options( stream_options )

    s2p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
    s2profiles = [s2p1]
    s2 = dds.depth_stream_server( "s2", "sensor" )
    s2.init_profiles( s2profiles, 0 )
    stream_options = []
    for index, value in enumerate( stream_values ):
        option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
        stream_options.append( option )
    s2.init_options( stream_options )

    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1, s2], dev_options, {} )

def close_server():
    global server
    server = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
