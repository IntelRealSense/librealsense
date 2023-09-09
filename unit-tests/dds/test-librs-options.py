# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries 2

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
            options = []
            option = dds.option( 'Backlight Compensation', 'RGB Camera' )
            option.set_range( dds.option_range( 0, 1, 1, 0 ) )
            option.set_description( 'Backlight custom description' )
            options.append( option )
            option = dds.option( 'Custom Option', 'RGB Camera' )
            option.set_range( dds.option_range( 0, 1, 1, 0 ) )
            option.set_description( 'Something' )
            options.append( option )
            s1.init_options( options )
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
        context = rs.context( { 'dds': { 'domain': 123, 'participant': 'client' }} )
        only_sw_devices = int(rs.product_line.sw_only) | int(rs.product_line.any)

    with test.closure( 'Find the server', on_fail=test.ABORT ):
        dev = wait_for_devices( context, only_sw_devices, n=1. )
        for s in dev.query_sensors():
            break
        options = test.info( "supported options", s.get_supported_options() )
        test.check_equal( len(options), 3 )  # 'Frames Queue Size' gets added to all sensors!!?!?!

    dev = None
    context = None

test.print_results()
