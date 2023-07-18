# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as server
from rspy import log, test


server.debug( log.is_debug_on(), log.nested )


#############################################################################################
#
test.start( "participant-init" )

participant = server.participant()
test.check( not participant )

participant.init( 123, "test-participant-server" )

test.check( participant )
test.check( participant.is_valid() )

test.finish()
#
#############################################################################################
test.print_results_and_exit()
