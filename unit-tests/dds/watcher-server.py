# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as server
from rspy import log, test


server.debug( True, log.nested )


#############################################################################################
#
test.start( "broadcast two devices" )

participant = server.participant()
participant.init( 123, "test-watcher-server" )
test.check( participant.is_valid() )

publisher = server.publisher( participant )

di = server.device_info()
di.name = 'my device'
di.serial = '12345'
di.product_line = 'PL'
di.topic_root = 'testing/my/device/sn'
broadcaster1 = server.device_broadcaster( publisher, di )

from time import sleep
sleep(1)  # give the client some time with it

di = server.device_info()
di.name = 'another device'
di.serial = '67890'
di.product_line = 'PL'
di.topic_root = 'testing/my/device2/sn'
broadcaster2 = server.device_broadcaster( publisher, di )

from time import sleep
sleep(1)  # give the client some time with it

broadcaster1 = broadcaster2 = None
participant = None
test.finish()
#
#############################################################################################
test.print_results_and_exit()
