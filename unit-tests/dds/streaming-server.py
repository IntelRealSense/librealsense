# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test
import flexible

dds.debug( log.is_debug_on(), log.nested )


participant = dds.participant()
participant.init( 123, "streaming-server" )


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
