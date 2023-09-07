# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test


dds.debug( log.is_debug_on(), 'C  ' )
log.nested = 'C  '


participant = dds.participant()
participant.init( 123, "test-options" )

info = dds.message.device_info()
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
    with test.closure( "Test no options" ):
        remote.run( 'test_no_options()' )
        device = dds.device( participant, info )
        device.wait_until_ready()

        options = device.options();
        for option in options:
            test.unreachable() # Test no device option

        test.check_equal( device.n_streams(), 1 )
        for stream in device.streams():
            options = stream.options();
            for option in options:
                test.unreachable() # Test no stream option

        remote.run( 'close_server()' )
    device = None
    #
    #############################################################################################
    #
    with test.closure( "Test device options discovery" ):
        test_values = list(range(17))
        remote.run( 'test_device_options_discovery(' + str( test_values ) + ')' )
        device = dds.device( participant, info )
        device.wait_until_ready()

        options = device.options();
        test.check_equal( len( options ), len(test_values) )
        for index, option in enumerate( options ):
            test.check_equal( option.get_value(), float( test_values[index] ) )

        option.set_value( -1. )  # only on client!
        test.check_equal( device.query_option_value( option ), float( test_values[index] ) )
        test.check_equal( option.get_value(), float( test_values[index] ) )  # from server

        device.set_option_value( option, -2. )  # TODO this is not valid for the option range!
        test.check_equal( option.get_value(), -2. )

        remote.run( 'close_server()' )
    device = None
    #
    #############################################################################################
    #
    with test.closure( "Test stream options discovery" ):
        #send values to be checked later as string parameter to the function
        remote.run( 'test_stream_options_discovery(1, 0, 123456, 123, 12, "opt3 of s1")' )
        device = dds.device( participant, info )
        device.wait_until_ready()
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

        option = options[1]
        test.check_equal( option.get_value(), 0. )  # not default!?
        option.set_value( 1. )  # only on client!
        test.check_equal( device.query_option_value( option ), 0. )
        test.check_equal( option.get_value(), 0. )  # from server

        device.set_option_value( option, 12. )
        test.check_equal( option.get_value(), 12. )

        remote.run( 'close_server()' )
    device = None
    #
    #############################################################################################
    #
    with test.closure( "Test device and multiple stream options discovery" ):
        test_values = list(range(5))
        remote.run( 'test_device_and_multiple_stream_options_discovery(' + str( test_values ) + ', ' + str( test_values ) + ')' )
        device = dds.device( participant, info )
        device.wait_until_ready()

        options = device.options();
        test.check_equal( len( options ), len(test_values) )
        for index, option in enumerate( options ):
            test.check_equal( option.get_value(), float( test_values[index] ) )

        for stream in device.streams():
            options = stream.options();
            test.check_equal( len( options ), len(test_values) )
            for index, option in enumerate( options ):
                test.check_equal( option.get_value(), float( test_values[index] ) )

        remote.run( 'close_server()' )
    device = None
    #
    #############################################################################################


participant = None
test.print_results_and_exit()
