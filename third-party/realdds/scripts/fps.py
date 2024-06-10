# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from argparse import ArgumentParser
args = ArgumentParser()
args.add_argument( '--debug', action='store_true', help='enable debug mode' )
args.add_argument( '--quiet', action='store_true', help='No output; just the minimum FPS as a number' )
args.add_argument( '--device', metavar='<path>', required=True, help='the topic root for the device' )
def time_arg(x):
    t = int(x)
    if t <= 0:
        raise ValueError( f'--time should be a positive integer' )
    return t
args.add_argument( '--time', metavar='<seconds>', type=time_arg, default=5, help='how long to gather frames for (default=5)' )
def domain_arg(x):
    t = int(x)
    if t <= 0 or t > 232:
        raise ValueError( f'--domain should be [0,232]' )
    return t
args.add_argument( '--domain', metavar='<0-232>', type=domain_arg, default=-1, help='DDS domain to use (default=0)' )
args.add_argument( '--with-metadata', action='store_true', help='stream with metadata, if available (default off)' )
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
if not args.with_metadata:
    settings['device'] = { 'metadata' : False };

participant = dds.participant()
participant.init( dds.load_rs_settings( settings ), args.domain )

# Most important is the topic-root: this assumes we know it in advance and do not have to
# wait for a device-info message (which would complicate the code here).
info = dds.message.device_info()
info.name = 'Dummy Device'
info.topic_root = args.device

# Create the device and initialize
# The server must be up and running, or the init will time out!
device = dds.device( participant, info )
try:
    i( 'Looking for device at', info.topic_root, '...' )
    device.wait_until_ready()  # If unavailable before timeout, this throws
except:
    e( 'Cannot find device' )
    sys.exit(1)

n_depth = 0
def on_depth_image( stream, image ):
    #d( f'----> depth {image}')
    global n_depth
    n_depth += 1

n_color = 0
def on_color_image( stream, image ):
    #d( f'----> color {image}')
    global n_color
    n_color += 1

i( device.n_streams(), 'streams available' )
depth_stream = None
color_stream = None
for stream in device.streams():
    profiles = stream.profiles()
    i( '   ', stream.sensor_name(), '/', stream.default_profile().to_string()[1:-1] )
    if stream.type_string() == 'depth':
        depth_stream = stream
        stream.on_data_available( on_depth_image )
    elif stream.type_string() == 'color':
        color_stream = stream
        stream.on_data_available( on_color_image )
    else:
        continue

    stream_topic = 'rt/' + info.topic_root + '_' + stream.name()
    stream.open( stream_topic, dds.subscriber( participant ) )
    stream.start_streaming()

# Wait until we have at least one frame from each
tries = 3
while tries > 0:
    if n_depth > 0  and  n_color > 0:
        break
    time.sleep( 1 )
else:
    raise RuntimeError( 'timed out waiting for frames to arrive' )
n_depth = n_color = 0
time_started = time.time()
i( 'Starting                       :', time_started )

# Measure number of frames in a period of time
time.sleep( args.time )

result_depth = n_depth
result_color = n_color
time_stopped = time.time()
i( 'Stopping                       :', time_stopped )
depth_stream.stop_streaming()
color_stream.stop_streaming()
depth_stream.close()
color_stream.close()

time_delta = time_stopped - time_started
i( 'Elapsed                        :', time_delta )
i( 'Number of Depth frames received:', result_depth, '=', result_depth / time_delta, 'fps' )
i( 'Number of Color frames received:', result_color, '=', result_color / time_delta, 'fps' )

if args.quiet:
    print( min( result_color, result_depth ) / time_delta )

