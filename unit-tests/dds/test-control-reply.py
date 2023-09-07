# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries 2

from rspy import log, test
import pyrealdds as dds
dds.debug( log.is_debug_on() )

with test.remote.fork( nested_indent=None ) as remote:
    if remote is None:  # we're the fork

        with test.closure( 'Start the server participant' ):
            participant = dds.participant()
            participant.init( 123, 'server' )

        with test.closure( 'Create the server' ):
            device_info = dds.message.device_info()
            device_info.name = 'Some device'
            device_info.topic_root = 'control-reply/device'
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.color_stream_server( 's1', 'sensor' )
            s1.init_profiles( s1profiles, 0 )
            server = dds.device_server( participant, device_info.topic_root )
            server.init( [s1], [], {} )

        with test.closure( 'Set up a control handler' ):
            n_replies = 0
            def _on_control( server, id, control, reply ):
                # the control has already been output to debug by the calling code, as will the reply
                global n_replies
                n_replies += 1
                reply['sequence'] = n_replies            # to show that we've processed it
                reply['nested-json'] = { 'more': True }  # to show off
                return True  # otherwise the control will be flagged as error
            server.on_control( _on_control )

        with test.closure( 'Broadcast the device', on_fail=test.ABORT ):
            server.broadcast( device_info )

        raise StopIteration()  # quit the remote


    ###############################################################################################################
    # The client is a device from which we send controls
    #
    from dds import wait_for_devices

    with test.closure( 'Start the client participant' ):
        participant = dds.participant()
        participant.init( 123, 'client' )

    with test.closure( 'Wait for the device' ):
        info = dds.message.device_info()
        info.name = 'Server Device'
        info.topic_root = 'control-reply/device'
        device = dds.device( participant, participant.create_guid(), info )
        device.wait_until_ready()  # If unavailable before timeout, this throws

    with test.closure( 'Set up a notification handler' ):
        n_replies = 0
        def _on_notification( device, id, notification ):
            return True  # otherwise the notification will be flagged as unhandled
        device.on_notification( _on_notification )

    wait_for_reply = True
    sequence = 0
    def send( json ):
        global sequence
        sequence += 1
        reply = device.send_control( json, wait_for_reply )
        test.check_equal( reply['id'], json['id'] )
        test.check_equal( reply['sequence'], sequence )
        return reply

    with test.closure( 'Send some controls' ):
        send( { 'id': 'control' } )
        send( { 'id': 'control-2' } )

    device = None

test.print_results()
