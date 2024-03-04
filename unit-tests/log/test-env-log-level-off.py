# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common
import os


#############################################################################################
#
with test.closure( "Without LRS_LOG_LEVEL" ):
    test.check( not os.environ.get('LRS_LOG_LEVEL'), on_fail=test.ABORT )

    rs.log_to_callback( rs.log_severity.error, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    common.log_all()
    test.check_equal( common.n_messages, 1 )
#
#############################################################################################
test.print_results_and_exit()
