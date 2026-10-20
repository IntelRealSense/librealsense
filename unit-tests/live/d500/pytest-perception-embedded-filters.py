# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

# The perception stream and the embedded filters can run at the same time.
#
# The decimation and temporal embedded filters (DPP) run on the camera CPU, so they no longer
# compete with object detection for the DSP. The SDK must not reject either order: enabling a
# filter and then starting perception, or toggling a filter while perception is opened or streaming.
#
# The test adapts to the device at runtime: it skips when the perception stream or a
# given embedded filter is not present, rather than gating on firmware versions.

import pytest
import pyrealsense2 as rs
import logging
log = logging.getLogger(__name__)


pytestmark = [
    pytest.mark.device_each( "D555" ),
    pytest.mark.device_each( "D585" ),
    pytest.mark.context( "nightly" ),
]


EMBEDDED_FILTERS = [
    rs.embedded_filter_type.decimation,
    rs.embedded_filter_type.temporal,
    rs.embedded_filter_type.improved_close_range_depth,
]

ENABLED = rs.option.embedded_filter_enabled


@pytest.fixture
def depth_sensor(test_device):
    dev, _ = test_device
    return dev.first_depth_sensor()


@pytest.fixture
def perception_sensor(test_device):
    dev, _ = test_device
    try:
        return dev.first_perception_sensor()  # throws if the device has no perception sensor
    except RuntimeError:
        pytest.skip( "Device has no perception sensor" )


@pytest.fixture
def perception_profile(perception_sensor):
    profile = next( ( p for p in perception_sensor.get_stream_profiles()
                      if p.stream_type() == rs.stream.object_detection ), None )
    if profile is None:
        pytest.skip( "Perception sensor has no object-detection profile" )
    return profile


# Over USB the decimation/temporal DPP filters are toggled through the 'enabled' field of their composite
# option; over DDS (and for the other filters) through the embedded_filter_enabled option.
DPP_COMPOSITE = {
    rs.embedded_filter_type.decimation: ( rs.composite_option_id.decimation_filter_dpp,
                                          'get_decimation_filter_dpp_config', 'set_decimation_filter_dpp_config' ),
    rs.embedded_filter_type.temporal: ( rs.composite_option_id.temporal_filter_dpp,
                                        'get_temporal_filter_dpp_config', 'set_temporal_filter_dpp_config' ),
}


class FilterToggle:
    def __init__( self, ef, filter_type ):
        self._ef = ef
        self._composite = None
        if not ef.supports( ENABLED ) and filter_type in DPP_COMPOSITE:
            option_id, getter, setter = DPP_COMPOSITE[filter_type]
            if option_id in ef.get_supported_composite_options():
                self._composite = ( option_id, getattr( ef, getter ), getattr( ef, setter ) )
        if not self._composite and not ef.supports( ENABLED ):
            pytest.skip( f"Embedded filter {filter_type} has no enable control on this device" )

    def get( self ):
        if self._composite:
            option_id, getter, _ = self._composite
            return float( getter( option_id ).enabled )
        return self._ef.get_option( ENABLED )

    def set( self, value ):
        if self._composite:
            option_id, getter, setter = self._composite
            cfg = getter( option_id )
            cfg.enabled = int( value )
            setter( option_id, cfg )
        else:
            self._ef.set_option( ENABLED, value )


@pytest.fixture
def embedded_filter(depth_sensor, filter_type):
    # get_embedded_filter throws (rather than returning falsy) when the filter is not present.
    try:
        ef = depth_sensor.get_embedded_filter( filter_type )
    except Exception:
        pytest.skip( f"Embedded filter {filter_type} not present on this device" )
    toggle = FilterToggle( ef, filter_type )
    initial = toggle.get()
    yield toggle
    toggle.set( initial )


def start_perception(perception_sensor, perception_profile):
    perception_sensor.open( perception_profile )
    try:
        perception_sensor.start( lambda f: None )
    except Exception:
        perception_sensor.close()  # start never took effect; undo the open before propagating
        raise


def stop_perception(perception_sensor):
    perception_sensor.stop()
    perception_sensor.close()


@pytest.mark.parametrize( "filter_type", EMBEDDED_FILTERS, ids=lambda t: t.name )
def test_perception_starts_with_filter_enabled(perception_sensor, perception_profile, embedded_filter):
    embedded_filter.set( 1.0 )
    start_perception( perception_sensor, perception_profile )
    try:
        assert embedded_filter.get() == 1.0
    finally:
        stop_perception( perception_sensor )


@pytest.mark.parametrize( "filter_type", EMBEDDED_FILTERS, ids=lambda t: t.name )
def test_filter_toggles_while_perception_streams(perception_sensor, perception_profile, embedded_filter):
    embedded_filter.set( 0.0 )
    start_perception( perception_sensor, perception_profile )
    try:
        embedded_filter.set( 1.0 )
        assert embedded_filter.get() == 1.0
        embedded_filter.set( 0.0 )
        assert embedded_filter.get() == 0.0
    finally:
        stop_perception( perception_sensor )


@pytest.mark.parametrize( "filter_type", EMBEDDED_FILTERS, ids=lambda t: t.name )
def test_filter_enables_while_perception_opened_not_streaming(perception_sensor, perception_profile, embedded_filter):
    # Covers the open()->start() window: the perception sensor is opened but not yet streaming
    embedded_filter.set( 0.0 )
    perception_sensor.open( perception_profile )
    try:
        embedded_filter.set( 1.0 )
        assert embedded_filter.get() == 1.0
        perception_sensor.start( lambda f: None )
        perception_sensor.stop()
    finally:
        perception_sensor.close()
