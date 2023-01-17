# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import common


import tempfile, os.path
temp_dir = tempfile.gettempdir()


#############################################################################################
#
test.start( f"Rolling logger (max 1M)" )

try:
    log_filename = os.path.join( temp_dir, 'rolling.log' )
    rs.log_to_file( rs.log_severity.info, log_filename )

    max_size = 1  # MB
    rs.enable_rolling_log_file( max_size )

    for i in range( 15000 ):
        rs.log( rs.log_severity.info, f'debug message {i}' )
    rs.reset_logger()

    with open( log_filename, "rb" ) as log_file:
        log_file.seek( 0, 2 )  # 0 bytes from end
        log_size = log_file.tell()
    del log_file
    log.d( f'{log_filename} size: {log_size}' )

    old_filename = log_filename + ".old"
    with open( old_filename, "rb" ) as old_file:
        old_file.seek( 0, 2 )  # 0 bytes from end
        old_size = old_file.tell()
    del old_file
    log.d( f'{old_filename} size: {log_size}' )

    max_size_in_bytes = max_size * 1024 * 1024
    size = log_size + old_size
    test.check( size <= 2 * max_size_in_bytes )
except:
    test.unexpected_exception()

test.finish()
#
#############################################################################################
test.print_results_and_exit()
