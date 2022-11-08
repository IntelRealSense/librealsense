# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test


device_info = dds.device_info()
device_info.name = "Intel RealSense D435I"
device_info.serial = "036522070660"
device_info.product_line = "D400"
device_info.topic_root = "realdds/D435I/" + device_info.serial


def build( participant ):
    """
    Build a D435i device server for use in DDS
    """
    accel = accel_stream()
    gyro = gyro_stream()
    depth = depth_stream()
    ir1 = ir_stream( 1 )
    ir2 = ir_stream( 2 )
    color = color_stream()
    #
    d435i = dds.device_server( participant, device_info.topic_root )
    d435i.init( [accel, color, depth, gyro, ir1, ir2] )
    return d435i


def accel_stream_profiles():
    return [
        dds.motion_stream_profile( 200, dds.stream_format("MXYZ") ),
        dds.motion_stream_profile( 100, dds.stream_format("MXYZ") )
        ]


def accel_stream():
    stream = dds.accel_stream_server( "Accel", "Motion Module" )
    stream.init_profiles( accel_stream_profiles(), 0 )
    return stream


def gyro_stream_profiles():
    return [
        dds.motion_stream_profile( 400, dds.stream_format("MXYZ") ),
        dds.motion_stream_profile( 200, dds.stream_format("MXYZ") )
        ]


def gyro_stream():
    stream = dds.gyro_stream_server( "Gyro", "Motion Module" )
    stream.init_profiles( gyro_stream_profiles(), 0 )
    return stream


def depth_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("Z16"), 1280,720 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("Z16"), 848,480 ),
        dds.video_stream_profile( 300, dds.stream_format("Z16"), 848,100 ),
        dds.video_stream_profile( 100, dds.stream_format("Z16"), 848,100 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("Z16"), 640,480 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("Z16"), 640,360 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 480,270 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 480,270 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 480,270 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 480,270 ),
        dds.video_stream_profile( 6, dds.stream_format("Z16"), 480,270 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("Z16"), 424,240 ),
        dds.video_stream_profile( 300, dds.stream_format("Z16"), 256,144 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 256,144 )
        ]


def depth_stream():
    stream = dds.depth_stream_server( "Depth", "Stereo Module" )
    stream.init_profiles( depth_stream_profiles(), 0 )
    return stream


def ir_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 1280,800 ),
        dds.video_stream_profile( 25, dds.stream_format("Y16"), 1280,800 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 1280,800 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 1280,800 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("GREY"), 1280,720 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("GREY"), 848,480 ),
        dds.video_stream_profile( 300, dds.stream_format("GREY"), 848,100 ),
        dds.video_stream_profile( 100, dds.stream_format("GREY"), 848,100 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("GREY"), 640,480 ),
        dds.video_stream_profile( 25, dds.stream_format("Y16"), 640,400 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 640,400 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("GREY"), 640,360 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 480,270 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 480,270 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 480,270 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 480,270 ),
        dds.video_stream_profile( 6, dds.stream_format("GREY"), 480,270 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("GREY"), 424,240 )
        ]


def ir_stream( number ):
    stream = dds.ir_stream_server( "Infrared " + str(number), "Stereo Module" )
    stream.init_profiles( ir_stream_profiles(), 0 )
    return stream


def color_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("BYR2"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 1920,1080 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 1920,1080 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 1920,1080 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 1920,1080 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 1920,1080 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 1920,1080 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 1920,1080 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 1920,1080 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 1920,1080 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 1920,1080 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 1920,1080 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 1920,1080 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 1920,1080 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 1280,720 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 1280,720 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 1280,720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 1280,720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 1280,720 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 1280,720 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 1280,720 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 1280,720 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 960,540 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 960,540 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 960,540 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 960,540 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 960,540 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 960,540 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 960,540 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 960,540 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 960,540 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 960,540 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 960,540 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 960,540 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 960,540 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 960,540 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 960,540 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 960,540 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 960,540 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 960,540 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 960,540 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 960,540 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 960,540 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 960,540 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 960,540 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 960,540 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 848,480 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 848,480 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 848,480 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 848,480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 640,480 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 640,480 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 640,480 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 640,480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 640,360 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 640,360 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 640,360 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 640,360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 424,240 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 424,240 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 424,240 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 424,240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 320,240 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 320,240 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 320,240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 320,240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 320,240 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 320,240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 320,240 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 320,240 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 320,240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 320,240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 320,240 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 320,240 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 320,240 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 320,240 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 320,240 ),
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 320,240 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 320,240 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 320,240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 320,180 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"), 320,180 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 320,180 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 320,180 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 320,180 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 320,180 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 320,180 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"), 320,180 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 320,180 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 320,180 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 320,180 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 320,180 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB8"), 320,180 ),
        dds.video_stream_profile( 6, dds.stream_format("Y16"), 320,180 ),
        dds.video_stream_profile( 6, dds.stream_format("BGRA"), 320,180 ), 
        dds.video_stream_profile( 6, dds.stream_format("RGBA"), 320,180 ),
        dds.video_stream_profile( 6, dds.stream_format("RGB2"), 320,180 ),
        dds.video_stream_profile( 6, dds.stream_format("YUYV"), 320,180 )
        ]


def color_stream():
    stream = dds.color_stream_server( "Color",  "RGB Camera" )
    stream.init_profiles( color_stream_profiles(), 0 )
    return stream
