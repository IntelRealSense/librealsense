# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common


#############################################################################################
#
test.start( f"Logging to two callbacks" )

try:
    rs.log_to_callback( rs.log_severity.error, common.message_counter )
    rs.log_to_callback( rs.log_severity.error, common.message_counter_2 )
    test.check_equal( common.n_messages, 0 )
    test.check_equal( common.n_messages_2, 0 )
    common.log_all()
    test.check_equal( common.n_messages, 1 )
    test.check_equal( common.n_messages_2, 1 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
