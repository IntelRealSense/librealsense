# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test
import d435i

dds.debug( True, log.nested )


participant = dds.participant()
participant.init( 123, "device-broadcaster" )

# Start a broadcaster to communicate to our client (librealsense)
broadcaster = dds.device_broadcaster( participant )
broadcaster.run()
# These are the servers currently broadcast
servers = {}


def broadcast_device( camera, device_info ):
    """
    E.g.:
        instance = broadcast_device( d435i, d435i.device_info )
    """
    global servers
    instance = device_info.serial
    if not instance:
        raise RuntimeError( "serial-number must be filled out" )
    servers[instance] = {
        'info' : device_info,
        'server' : camera.build( participant )
        }
    broadcaster.add_device( device_info )
    return instance


def close_server( instance ):
    """
    Close the instance returned by broadcast_device()
    """
    global servers
    entry = servers[instance]  # throws if does not exist
    broadcaster.remove_device( entry['info'] )
    del servers[instance]


# From here down, we're in "interactive" mode (see test-device-init.py)
# ...
