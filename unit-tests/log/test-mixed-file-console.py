# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common
import tempfile, os.path


#############################################################################################
#
test.start( f"Mixed file & console logging" )

try:
    filename = tempfile.mktemp()
    log.d( f'Filename logging to: {filename}' )
    rs.log_to_file( rs.log_severity.error, filename )
    rs.log_to_console( rs.log_severity.error )

    common.log_all()

    rs.reset_logger()  # Should flush!
    #el::Loggers::flushAll();   // requires static!

    test.check_equal( common.count_lines( filename ), 1 )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
