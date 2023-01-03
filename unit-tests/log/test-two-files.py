# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common
import tempfile, os.path


#############################################################################################
#
test.start( f"Double file logging" )

try:
    filename1 = tempfile.mktemp()
    filename2 = tempfile.mktemp()
    log.d( f'Filename1 logging to: {filename1}' )
    log.d( f'Filename2 logging to: {filename2}' )
    rs.log_to_file( rs.log_severity.error, filename1 )
    rs.log_to_file( rs.log_severity.error, filename2 )

    # Following should log to only the latter!
    common.log_all()

    rs.reset_logger()  # Should flush!
    #el::Loggers::flushAll();   // requires static!

    test.check_equal( common.count_lines( filename1 ), 0 )
    test.check_equal( common.count_lines( filename2 ), 1 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
