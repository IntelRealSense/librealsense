# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "device-init-server" )


def test_one_stream():
    log.d( 'test-one-stream running...' )
    s1p1 = dds.video_stream_profile( dds.stream_uid(23,45), dds.stream_format("RGB8"), 9, 10, 10, 123 )
    s1profiles = [s1p1]
    s1 = dds.video_stream_server( "s1", s1profiles )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1] )

def test_no_profiles():
    log.d( 'test-no-profiles running...' )
    s1profiles = []
    s1 = dds.video_stream_server( "s1", s1profiles )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [s1] )

def test_no_streams():
    log.d( 'test-no-streams running...' )
    global server
    server = dds.device_server( participant, "realdds/device/topic-root" )
    server.init( [] )


def close_server():
    global server
    server = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
