# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test


dds.debug( log.is_debug_on(), log.nested )


participant = dds.participant()
participant.init( 123, "server" )
test.check( participant.is_valid() )

publisher = dds.publisher( participant )

broadcasters = []


def broadcast( props ):
    global broadcasters, publisher
    serial = props.setdefault( 'serial', str(len(broadcasters)) )
    props.setdefault( 'name', f'device{serial}' )
    props.setdefault( 'topic-root', f'device{serial}' )
    di = dds.message.device_info.from_json( props )
    broadcasters.append( dds.device_broadcaster( publisher, di ) )


def unbroadcast_all():
    global broadcasters
    broadcasters = []


# From here down, we're in "interactive" mode (see test-watcher.py)
# ...
