# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

# HDR Mode selects the processing pipeline on D5x5 USB SKUs, over the HWM CUSTOM_CMD.
# The firmware applies it at the next stream start and refuses changes while the sensor is open.

import pytest
import pyrealsense2 as rs
import time
import logging
log = logging.getLogger(__name__)

pytestmark = [
    pytest.mark.device_each("D585"),
    pytest.mark.device_exclude("D585S"),
    pytest.mark.skip(reason="No D585 with HDR mode firmware in CI"),
]

HDR = rs.option.hdr_mode
MODES = rs.hdr_mode


@pytest.fixture
def hdr_sensor(test_device):
    """The sensor holding the HDR mode (color on 3C, depth on 2C), restored to the boot default at the end."""
    dev, _ = test_device
    sensor = next((s for s in dev.sensors if s.supports(HDR)), None)
    if not sensor:
        pytest.skip("HDR mode is not supported by this firmware")
    sensor.set_option(HDR, MODES.single_frame_wdr)
    yield sensor
    try:
        sensor.set_option(HDR, MODES.single_frame_wdr)
    except Exception as e:
        log.warning("could not restore HDR mode: %s", e)


def color_profile(sensor, fps=30):
    profile = next((p for p in sensor.profiles
                    if p.stream_type() == rs.stream.color and p.fps() == fps
                    and p.as_video_stream_profile().width() == 1280 and p.as_video_stream_profile().height() == 720), None)
    if not profile:
        pytest.skip(f"no 1280x720 {fps} fps color profile")
    return profile


def test_hdr_mode_range(hdr_sensor):
    r = hdr_sensor.get_option_range(HDR)
    assert r.min == MODES.off
    assert r.max == MODES.multi_frame_hdr
    assert r.step == 1
    assert r.default == MODES.single_frame_wdr


def test_hdr_mode_set_get(hdr_sensor):
    for mode in (MODES.off, MODES.single_frame_wdr):
        hdr_sensor.set_option(HDR, mode)
        assert hdr_sensor.get_option(HDR) == mode


def test_hdr_mode_rejects_out_of_range(hdr_sensor):
    for bad in (int(MODES.multi_frame_hdr) + 1, 256, -1, 0.5):
        with pytest.raises(Exception):
            hdr_sensor.set_option(HDR, bad)
    assert hdr_sensor.get_option(HDR) == MODES.single_frame_wdr


def test_hdr_mode_rejected_while_open(hdr_sensor):
    # An open sensor blocks the set even before it starts; GET keeps working.
    assert not hdr_sensor.is_option_read_only(HDR)
    hdr_sensor.open(color_profile(hdr_sensor))
    try:
        assert hdr_sensor.is_option_read_only(HDR)
        with pytest.raises(Exception):
            hdr_sensor.set_option(HDR, MODES.off)
        hdr_sensor.start(lambda frame: None)
        try:
            assert hdr_sensor.get_option(HDR) == MODES.single_frame_wdr
        finally:
            hdr_sensor.stop()
    finally:
        hdr_sensor.close()
    assert not hdr_sensor.is_option_read_only(HDR)
    hdr_sensor.set_option(HDR, MODES.off)
    assert hdr_sensor.get_option(HDR) == MODES.off


def test_hdr_mode_multi_frame_streams(hdr_sensor):
    try:
        hdr_sensor.set_option(HDR, MODES.multi_frame_hdr)
    except Exception as e:
        pytest.skip(f"Multi-Frame HDR not supported by this firmware build: {e}")
    assert hdr_sensor.get_option(HDR) == MODES.multi_frame_hdr

    # The firmware merges short/long pairs, so the host still receives the requested output fps.
    frames = []
    hdr_sensor.open(color_profile(hdr_sensor, 30))
    hdr_sensor.start(lambda frame: frames.append(frame.get_timestamp()))
    try:
        time.sleep(4)
    finally:
        hdr_sensor.stop()
        hdr_sensor.close()
    assert len(frames) > 2 * 30
    assert hdr_sensor.get_option(HDR) == MODES.multi_frame_hdr
