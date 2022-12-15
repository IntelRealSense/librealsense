# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds

import pyrealdds as dds
from rspy import log, test
import d435i


dds.debug( True, 'C  ' )
log.nested = 'C  '


participant = dds.participant()
participant.init( 123, "test-device-init" )

info = dds.device_info()
info.name = "Test Device"
info.topic_root = "realdds/device/topic-root"


import os.path
cwd = os.path.dirname(os.path.realpath(__file__))
remote_script = os.path.join( cwd, 'device-init-server.py' )
second_client_script = os.path.join( cwd, 'device-2nd-client.py' )
with test.remote( remote_script, nested_indent="  S" ) as remote:
    remote.wait_until_ready()
    #
    #############################################################################################
    #
    test.start( "Test 1 stream..." )
    try:
        remote.run( 'test_one_stream()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run( 1000 )  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 1 )
        for stream in device.streams():
            profiles = stream.profiles()
            test.check_equal( stream.name(), "s1" )
            test.check_equal( stream.sensor_name(), "sensor" )
            test.check_equal( len( profiles ), 1 )
            test.check_equal( str(profiles[0]), '<pyrealdds.depth_stream_profile 10x10 RGB8 @ 9 Hz>' )
            test.check_equal( profiles[0].stream(), stream )
            test.check_equal( stream.default_profile_index(), 0 )
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test motion stream..." )
    try:
        remote.run( 'test_one_motion_stream()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run( 1000 )  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 1 )
        for stream in device.streams():
            profiles = stream.profiles()
            test.check_equal( stream.name(), "s2" )
            test.check_equal( stream.sensor_name(), "sensor2" )
            test.check_equal( stream.type_string(), "accel" )
            test.check_equal( len( profiles ), 1 )
            test.check_equal( str(profiles[0]), '<pyrealdds.accel_stream_profile RGB8 @ 30 Hz>' )
            test.check_equal( profiles[0].stream(), stream )
            test.check_equal( stream.default_profile_index(), 0 )
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test no streams..." )
    try:
        remote.run( 'test_no_streams()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run( 1000 )  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 0 )
        for stream in device.streams():
            test.unreachable()
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test no profiles... (should fail on server side)" )
    try:
        remote.run( 'test_no_profiles()', timeout=5 )
    except test.remote.Error as e:
        # this fails because streams require at least one profile
        test.check_exception( e, test.remote.Error, "RuntimeError: at least one profile is required to initialize stream 's1'" )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test 10 profiles..." )
    try:
        remote.run( 'test_n_profiles(10)', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run( 1000 )  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 1 )
        for stream in device.streams():
            profiles = stream.profiles() 
        test.check_equal( 10, len( profiles ))
        # Go thru the profiles, make sure they're in the right order (json is unordered)
        fibo = [0,1]
        for i in range(10):
            v = fibo[-2] + fibo[-1]
            fibo[-2] = fibo[-1]
            fibo[-1] = v
            test.check_equal( profiles[i].width(), i )
            test.check_equal( profiles[i].height(), v )
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test D435i..." )
    try:
        remote.run( 'test_d435i()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), d435i.device_info )
        device.run( 1000 )  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 6 )
        for stream in device.streams():
            profiles = stream.profiles() 
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################
    #
    test.start( "Test discovery of another client device..." )
    try:
        remote.run( 'test_one_stream()', timeout=5 )
        device = dds.device( participant, participant.create_guid(), info )
        device.run( 1000 )  # If no device is available in 30 seconds, this will throw
        test.check( device.is_running() )
        test.check_equal( device.n_streams(), 1 )
        # now start another server, and have it access the device
        # NOTE: I had trouble nesting another 'with' statement here...
        with test.remote( second_client_script, name="remote2", nested_indent="  c" ) as remote2:
            remote2.wait_until_ready()
            remote2.run( 'test_second_device()', timeout=5 )
            # the notifications for the second client should be sent again from the server -- we
            # should ignore them...
            remote2.run( 'close_device()', timeout=5 )
        remote.run( 'close_server()', timeout=5 )
    except:
        test.unexpected_exception()
    device = None
    test.finish()
    #
    #############################################################################################


participant = None
test.print_results_and_exit()
