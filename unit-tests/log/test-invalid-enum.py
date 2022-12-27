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
    # Following does not work in Python 3.6:
    #     TypeError: __init__(): incompatible constructor arguments. The following argument types are supported:
    #         1. pyrealsense2.pyrealsense2.log_severity(value: int)
    #     Invoked with: -1
    # I.e., instead of getting a RuntimeError from the C++, we get a TypeError from Python...
    #test.check_throws( lambda: rs.log( rs.log_severity( -1 ), "-1" ), RuntimeError, 'invalid enum value for argument "severity"' )
    test.check_equal( common.n_messages, 1 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
