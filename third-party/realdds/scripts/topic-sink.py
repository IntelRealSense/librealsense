# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

from argparse import ArgumentParser
from argparse import ArgumentTypeError as ArgumentError  # NOTE: only ArgumentTypeError passes along the original error string
args = ArgumentParser()
args.add_argument( '--debug', action='store_true', help='enable debug mode' )
args.add_argument( '--quiet', action='store_true', help='no output' )
args.add_argument( '--topic', metavar='<path>', help='the topic on which to listen' )
args.add_argument( '--blob', action='store_true', help='when set, listen for blobs' )
args.add_argument( '--image', action='store_true', help='when set, listen for images' )
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

import sys

if not args.topic:
    e( '--topic is required' )
    sys.exit( 1 )
topic_path = args.topic


import pyrealdds as dds
import time

dds.debug( args.debug )

settings = {}

participant = dds.participant()
participant.init( dds.load_rs_settings( settings ), args.domain )

if args.blob:
    reader = dds.topic_reader( dds.message.blob.create_topic( participant, topic_path ))
    def on_data_available( reader ):
        got_something = False
        while True:
            sample = dds.message.sample_info()
            msg = dds.message.blob.take_next( reader, sample )
            if not msg:
                if not got_something:
                    raise RuntimeError( "expected message not received!" )
                break
            i( f'-----> {msg}', )
            got_something = True
    reader.on_data_available( on_data_available )
    reader.run( dds.topic_reader.qos() )

elif args.image:
    reader = dds.topic_reader( dds.message.image.create_topic( participant, topic_path ))
    def on_data_available( reader ):
        got_something = False
        while True:
            sample = dds.message.sample_info()
            msg = dds.message.image.take_next( reader, sample )
            if not msg:
                if not got_something:
                    raise RuntimeError( "expected message not received!" )
                break
            i( f'-----> {msg}', )
            got_something = True
    reader.on_data_available( on_data_available )
    reader.run( dds.topic_reader.qos( dds.reliability.best_effort, dds.durability.volatile ) )

else:
    reader = dds.topic_reader( dds.message.flexible.create_topic( participant, topic_path ))
    import json
    def on_data_available( reader ):
        got_something = False
        while True:
            sample = dds.message.sample_info()
            msg = dds.message.flexible.take_next( reader, sample )
            if not msg:
                if not got_something:
                    raise RuntimeError( "expected message not received!" )
                break
            i( f'-----> {json.dumps( msg.json_data(), indent=4 )}', )
            got_something = True
    reader.on_data_available( on_data_available )
    reader.run( dds.topic_reader.qos() )

# Keep waiting until the user breaks...
import signal
def handler(signum, frame):
    #res = input("Ctrl-c was pressed. Do you really want to exit? y/n ")
    #if res == 'y':
    sys.exit( 0 )
signal.signal( signal.SIGINT, handler )
i( 'Press ctrl+C to break...' )
while True:
    time.sleep(1)

