# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries:gha 2

from rspy import log, test
with test.remote.fork( nested_indent=None ) as remote:
    if remote is None:  # we're the fork
        import pyrealdds as dds
        dds.debug( log.is_debug_on() )

        with test.closure( 'Start the server participant' ):
            participant = dds.participant()
            participant.init( 123, 'server' )

        with test.closure( 'Create the server' ):
            device_info = dds.message.device_info()
            device_info.name = 'Options device'
            device_info.topic_root = 'librs-options/device'
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.color_stream_server( 's1', 'sensor' )
            s1.init_profiles( s1profiles, 0 )
            s1.init_options( [
                dds.option.from_json( ['Backlight Compensation', 0, 0, 1, 1, 0, 'Backlight custom description'] ),
                dds.option.from_json( ['Boolean Option', False, False, 'Something'] ),
                dds.option.from_json( ['Integer Option', 1, None, 'Something', ['optional']] ),
                dds.option.from_json( ['Enum Option', 'First', ['First','Last','Everything'], 'Last', 'My'] )
                ] )
            server = dds.device_server( participant, device_info.topic_root )
            server.init( [s1], [], {} )

        with test.closure( 'Broadcast the device', on_fail=test.ABORT ):
            server.broadcast( device_info )

        raise StopIteration()  # quit the remote


    ###############################################################################################################
    # The client is LibRS
    #
    import pyrealsense2 as rs
    if log.is_debug_on():
        rs.log_to_console( rs.log_severity.debug )
    from dds import wait_for_devices

    with test.closure( 'Initialize librealsense context', on_fail=test.ABORT ):
        context = rs.context( { 'dds': { 'enabled': True, 'domain': 123, 'participant': 'client' }} )
        only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any)

    with test.closure( 'Find the server', on_fail=test.ABORT ):
        dev = wait_for_devices( context, only_sw_devices, n=1. )
        for s in dev.query_sensors():
            break
        options = test.info( "supported options", s.get_supported_options() )
        test.check_equal( len(options), 5 )  # 'Frames Queue Size' gets added to all sensors!!?!?!

    with test.closure( 'Play with integer option' ):
        io = next( o for o in options if str(o) == 'Integer Option' )
        iv = s.get_option_value( io )
        test.check_equal( iv.type, rs.option_type.integer )
        test.check_equal( iv.value, 1 )
        test.check_equal( s.get_option( io ), 1. )
        s.set_option( io, 5 )
        test.check_equal( s.get_option( io ), 5. )

    with test.closure( 'Play with boolean option' ):
        bo = next( o for o in options if str(o) == 'Boolean Option' )
        bv = s.get_option_value( bo )
        test.check_equal( bv.type, rs.option_type.boolean )
        test.check_equal( bv.value, False )
        test.check_equal( s.get_option( bo ), 0. )
        s.set_option( bo, 1. )
        test.check_equal( s.get_option( bo ), 1. )
        test.check_throws( lambda: s.set_option( bo, 2. ), RuntimeError, 'not a boolean: 2' )
        test.check_throws( lambda: s.set_option( bo, 1.01 ), RuntimeError, 'not a boolean: 1.01' )

    with test.closure( 'Play with enum option' ):
        eo = next( o for o in options if str(o) == 'Enum Option' )
        ev = s.get_option_value( eo )
        test.check_equal( ev.type, rs.option_type.string )
        test.check_equal( ev.value, 'First' )
        er = s.get_option_range( ev.id )
        test.check_equal( er.min, 0. )
        test.check_equal( er.max, 2. )
        test.check_equal( er.default, 1. )
        test.check_equal( er.step, 1. )
        test.check_equal( s.get_option_value_description( eo, 0. ), 'First' )
        test.check_equal( s.get_option_value_description( eo, 1. ), 'Last' )
        test.check_equal( s.get_option_value_description( eo, 2. ), 'Everything' )
        test.check_equal( s.get_option( eo ), 0. )
        s.set_option( eo, 2. )
        test.check_equal( s.get_option_value( eo ).value, 'Everything' )
        s.set_option_value( eo, 'Last' )
        test.check_equal( s.get_option( eo ), 1. )

    with test.closure( 'All done' ):
        dev = None
        context = None

test.print_results()
