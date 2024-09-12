# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from argparse import ArgumentParser
from argparse import ArgumentTypeError as ArgumentError  # NOTE: only ArgumentTypeError passes along the original error string
args = ArgumentParser()
args.add_argument( '--debug', action='store_true', help='enable debug mode' )
args.add_argument( '--quiet', action='store_true', help='no output' )
args.add_argument( '--device', metavar='<path>', help='the topic root for the device' )
args.add_argument( '--topic', metavar='<path>', help='the topic on which to send a message/blob, if --device is not supplied' )
import json
def json_arg(x):
    try:
        return json.loads(x)
    except Exception as e:
        raise ArgumentError( str(e) )
args.add_argument( '--message', metavar='<json>', type=json_arg, help='a message to send', default='{"id":"ping","message":"some message"}' )
args.add_argument( '--blob', metavar='<filename>', help='a file to send' )
args.add_argument( '--ack', action='store_true', help='wait for acks' )
def domain_arg(x):
    t = int(x)
    if t <= 0 or t > 232:
        raise ArgumentError( f'--domain should be [0-232]' )
    return t
args.add_argument( '--domain', metavar='<0-232>', type=domain_arg, default=-1, help='DDS domain to use (default=0)' )
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
participant.init( dds.load_rs_settings( settings ), args.domain )

message = args.message

if args.blob:
    if not args.topic:
        e( '--blob requires --topic' )
        sys.exit( 1 )
    topic_path = args.topic
    if not os.path.isfile( args.blob ):
        e( '--blob <file> does not exist:', args.blob )
        sys.exit( 1 )
    writer = dds.topic_writer( dds.message.blob.create_topic( participant, topic_path ))
    writer.run( dds.topic_writer.qos() )  # reliable
    # Let the client pick up on the new entity - if we send it too quickly, they won't see it before we disappear...
    time.sleep( 1 )
    with open( args.blob, mode='rb' ) as file: # b is important -> binary
        blob = dds.message.blob( file.read() )
    if not writer.has_readers():
        e( 'No readers exist on topic:', topic_path )
        sys.exit( 1 )
    i( f'Writing {blob} on {topic_path} ...' )
    start = dds.now()
    blob.write_to( writer )
    if args.ack:
        if not writer.wait_for_acks( dds.time( 5. ) ):  # seconds
            e( 'Timeout waiting for ack' )
            sys.exit( 1 )
        i( f'Acknowledged ({dds.timestr( dds.now(), start )})' )
    else:
        i( f'Done' )

elif args.device:
    info = dds.message.device_info()
    info.name = 'Dummy Device'
    info.topic_root = args.device
    device = dds.device( participant, info )
    try:
        i( 'Looking for device at', info.topic_root, '...' )
        device.wait_until_ready()  # If unavailable before timeout, this throws
    except:
        e( 'Cannot find device' )
        sys.exit( 1 )

    wait_for_reply = True
    i( f'Sending {message} on {info.topic_root}' )
    reply = device.send_control( message, wait_for_reply )
    i( f'Got back {reply}' )

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
    if not writer.has_readers():
        e( 'No readers exist on topic:', topic_path )
        sys.exit( 1 )
    start = dds.now()
    dds.message.flexible( message ).write_to( writer )
    i( f'Sent {message} on {topic_path}' )
    if args.ack:
        if not writer.wait_for_acks( dds.time( 5. ) ):  # seconds
            e( 'Timeout waiting for ack' )
            sys.exit( 1 )
        i( f'Acknowledged ({dds.timestr( dds.now(), start )})' )


