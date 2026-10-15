# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

# FW error polling over GMSL: inject a FW error with a HWM command and expect the
# matching notification from the polling handler, reported once (read-to-clear).

import logging
import threading
import time

import pytest
import pyrealsense2 as rs
import pyrsutils as rsutils
from rspy.pytest.device_helpers import require_min_fw_version

log = logging.getLogger(__name__)

pytestmark = [
    pytest.mark.device_each( "D400*" ),
    pytest.mark.device_type( "GMSL" ),  # USB polling is covered by pytest-notifications-callback-gil
]

OPCODE_TRIGGER_ERROR    = 0x4D  # HWM opcode that injects a firmware error
ERROR_ID_REC_ERROR      = 25    # one-time error class: latched once, never re-raised
EXPECTED_DESCRIPTION    = "REC error"
NOTIFICATION_TIMEOUT_S  = 5     # the handler polls every 1 s
SETTLE_AFTER_FIRST_S    = 3     # an uncleared error would be re-reported on each poll


def test_fw_error_polling( test_device ):
    dev, _ = test_device
    require_min_fw_version( dev, rsutils.version( 5, 17, 6, 0 ), "GMSL FW error polling" )
    if dev.get_info( rs.camera_info.camera_locked ) == "YES":
        pytest.skip( "FW rejects error injection on locked cameras" )

    depth_sensor = dev.first_depth_sensor()
    assert depth_sensor.supports( rs.option.error_polling_enabled ), \
        "error polling not supported - is the d4xx driver older than 1.0.7.6?"

    notifications = []
    received = threading.Event()

    def on_notification( n ):
        log.debug( "notification: %s (%s)", n.get_description(), n.get_category() )
        notifications.append( n.get_description() )
        received.set()

    # Registering a callback turns error polling on
    depth_sensor.set_notifications_callback( on_notification )
    assert depth_sensor.get_option( rs.option.error_polling_enabled ) == 1

    proto = rs.debug_protocol( dev )
    reply = list( proto.send_and_receive_raw_data( proto.build_command( OPCODE_TRIGGER_ERROR, ERROR_ID_REC_ERROR ) ) )
    assert reply[:4] == [OPCODE_TRIGGER_ERROR, 0, 0, 0], f"HWM error injection rejected: {reply[:8]}"

    assert received.wait( NOTIFICATION_TIMEOUT_S ), \
        f"no FW error notification within {NOTIFICATION_TIMEOUT_S} [sec] of injecting error {ERROR_ID_REC_ERROR}"
    time.sleep( SETTLE_AFTER_FIRST_S )

    log.info( "notifications: %s", notifications )
    assert notifications == [EXPECTED_DESCRIPTION]
