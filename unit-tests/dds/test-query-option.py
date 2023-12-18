# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries:gha 2

from rspy import log, test
import pyrealdds as dds
dds.debug( log.is_debug_on() )

device_info = dds.message.device_info()
device_info.topic_root = 'server/device'

with test.remote.fork( nested_indent=None ) as remote:
    if remote is None:  # we're the fork

        with test.closure( 'Start the server participant' ):
            participant = dds.participant()
            participant.init( 123, 'server' )

        with test.closure( 'Create the server' ):
            device_info.name = 'Some device'
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.color_stream_server( 's1', 'sensor' )
            s1.init_profiles( s1profiles, 0 )
            s1.init_options( [
                dds.option( 'Backlight Compensation', dds.option_range( 0, 1, 1, 0 ), 'Backlight custom description' ),
                dds.option( 'Custom Option', dds.option_range( 0, 1, 0.1, 0.5 ), 'Something' ),
                dds.option( 'Option 3', dds.option_range( 0, 50, 1, 25 ), 'Something Else' )
                ] )
            server = dds.device_server( participant, device_info.topic_root )
            server.init( [s1], [], {} )

        raise StopIteration()  # exit the 'with' statement


    ###############################################################################################################
    # The client is a device from which we send controls
    #
    from dds import wait_for_devices

    with test.closure( 'Start the client participant' ):
        participant = dds.participant()
        participant.init( 123, 'client' )

    with test.closure( 'Wait for the device' ):
        device_info.name = 'Device1'
        device = dds.device( participant, device_info )
        device.wait_until_ready()

    with test.closure( 'Query single option, option-name: "Option 3"' ):
        reply = device.send_control( {
                'id': 'query-option',
                'stream-name': 's1',
                'option-name': 'Option 3'
            }, True )  # Wait for reply
        test.info( 'reply', reply )
        test.check_equal( reply.get( 'value' ), 25. )

    with test.closure( 'Option names are case-sensitive' ):
        test.check_throws( lambda:
            device.send_control( {
                'id': 'query-option',
                'stream-name': 's1',
                'option-name': 'custom option'
            }, True ),  # Wait for reply
            RuntimeError,
            'control #1 "query-option" failed: [error] \'s1\' option \'custom option\' not found' )

    with test.closure( 'Query all options, option-name:[]' ):
        reply = device.send_control( {
                'id': 'query-option',
                'stream-name': 's1',
                'option-name': []  # get all options
            }, True )  # Wait for reply
        test.info( 'reply', reply )
        values = reply.get( 'option-values' )
        if test.check( values ):
            test.check_equal( len(values), 3 )
            test.check_equal( type(values), dict )

    with test.closure( 'Query multiple options, option-name:["Option 3","Custom Option"]' ):
        reply = device.send_control( {
                'id': 'query-option',
                'stream-name': 's1',
                'option-name': ['Option 3', 'Custom Option']
            }, True )  # Wait for reply
        test.info( 'reply', reply )
        values = reply.get( 'value' )
        if test.check( values ):
            test.check_equal( len(values), 2 )
            test.check_equal( type(values), list )

    device = None

test.print_results()
