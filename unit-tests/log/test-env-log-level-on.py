# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
test.set_env_vars( { 'LRS_LOG_LEVEL' : 'WARN' } )

import pyrealsense2 as rs
import common
import os


#############################################################################################
#
test.start( "With LRS_LOG_LEVEL" )

try:
    test.check_equal( os.environ.get('LRS_LOG_LEVEL'), 'WARN', abort_if_failed=True )

    rs.log_to_callback( rs.log_severity.error, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    common.log_all()
    # Without LRS_LOG_LEVEL, the result would be 1 (error)
    #test.check_equal( common.n_messages, 1 )
    # But, since we have the log-level forced to warning:
    test.check_equal( common.n_messages, 2 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
