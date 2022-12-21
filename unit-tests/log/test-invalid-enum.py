# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common


#############################################################################################
#
test.start( f"Logging with invalid enum" )

try:
    rs.log_to_callback( rs.log_severity.debug, common.message_counter )
    test.check_equal( common.n_messages, 0 )
    # Following will throw a recoverable_exception, which will issue a log by itself!
    test.check_throws( lambda: rs.log( rs.log_severity( 10 ), "10" ), RuntimeError, 'invalid enum value for argument "severity"' )
    test.check_throws( lambda: rs.log( rs.log_severity( -1 ), "-1" ), RuntimeError, 'invalid enum value for argument "severity"' )
    test.check_equal( common.n_messages, 2 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
