# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common


#############################################################################################
#
test.start( f"reset-logger" )

try:
    rs.log_to_callback( rs.log_severity.info, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    rs.reset_logger()
    common.log_all()
    test.check_equal( common.n_messages, 0 )

    rs.log_to_callback( rs.log_severity.info, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    common.log_all()
    test.check_equal( common.n_messages, 3 )

    rs.reset_logger()
    common.log_all()
    test.check_equal( common.n_messages, 3 )

    common.n_messages = 0
    rs.log_to_callback( rs.log_severity.debug, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    common.log_all()
    test.check_equal( common.n_messages, 4 )

    common.n_messages = 0
    rs.reset_logger()
    common.log_all()
    test.check_equal( common.n_messages, 0 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
