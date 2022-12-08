# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test


dds.debug( True, 'C  ' )
log.nested = 'C  '


participant = dds.participant()
participant.init( 123, "test-options" )

info = dds.device_info()
info.name = "Test Device"
info.topic_root = "realdds/device/topic-root"


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'options-server.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Test no options..." )
    try:
        remote.run( 'test_no_options()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run()  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        
        options = device.options();
        for option in options:
            test.unreachable() # Test no device option
        
        test.check_equal( device.n_streams(), 1 )
        for stream in device.streams():
            options = stream.options();
            for option in options:
                test.unreachable() # Test no stream option
                
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test device options discovery..." )
    try:
        remote.run( 'test_device_options_discovery()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run()  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        
        options = device.options();
        test.check_equal( len( options ), 3 )
        test.check_equal( options[0].get_value(), 1. )
        test.check_equal( options[1].get_value(), 2. )
        test.check_equal( options[2].get_value(), 3. )
        
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test stream options discovery..." )
    try:
        remote.run( 'test_stream_options_discovery()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run()  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 1 )
        for stream in device.streams():
            options = stream.options();
            test.check_equal( len( options ), 3 )
            test.check_equal( options[0].get_value(), 1. )
            test.check_equal( options[1].get_range().min, 0. )
            test.check_equal( options[1].get_range().max, 123456. )
            test.check_equal( options[1].get_range().step, 123. )
            test.check_equal( options[1].get_range().default_value, 12. )
            test.check_equal( options[2].get_description(), "opt3 of s1" )
                
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################


participant = None
test.print_results_and_exit()
