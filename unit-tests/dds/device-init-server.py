# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "device-init-server" )


def test_one_stream():
    s1p1 = dds.video_stream_profile( dds.stream_format("RGB8"), 9, 10, 10 )
    s1profiles = [s1p1]
    s1 = dds.video_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    log.d( s1.profiles() )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1] )

def test_one_motion_stream():
    s1p1 = dds.motion_stream_profile( dds.stream_format("RGB8"), 30 )
    s1profiles = [s1p1]
    s1 = dds.motion_stream_server( "s2", "sensor2" )
    s1.init_profiles( s1profiles, 0 )
    log.d( s1.profiles() )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1] )

def test_no_profiles():
    s1profiles = []
    s1 = dds.video_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1] )

def test_no_streams():
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [] )

def test_n_profiles( n_profiles ):
    assert n_profiles > 0
    s1profiles = []
    fibo = [0,1]
    for i in range(n_profiles):
        width = i
        height = fibo[-2] + fibo[-1]
        fibo[-2] = fibo[-1]
        fibo[-1] = height
        s1profiles += [dds.video_stream_profile( dds.stream_format("RGB8"), 9, width, height )]
    s1 = dds.video_stream_server( "s1", "sensor" )
    s1.init_profiles( s1profiles, 0 )
    log.d( s1.profiles() )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1] )


def close_server():
    global server
    server = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
