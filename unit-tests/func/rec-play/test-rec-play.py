# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import pyrealsense2 as rs, os, time, tempfile
from rspy import devices, log, test

current_dir = os.path.dirname( os.path.abspath( __file__ ))

f = tempfile.NamedTemporaryFile( mode='w+', suffix='.bag', prefix='rnp_', dir=current_dir)
file_name = f.name
print(file_name)
pipeline = rs.pipeline()
cfg = rs.config()
cfg.enable_record_to_file( file_name )
pipeline.start( cfg )
time.sleep(3)
pipeline.stop()
pipeline = rs.pipeline()
cfg = rs.config()
cfg.enable_device_from_file( file_name )
cfg.enable_stream(rs.stream.depth, rs.format.z16, 30)
pipeline.start( cfg )
device = pipeline.get_active_profile().get_device()
playback = rs.playback( device )
playback.resume()
pipeline.wait_for_frames() # fails, no frames arrive
print("waited")
pipeline.stop()
#
#
# dev = test.find_first_device_or_exit()
# recorder = rs.recorder( "recordings.bag", dev )
# depth_sensor = dev.first_depth_sensor()
# color_sensor = dev.first_color_sensor()
#
# cp = next(p for p in color_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.color
#                 and p.format() == rs.format.yuyv
#                 and p.as_video_stream_profile().width() == 1280
#                 and p.as_video_stream_profile().height() == 720)
#
# dp = next(p for p in
#                 depth_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.depth
#                 and p.format() == rs.format.z16
#                 and p.as_video_stream_profile().width() == 1024
#                 and p.as_video_stream_profile().height() == 768)
#
# depth_sensor.open( dp )
# depth_sensor.start( lambda f: None )
# color_sensor.open( cp )
# color_sensor.start( lambda f: None )
#
# time.sleep(9)
# recorder.pause()
# color_sensor.stop()
# color_sensor.close()
# depth_sensor.stop()
# depth_sensor.close()
#
# recorder = None
# ctx = rs.context()
# playback = ctx.load_device( "recordings.bag" )
#
# frames = False
# def got_frames(frame):
#     global frames
#     frames = True
#
# depth_sensor = playback.first_depth_sensor()
# color_sensor = playback.first_color_sensor()
# cp = next(p for p in color_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.color
#                 and p.format() == rs.format.yuyv
#                 and p.as_video_stream_profile().width() == 1280
#                 and p.as_video_stream_profile().height() == 720)
#
# dp = next(p for p in
#                 depth_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.depth
#                 and p.format() == rs.format.z16
#                 and p.as_video_stream_profile().width() == 1024
#                 and p.as_video_stream_profile().height() == 768)
#
# depth_sensor.open( dp )
# depth_sensor.start( got_frames )
#
# color_sensor.open( cp )
# color_sensor.start( got_frames )
#
# time.sleep(3)
# print("res:", frames)
#
#
#
#
#
#
# sync = rs.syncer()
# dev = test.find_first_device_or_exit()
# recorder = rs.recorder( "recordings.bag", dev )
# depth_sensor = dev.first_depth_sensor()
# color_sensor = dev.first_color_sensor()
#
# cp = next(p for p in color_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.color
#                 and p.format() == rs.format.yuyv
#                 and p.as_video_stream_profile().width() == 1280
#                 and p.as_video_stream_profile().height() == 720)
#
# dp = next(p for p in
#                 depth_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.depth
#                 and p.format() == rs.format.z16
#                 and p.as_video_stream_profile().width() == 1024
#                 and p.as_video_stream_profile().height() == 768)
#
# depth_sensor.open( dp )
# depth_sensor.start( sync )
# color_sensor.open( cp )
# color_sensor.start( sync )
#
# time.sleep(9)
# recorder.pause()
# color_sensor.stop()
# color_sensor.close()
# depth_sensor.stop()
# depth_sensor.close()
#
# recorder = None
# ctx = rs.context()
# playback = ctx.load_device( "recordings.bag" )
#
# depth_sensor = playback.first_depth_sensor()
# color_sensor = playback.first_color_sensor()
# cp = next(p for p in color_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.color
#                 and p.format() == rs.format.yuyv
#                 and p.as_video_stream_profile().width() == 1280
#                 and p.as_video_stream_profile().height() == 720)
#
# dp = next(p for p in
#                 depth_sensor.profiles if p.fps() == 30
#                 and p.stream_type() == rs.stream.depth
#                 and p.format() == rs.format.z16
#                 and p.as_video_stream_profile().width() == 1024
#                 and p.as_video_stream_profile().height() == 768)
#
# # sync = rs.syncer()
#
# depth_sensor.open( dp )
# depth_sensor.start( sync )
#
# color_sensor.open( cp )
# color_sensor.start( sync )
# sync.wait_for_frames()
# print("waited")
#
#
