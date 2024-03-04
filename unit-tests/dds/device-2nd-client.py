# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( log.is_debug_on(), log.nested )


participant = dds.participant()
participant.init( 123, f'client-{log.nested.strip()}' )


info = dds.message.device_info()
info.name = "2nd Device"
info.topic_root = "realdds/device/topic-root"


def test_second_device():
    global device
    device = dds.device( participant, info )
    device.wait_until_ready()  # If no device is available before timeout, this will throw

def close_device():
    global device
    device = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
