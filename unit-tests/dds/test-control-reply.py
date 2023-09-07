# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#xxtest:retries 2

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

        raise StopIteration()  # exit the 'with' statement


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
        device = dds.device( participant, info )
        device.wait_until_ready()

    with test.closure( 'Set up a notification handler' ):
        n_replies = 0
        notification_count = dict()
        reply_count = dict()
        notification_count[device.guid()] = 0
        reply_count[device.guid()] = 0
        import threading
        have_reply = threading.Event()
        def _on_notification( device, id, notification ):
            global notification_count, have_reply
            have_reply.set()
            notification_count[device.guid()] += 1
            sample = notification.get( 'sample' )
            if sample is None:
                log.d( f'notification to {device}: {notification}' )
            elif sample[0] == str(device.guid()):
                log.d( f'reply to {device}' )
                reply_count[device.guid()] += 1
            else:
                log.d( f'notification to {device}' )
            return True  # otherwise the notification will be flagged as unhandled
        device.on_notification( _on_notification )

    with test.closure( 'Send a notification that is not a reply' ):
        dev1_notifications = notification_count[device.guid()]
        dev1_replies = reply_count[device.guid()]
        have_reply.clear()
        remote.run( 'server.publish_notification( { "id": "something" } )' )
        have_reply.wait( 3 )
        test.check_equal( notification_count[device.guid()], dev1_notifications + 1 )  # notification
        test.check_equal( reply_count[device.guid()], dev1_replies )                   # not a reply

    wait_for_reply = True
    server_sequence = 0
    def control( device, json ):
        global server_sequence
        server_sequence += 1
        reply = device.send_control( json, wait_for_reply )
        test.check_equal( reply['id'], json['id'] )
        if test.check( reply.get('control') is not None ):
            test.check_equal( reply['control']['id'], json['id'] )
        test.check_equal( reply['sequence'], server_sequence )
        test.check_equal( reply['sample'][0], str(device.guid()) )
        return reply

    with test.closure( 'Send some controls' ):
        control( device, { 'id': 'control' } )
        control( device, { 'id': 'control-2' } )

    with test.closure( 'Add a second device!' ):
        device2 = dds.device( participant, info )
        notification_count[device2.guid()] = 0
        reply_count[device2.guid()] = 0
        device2.on_notification( _on_notification )
        device2.wait_until_ready()

    with test.closure( 'Controls generate notifications to all devices' ):
        dev1_notifications = notification_count[device.guid()]
        dev2_notifications = notification_count[device2.guid()]
        control( device, { 'id': 'dev1' } )
        test.check_equal( notification_count[device.guid()], dev1_notifications + 1 )
        test.check_equal( notification_count[device2.guid()], dev2_notifications + 1 )  # both get notifications
        control( device2, { 'id': 'dev2' } )
        test.check_equal( notification_count[device.guid()], dev1_notifications + 2 )
        test.check_equal( notification_count[device2.guid()], dev2_notifications + 2 )

    with test.closure( 'But only one gets a reply' ):
        dev1_replies = reply_count[device.guid()]
        dev2_replies = reply_count[device2.guid()]
        control( device, { 'id': 'dev1' } )
        test.check_equal( reply_count[device.guid()], dev1_replies + 1 )
        test.check_equal( reply_count[device2.guid()], dev2_replies )
        control( device2, { 'id': 'dev2' } )
        test.check_equal( reply_count[device.guid()], dev1_replies + 1 )
        test.check_equal( reply_count[device2.guid()], dev2_replies + 1 )

    device = None

test.print_results()
