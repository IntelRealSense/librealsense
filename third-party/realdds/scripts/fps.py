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
args.add_argument( '--depth', action='store_true', help='stream Depth' )
args.add_argument( '--color', action='store_true', help='stream Color' )
args.add_argument( '--ir1', action='store_true', help='stream IR1' )
args.add_argument( '--ir2', action='store_true', help='stream IR2' )
args.add_argument( '--fps', metavar='5,15,30,60,90', type=int, default=30, help='Frames per second' )
def res_arg(x):
    import re
    m = re.fullmatch( r'(\d+)x(\d+)', x )
    if not m:
        raise ValueError( f'--res should be WIDTHxHEIGHT' )
    return [int(m.group(1)), int(m.group(2))]
args.add_argument( '--res', metavar='WxH', type=res_arg, default=[1280,720], help='Resolution as WIDTHxHEIGHT' )
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

streams = []
if args.depth:
    streams.append( 'Depth' )
if args.color:
    streams.append( 'Color' )
if args.ir1:
    streams.append( 'Infrared_1' )
if args.ir2:
    streams.append( 'Infrared_2' )
if not streams:
    streams = ['Depth', 'Color']

n_stream_frames = {}
for s in streams:
    n_stream_frames[s] = 0
capturing = True
def on_image( stream, image, sample ):
    #d( f'----> {image}')
    global n_stream_frames, capturing
    if capturing:
        n_stream_frames[stream.name()] += 1

fps = args.fps
width = args.res[0]
height = args.res[1]
type_format = { 'depth': str(dds.video_encoding.z16), 'color': str(dds.video_encoding.yuyv), 'ir': str(dds.video_encoding.y8) }

i( device.n_streams(), 'streams available' )
stream_profiles = {}
for stream in device.streams():
    #profiles = stream.profiles()
    if stream.name() not in streams:
        continue

    #stream.default_profile().to_string()[1:-1]
    profile = [fps,type_format[stream.type_string()],width,height]
    i( f'   {stream.sensor_name()} / {stream.name()} {profile}' )
    stream_profiles[stream.name()] = profile

device.send_control( { 'id': 'open-streams',
                       'stream-profiles': stream_profiles }, True )

for stream in device.streams():
    if stream.name() not in streams:
        continue

    stream.on_data_available( on_image )
    stream_topic = 'rt/' + info.topic_root + '_' + stream.name()
    stream.open( stream_topic, dds.subscriber( participant ) )
    stream.start_streaming()

# Wait until we have at least one frame from each
tries = 5
while tries > 0:
    for n in n_stream_frames.values():
        if n <= 0:
            break
    else:
        break
    time.sleep( 1 )
    tries -= 1
else:
    raise RuntimeError( f'timed out waiting for all frames to arrive {n_stream_frames}' )
time_started = time.time()
i( f'Starting @ {time_started}' )
n_frames_at_start = dict( n_stream_frames )
capturing = True

# Measure number of frames in a period of time
time.sleep( args.time )

capturing = False
time_stopped = time.time()
i( f'Stopping @ {time_stopped}' )
for stream in device.streams():
    if stream.name() in streams:
        stream.stop_streaming()
for stream in device.streams():
    if stream.name() in streams:
        stream.close()

time_delta = time_stopped - time_started
i( 'Elapsed :', time_delta )
for s in streams:
    n_frames = n_stream_frames[s] - n_frames_at_start[s]
    i( f'   {s:16}: {n_frames:4} frames @ FPS {n_frames / time_delta}' )

if args.quiet:
    print( min( result_color, result_depth ) / time_delta )

