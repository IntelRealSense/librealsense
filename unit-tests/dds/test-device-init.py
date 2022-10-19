# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test


dds.debug( True, 'cli' )
log.nested = 'cli'


participant = dds.participant()
participant.init( 123, "test-device-init" )

info = dds.device_info()
info.name = "Test Device"
info.topic_root = "realdds/device/topic-root"


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
dir( test )
with test.remote( os.path.join( cwd, 'device-init-server.py' )) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Test 1..." )
    try:
        remote.run( 'test_one()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run()  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.num_of_sensors(), 1 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################


participant = None
test.print_results_and_exit()
