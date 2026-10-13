# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

# Object Detection (PD) runs on its own: no Depth or Color capture is needed, over USB and over GMSL.
# Each session must deliver valid detection results and stop cleanly, so the next one starts again.
#
# The test adapts to the device: it skips when there is no perception sensor (e.g. GMSL without the Perception MUX).

import pytest
import pyrealsense2 as rs
import struct
import time
import logging
log = logging.getLogger(__name__)


pytestmark = [
    pytest.mark.device_each( "D585" ),
    pytest.mark.device_exclude( "D585S" ),   # the safety SKU has no perception sensor
]

DURATION_S = 4
MIN_PD_FRAMES = 10   # GMSL runs PD at ~15 fps
ODET_HEADER = 20     # magic, version, type, flags, payload size, spare, crc
ODET_PAYLOAD_MIN = 23   # payload header of a result with no detections


@pytest.fixture
def pd(test_device):
    dev, _ = test_device
    try:
        sensor = dev.first_perception_sensor()   # throws if the device has no perception sensor
    except RuntimeError:
        pytest.skip( "Device has no perception sensor" )
    profile = next( ( p for p in sensor.get_stream_profiles() if p.stream_type() == rs.stream.object_detection ), None )
    if profile is None:
        pytest.skip( "Perception sensor has no object-detection profile" )
    yield sensor, profile
    for close in ( sensor.stop, sensor.close ):   # never hand the next test an open sensor
        try:
            close()
        except RuntimeError:
            pass                                 # not started/open, the normal path


class Collector:
    """PD results with what the checks need, taken at arrival."""
    def __init__(self):
        self.frames = []
    def __call__(self, f):
        head = bytes( f.get_data() )[:ODET_HEADER]
        self.frames.append( { 'time': time.monotonic(), 'size': f.get_data_size(), 'magic': head[:4],
                              'payload': struct.unpack_from( '<I', head, 8 )[0], 'number': f.get_frame_number() } )
    def since(self, t):
        return [i for i in self.frames if i['time'] >= t]


def check_results(frames):
    assert len( frames ) >= MIN_PD_FRAMES
    # USB sends the exact result length, GMSL pads it to the profile size; the header declares the valid part
    assert all( i['magic'] == b'ODET' and ODET_PAYLOAD_MIN <= i['payload'] <= i['size'] - ODET_HEADER for i in frames )
    assert all( b['number'] > a['number'] for a, b in zip( frames, frames[1:] ) )


def test_pd_standalone_open_close_twice(pd):
    sensor, profile = pd
    for _ in range( 2 ):   # the second session proves the first one stopped cleanly
        frames = Collector()
        sensor.open( profile )
        sensor.start( frames )
        time.sleep( DURATION_S )
        sensor.stop()
        sensor.close()
        check_results( frames.frames )
        stopped = len( frames.frames )
        time.sleep( 0.5 )
        assert len( frames.frames ) == stopped, "no results after stop"


def test_pd_standalone_stop_start_resumes(pd):
    sensor, profile = pd
    frames = Collector()
    sensor.open( profile )
    sensor.start( frames )
    time.sleep( DURATION_S / 2 )
    sensor.stop()
    restart = time.monotonic()
    sensor.start( frames )
    time.sleep( DURATION_S )
    sensor.stop()
    sensor.close()
    check_results( frames.since( restart ) )
