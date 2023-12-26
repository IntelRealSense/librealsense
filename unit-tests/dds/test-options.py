# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries:gha 2

import pyrealdds as dds
from rspy import log, test

info = dds.message.device_info()
info.name = "Test Device"
info.topic_root = "realsense/options-device"

with test.remote.fork( nested_indent=None ) as remote:
    if remote is None:  # we're the fork

        dds.debug( log.is_debug_on(), log.nested )

        participant = dds.participant()
        participant.init( 123, "server" )


        def test_no_options():
            # Create one stream with one profile so device init won't fail
            # No device options, no stream options
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.depth_stream_server( "s1", "sensor" )
            s1.init_profiles( s1profiles, 0 )
            dev_opts = []
            global server
            server = dds.device_server( participant, info.topic_root )
            server.init( [s1], dev_opts, {} )

        def test_device_options_discovery( values ):
            # Create one stream with one profile so device init won't fail, no stream options
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.depth_stream_server( "s1", "sensor" )
            s1.init_profiles( s1profiles, 0 )
            dev_opts = []
            for index, value in enumerate( values ):
                option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
                dev_opts.append( option )
            global server
            server = dds.device_server( participant, info.topic_root )
            server.init( [s1], dev_opts, {})

        def test_stream_options_discovery( value, min, max, step, default, description ):
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.depth_stream_server( "s1", "sensor" )
            s1.init_profiles( s1profiles, 0 )
            so1 = dds.option( "opt1", dds.option_range( value, value, 0, value ), "opt1 is const" )
            so1.set_value( value )
            so2 = dds.option( "opt2", dds.option_range( min, max, step, default ), "opt2 with range" )
            so3 = dds.option( "opt3", dds.option_range( 0, 1, 0.05, 0.15 ), description )
            s1.init_options( [so1, so2, so3] )
            global server
            server = dds.device_server( participant, info.topic_root )
            server.init( [s1], [], {} )

        def test_device_and_multiple_stream_options_discovery( dev_values, stream_values ):
            dev_options = []
            for index, value in enumerate( dev_values ):
                option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
                dev_options.append( option )

            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.depth_stream_server( "s1", "sensor" )
            s1.init_profiles( s1profiles, 0 )
            stream_options = []
            for index, value in enumerate( stream_values ):
                option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
                stream_options.append( option )
            s1.init_options( stream_options )

            s2p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s2profiles = [s2p1]
            s2 = dds.depth_stream_server( "s2", "sensor" )
            s2.init_profiles( s2profiles, 0 )
            stream_options = []
            for index, value in enumerate( stream_values ):
                option = dds.option( f'opt{index}', dds.option_range( value, value, 0, value ), f'opt{index} description' )
                stream_options.append( option )
            s2.init_options( stream_options )

            global server
            server = dds.device_server( participant, info.topic_root )
            server.init( [s1, s2], dev_options, {} )

        def close_server():
            global server
            server = None

        raise StopIteration()  # exit the 'with' statement


    ###############################################################################################################
    #
    dds.debug( log.is_debug_on(), 'C  ' )
    log.nested = 'C  '

    participant = dds.participant()
    participant.init( 123, "test-options" )


    #############################################################################################
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


    #############################################################################################
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


    #############################################################################################
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
        test.check_equal( option.get_value(), option.get_range().default_value )
        option.set_value( 1. )  # only on client!
        test.check_equal( option.get_value(), 1. )
        test.check_equal( device.query_option_value( option ), option.get_range().default_value )  # from server
        test.check_equal( option.get_value(), option.get_range().default_value )  # client got updated!

        device.set_option_value( option, 12. )  # updates server & client
        test.check_equal( option.get_value(), 12. )

        remote.run( 'close_server()' )
    device = None


    #############################################################################################
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
    participant = None


#############################################################################################
test.print_results()
