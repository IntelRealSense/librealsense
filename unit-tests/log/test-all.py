# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common


#############################################################################################
#
test.start( f"log all" )

try:
    # debug severity include all log messages
    rs.log_to_callback( rs.log_severity.debug, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    common.log_all()
    test.check_equal( common.n_messages, 4 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
