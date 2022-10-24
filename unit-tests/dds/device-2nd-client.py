# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "device-2nd-client" )


info = dds.device_info()
info.name = "2nd Device"
info.topic_root = "realdds/device/topic-root"


def test_second_device():
    global device
    device = dds.device( participant, participant.create_guid(), info )
    device.run()  # If no device is available in 30 seconds, this will throw

def close_device():
    global device
    device = None


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
