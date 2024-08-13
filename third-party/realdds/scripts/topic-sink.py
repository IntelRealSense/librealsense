# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

from argparse import ArgumentParser
from argparse import ArgumentTypeError as ArgumentError  # NOTE: only ArgumentTypeError passes along the original error string
args = ArgumentParser()
args.add_argument( '--debug', action='store_true', help='enable debug mode' )
args.add_argument( '--quiet', action='store_true', help='no output' )
args.add_argument( '--flexible', metavar='<path>', action='append', help='the flexible (reliable) topic on which to listen' )
args.add_argument( '--flexible-be', metavar='<path>', action='append', help='the flexible (best-effort) topic on which to listen' )
args.add_argument( '--blob', metavar='<path>', action='append', help='the blob topic on which to listen' )
args.add_argument( '--image', metavar='<path>', action='append', help='the image topic on which to listen' )
def domain_arg(x):
    t = int(x)
    if t <= 0 or t > 232:
        raise ArgumentError( f'--domain should be [0-232]' )
    return t
args.add_argument( '--domain', metavar='<0-232>', type=domain_arg, default=-1, help='DDS domain to use (default=0)' )
def time_arg(x):
    t = float(x)
    if t < 0.:
        raise ValueError( f'--time should be >=0' )
    return t
args.add_argument( '--wait', metavar='<seconds>', type=time_arg, default=5., help='seconds to wait for writers (default 5; 0=disable)' )
args.add_argument( '--time', metavar='<seconds>', type=time_arg, help='runtime before stopping, in seconds (default 0=forever)' )
args.add_argument( '--not-ready', action='store_true', help='start output immediately, without waiting for all topics' )
args.add_argument( '--field', metavar='<name>', help='name to extract from the flexible JSON (or whole JSON is printed, indented)' )
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

if not args.flexible and not args.flexible_be and not args.blob and not args.image:
    e( '--flexible, --flexible-be, --blob, or --image required' )
    sys.exit( 1 )

import pyrealdds as dds
import time
from datetime import datetime

def timestamp():
   return datetime.now().time()

dds.debug( args.debug )

settings = {}

participant = dds.participant()
participant.init( dds.load_rs_settings( settings ), args.domain )

readers = []
ready = args.not_ready

# BLOB
def on_blob_available( reader ):
    if not ready:
        return
    got_something = False
    while True:
        sample = dds.message.sample_info()
        msg = dds.message.blob.take_next( reader, sample )
        if not msg:
            if not got_something:
                raise RuntimeError( "expected message not received!" )
            break
        i( f'{timestamp()} {msg}', )
        got_something = True
for topic_path in args.blob or []:
    reader = dds.topic_reader( dds.message.blob.create_topic( participant, topic_path ))
    reader.on_data_available( on_blob_available )
    reader.run( dds.topic_reader.qos() )
    i( f'Waiting for {topic_path}' )
    if args.wait and not reader.wait_for_writers( dds.time( args.wait ) ):
        e( f'Timeout waiting for writers on {topic_path}' )
        sys.exit( 1 )
    readers.append( reader )

# IMAGE
def on_image_available( reader ):
    if not ready:
        return
    got_something = False
    while True:
        sample = dds.message.sample_info()
        msg = dds.message.image.take_next( reader, sample )
        if not msg:
            if not got_something:
                raise RuntimeError( "expected message not received!" )
            break
        i( f'{timestamp()} {msg}', )
        got_something = True
for topic_path in args.image or []:
    reader = dds.topic_reader( dds.message.image.create_topic( participant, topic_path ))
    reader.on_data_available( on_image_available )
    reader.run( dds.topic_reader.qos( dds.reliability.best_effort, dds.durability.volatile ) )
    i( f'Waiting for {topic_path}' )
    if args.wait and not reader.wait_for_writers( dds.time( args.wait ) ):
        e( f'Timeout waiting for writers on {topic_path}' )
        sys.exit( 1 )
    readers.append( reader )

# FLEXIBLE
def on_flexible_available( reader ):
    if not ready:
        return
    got_something = False
    while True:
        sample = dds.message.sample_info()
        msg = dds.message.flexible.take_next( reader, sample )
        if not msg:
            if not got_something:
                raise RuntimeError( "expected message not received!" )
            break
        j = msg.json_data()
        if args.field:
            s = j.get( args.field )
            if not s:
                s = json.dumps( j )  # without indent
        else:
            s = dds.json_dump( j, indent=4 )
        i( f'{timestamp()} {sample} {s}', )
        got_something = True
for topic_path in args.flexible_be or []:
    reader = dds.topic_reader( dds.message.flexible.create_topic( participant, topic_path ))
    import json
    reader.on_data_available( on_flexible_available )
    reader.run( dds.topic_reader.qos( dds.reliability.best_effort, dds.durability.volatile ) )
    i( f'Waiting for {topic_path}' )
    if args.wait and not reader.wait_for_writers( dds.time( args.wait ) ):
        e( f'Timeout waiting for writers on {topic_path}' )
        sys.exit( 1 )
    readers.append( reader )
for topic_path in args.flexible or []:
    reader = dds.topic_reader( dds.message.flexible.create_topic( participant, topic_path ))
    import json
    reader.on_data_available( on_flexible_available )
    reader.run( dds.topic_reader.qos() )
    i( f'Waiting for {topic_path}' )
    if args.wait and not reader.wait_for_writers( dds.time( args.wait ) ):
        e( f'Timeout waiting for writers on {topic_path}' )
        sys.exit( 1 )
    readers.append( reader )

# Keep waiting until the user breaks...
def stop():
    global readers, ready
    ready = False
    for reader in readers:
        reader.stop()  # must stop or we get hangs!
    del readers
    sys.exit( 0 )

import signal
def handler(signum, frame):
    #res = input("Ctrl-c was pressed. Do you really want to exit? y/n ")
    #if res == 'y':
    stop()
signal.signal( signal.SIGINT, lambda signum, frame: stop() )

ready = True
if args.time:
    time.sleep( args.time )
    stop()
else:
    i( 'Press ctrl+C to break...' )
    while True:
        time.sleep(1)

