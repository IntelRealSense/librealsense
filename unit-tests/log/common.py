# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
import pyrealsense2 as rs
import os


def log_all():
    rs.log( rs.log_severity.debug, "debug message" )
    rs.log( rs.log_severity.info, "info message" )
    rs.log( rs.log_severity.warn, "warn message" )
    rs.log( rs.log_severity.error, "error message" )
    # NOTE: fatal messages will exit the process and so cannot be tested
    #rs.log( rs.log_severity.fatal, "fatal message" )
    rs.log( rs.log_severity.none, "no message" )  # ignored; no callback


n_messages = 0
def message_counter( severity, message ):
    global n_messages
    n_messages += 1
    log.d( message.full() )
    #
    test.check_equal( str(message), message.raw() )
    test.check_equal( repr(message), message.full() )


n_messages_2 = 0
def message_counter_2( severity, message ):
    global n_messages_2
    n_messages_2 += 1
    log.d( message.full() )


def count_lines( filename ):
    # -1 because the text always has an extra \n
    return len( open( filename, 'rt' ).read().split( '\n' )) - 1
