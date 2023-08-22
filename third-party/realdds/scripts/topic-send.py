# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from argparse import ArgumentParser
args = ArgumentParser()
args.add_argument( '--debug', action='store_true', help='enable debug mode' )
args.add_argument( '--quiet', action='store_true', help='No output; just the minimum FPS as a number' )
args.add_argument( '--device', metavar='<path>', help='the topic root for the device' )
args.add_argument( '--topic', metavar='<path>', help='the topic on which to send flexible message, if --device is not supplied' )
args.add_argument( '--message', metavar='<string>', help='a message to send', default='some message' )
def domain_arg(x):
    t = int(x)
    if t <= 0 or t > 232:
        raise ValueError( f'--domain should be [0,232]' )
    return t
args.add_argument( '--domain', metavar='<0-232>', type=domain_arg, default=0, help='DDS domain to use (default=0)' )
args = args.parse_args()


if args.quiet:
    def i( *a, **kw ):
        pass
else:
    def i( *a, **kw ):
        print( '-I-', *a, **kw )
def e( *a, **kw ):
    print( '-E-', *a, **kw )


import pyrealdds as dds
import time
import sys

dds.debug( args.debug )

settings = {}

participant = dds.participant()
participant.init( args.domain, 'fps', settings )

message = { 'id': 'ping', 'message': args.message }

if args.device:
    info = dds.message.device_info()
    info.name = 'Dummy Device'
    info.topic_root = args.device
    device = dds.device( participant, participant.create_guid(), info )
    try:
        i( 'Looking for device at', info.topic_root, '...' )
        device.wait_until_ready()  # If unavailable before timeout, this throws
    except:
        e( 'Cannot find device' )
        sys.exit( 1 )

    wait_for_reply = True
    reply = device.send_control( message, wait_for_reply )
    i( f'Sent {message} on {info.topic_root}; got back {reply}' )

    if args.debug or not wait_for_reply:
        # Sleep a bit, to allow us to catch and display any replies
        time.sleep( 2 )

elif not args.topic:
    e( 'Either --device or --topic is required' )
    sys.exit( 1 )

else:
    topic_path = args.topic
    writer = dds.topic_writer( dds.message.flexible.create_topic( participant, topic_path ))
    writer.run( dds.topic_writer.qos() )
    # Let the client pick up on the new entity - if we send it too quickly, they won't see it before we disappear...
    time.sleep( 1 )
    dds.message.flexible( message ).write_to( writer )
    i( f'Sent {message} on {topic_path}' )


