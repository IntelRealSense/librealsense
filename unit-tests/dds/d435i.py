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
    extrinsics = get_extrinsics()
    #
    d435i = dds.device_server( participant, device_info.topic_root )
    d435i.init( [accel, color, depth, gyro, ir1, ir2], [], extrinsics )
    return d435i


def accel_stream_profiles():
    return [
        dds.motion_stream_profile( 200, dds.stream_format("MXYZ") ),
        dds.motion_stream_profile( 100, dds.stream_format("MXYZ") )
        ]


def accel_stream():
    stream = dds.accel_stream_server( "Accel", "Motion Module" )
    stream.init_profiles( accel_stream_profiles(), 0 )
    stream.init_options( motion_module_options() )
    return stream


def gyro_stream_profiles():
    return [
        dds.motion_stream_profile( 400, dds.stream_format("MXYZ") ),
        dds.motion_stream_profile( 200, dds.stream_format("MXYZ") )
        ]


def gyro_stream():
    stream = dds.gyro_stream_server( "Gyro", "Motion Module" )
    stream.init_profiles( gyro_stream_profiles(), 0 )
    stream.init_options( motion_module_options() )
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
    stream.init_options( stereo_module_options() )
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
    stream.init_options( stereo_module_options() )
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
    stream.init_options( rgb_camera_options() )
    return stream

def stereo_module_options():
    options = []
    option_range = dds.dds_option_range()

    option = dds.dds_option( "Exposure", "Stereo Module" )
    option.set_value( 8500 )
    option_range.min = 1
    option_range.max = 200000
    option_range.step = 1
    option_range.default_value = 8500
    option.set_range( option_range )
    option.set_description( "Depth Exposure (usec)" )
    options.append( option )
    option = dds.dds_option( "Gain", "Stereo Module" )
    option.set_value( 16 )
    option_range.min = 16
    option_range.max = 248
    option_range.step = 1
    option_range.default_value = 16
    option.set_range( option_range )
    option.set_description( "UVC image gain" )
    options.append( option )
    option = dds.dds_option( "Enable Auto Exposure", "Stereo Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable Auto Exposure" )
    options.append( option )
    option = dds.dds_option( "Visual Preset", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 5
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Advanced-Mode Preset" )
    options.append( option )
    option = dds.dds_option( "Laser Power", "Stereo Module" )
    option.set_value( 150 )
    option_range.min = 0
    option_range.max = 360
    option_range.step = 30
    option_range.default_value = 150
    option.set_range( option_range )
    option.set_description( "Manual laser power in mw. applicable only when laser power mode is set to Manual" )
    options.append( option )
    option = dds.dds_option( "Emitter Enabled", "Stereo Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 2
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Emitter select, 0-disable all emitters, 1-enable laser, 2-enable laser auto (opt), 3-enable LED (opt)" )
    options.append( option )
    option = dds.dds_option( "Frames Queue Size", "Stereo Module" )
    option.set_value( 16 )
    option_range.min = 0
    option_range.max = 32
    option_range.step = 1
    option_range.default_value = 16
    option.set_range( option_range )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.dds_option( "Error Polling Enabled", "Stereo Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Enable / disable polling of camera internal errors" )
    options.append( option )
    option = dds.dds_option( "Output Trigger Enabled", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Generate trigger from the camera to external device once per frame" )
    options.append( option )
    option = dds.dds_option( "Depth Units", "Stereo Module" )
    option.set_value( 0.001 )
    option_range.min = 1e-06
    option_range.max = 0.01
    option_range.step = 1e-06
    option_range.default_value = 0.001
    option.set_range( option_range )
    option.set_description( "Number of meters represented by a single depth unit" )
    options.append( option )
    option = dds.dds_option( "Stereo Baseline", "Stereo Module" )
    option.set_value( 49.864 )
    option_range.min = 49.864
    option_range.max = 49.864
    option_range.step = 0
    option_range.default_value = 49.864
    option.set_range( option_range )
    option.set_description( "Distance in mm between the stereo imagers" )
    options.append( option )
    option = dds.dds_option( "Inter Cam Sync Mode", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 260
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Inter-camera synchronization mode: 0:Default, 1:Master, 2:Slave, 3:Full Salve, 4-258:Genlock with burst count of 1-255 frames for each trigger, 259 and 260 for two frames per trigger with laser ON-OFF and OFF-ON." )
    options.append( option )
    option = dds.dds_option( "Emitter On Off", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Alternating emitter pattern, toggled on/off on per-frame basis" )
    options.append( option )
    option = dds.dds_option( "Global Time Enabled", "Stereo Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )
    option = dds.dds_option( "Emitter Always On", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Emitter always on mode: 0:disabled(default), 1:enabled" )
    options.append( option )
    option = dds.dds_option( "Hdr Enabled", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.dds_option( "Sequence Name", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 3
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.dds_option( "Sequence Size", "Stereo Module" )
    option.set_value( 2 )
    option_range.min = 2
    option_range.max = 2
    option_range.step = 1
    option_range.default_value = 2
    option.set_range( option_range )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.dds_option( "Sequence Id", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 2
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.dds_option( "Auto Exposure Limit", "Stereo Module" )
    option.set_value( 200000 )
    option_range.min = 1
    option_range.max = 200000
    option_range.step = 1
    option_range.default_value = 8500
    option.set_range( option_range )
    option.set_description( "Exposure limit is in microseconds. If the requested exposure limit is greater than frame time, it will be set to frame time at runtime. Setting will not take effect until next streaming session." )
    options.append( option )
    option = dds.dds_option( "Auto Gain Limit", "Stereo Module" )
    option.set_value( 248 )
    option_range.min = 16
    option_range.max = 248
    option_range.step = 1
    option_range.default_value = 16
    option.set_range( option_range )
    option.set_description( "Gain limits ranges from 16 to 248. If the requested gain limit is less than 16, it will be set to 16. If the requested gain limit is greater than 248, it will be set to 248. Setting will not take effect until next streaming session." )
    options.append( option )
    option = dds.dds_option( "Auto Exposure Limit Toggle", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Toggle Auto-Exposure Limit" )
    options.append( option )
    option = dds.dds_option( "Auto Gain Limit Toggle", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Toggle Auto-Gain Limit" )
    options.append( option )

    return options

def rgb_camera_options():
    options = []
    option_range = dds.dds_option_range()

    option = dds.dds_option( "Backlight Compensation", "RGB Camera" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Enable / disable backlight compensation" )
    options.append( option )
    option = dds.dds_option( "Brightness", "RGB Camera" )
    option.set_value( 0 )
    option_range.min = -64
    option_range.max = 64
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "UVC image brightness" )
    options.append( option )
    option = dds.dds_option( "Contrast", "RGB Camera" )
    option.set_value( 50 )
    option_range.min = 0
    option_range.max = 100
    option_range.step = 1
    option_range.default_value = 50
    option.set_range( option_range )
    option.set_description( "UVC image contrast" )
    options.append( option )
    option = dds.dds_option( "Exposure", "RGB Camera" )
    option.set_value( 156 )
    option_range.min = 1
    option_range.max = 10000
    option_range.step = 1
    option_range.default_value = 156
    option.set_range( option_range )
    option.set_description( "Controls exposure time of color camera. Setting any value will disable auto exposure" )
    options.append( option )
    option = dds.dds_option( "Gain", "RGB Camera" )
    option.set_value( 6 )
    option_range.min = 0
    option_range.max = 128
    option_range.step = 1
    option_range.default_value = 64
    option.set_range( option_range )
    option.set_description( "UVC image gain" )
    options.append( option )
    option = dds.dds_option( "Gamma", "RGB Camera" )
    option.set_value( 300 )
    option_range.min = 100
    option_range.max = 500
    option_range.step = 1
    option_range.default_value = 300
    option.set_range( option_range )
    option.set_description( "UVC image gamma setting" )
    options.append( option )
    option = dds.dds_option( "Hue", "RGB Camera" )
    option.set_value( 0 )
    option_range.min = -180
    option_range.max = 180
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "UVC image hue" )
    options.append( option )
    option = dds.dds_option( "Saturation", "RGB Camera" )
    option.set_value( 64 )
    option_range.min = 0
    option_range.max = 100
    option_range.step = 1
    option_range.default_value = 64
    option.set_range( option_range )
    option.set_description( "UVC image saturation setting" )
    options.append( option )
    option = dds.dds_option( "Sharpness", "RGB Camera" )
    option.set_value( 50 )
    option_range.min = 0
    option_range.max = 100
    option_range.step = 1
    option_range.default_value = 50
    option.set_range( option_range )
    option.set_description( "UVC image sharpness setting" )
    options.append( option )
    option = dds.dds_option( "White Balance", "RGB Camera" )
    option.set_value( 4600 )
    option_range.min = 2800
    option_range.max = 6500
    option_range.step = 10
    option_range.default_value = 4600
    option.set_range( option_range )
    option.set_description( "Controls white balance of color image. Setting any value will disable auto white balance" )
    options.append( option )
    option = dds.dds_option( "Enable Auto Exposure", "RGB Camera" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable / disable auto-exposure" )
    options.append( option )
    option = dds.dds_option( "Enable Auto White Balance", "RGB Camera" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable / disable auto-white-balance" )
    options.append( option )
    option = dds.dds_option( "Frames Queue Size", "RGB Camera" )
    option.set_value( 16 )
    option_range.min = 0
    option_range.max = 32
    option_range.step = 1
    option_range.default_value = 16
    option.set_range( option_range )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.dds_option( "Power Line Frequency", "RGB Camera" )
    option.set_value( 3 )
    option_range.min = 0
    option_range.max = 3
    option_range.step = 1
    option_range.default_value = 3
    option.set_range( option_range )
    option.set_description( "Power Line Frequency" )
    options.append( option )
    option = dds.dds_option( "Auto Exposure Priority", "RGB Camera" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Restrict Auto-Exposure to enforce constant FPS rate. Turn ON to remove the restrictions (may result in FPS drop)" )
    options.append( option )
    option = dds.dds_option( "Global Time Enabled", "RGB Camera" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )

    return options

def motion_module_options():
    options = []
    option_range = dds.dds_option_range()

    option = dds.dds_option( "Frames Queue Size", "Motion Module" )
    option.set_value( 16 )
    option_range.min = 0
    option_range.max = 32
    option_range.step = 1
    option_range.default_value = 16
    option.set_range( option_range )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.dds_option( "Enable Motion Correction", "Motion Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable/Disable Automatic Motion Data Correction" )
    options.append( option )
    option = dds.dds_option( "Global Time Enabled", "Motion Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )

    return options

def get_extrinsics():
    extrinsics = {}

    extr = dds.extrinsics();
    extr.rotation = (0.9951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.02056039869785309,-0.00510153453797102,-0.011584061197936535)
    extrinsics[("Accel","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.005520000122487545,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Accel","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Accel","Gyro")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.005520000122487545,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Accel","Infrared 1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04434395954012871,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Accel","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.02059810981154442,0.0050893244333565235,0.011522269807755947)
    extrinsics[("Color","Accel")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.015078110620379448,-1.0675736120902002e-05,-0.00021772991749458015)
    extrinsics[("Color","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.02059810981154442,0.0050893244333565235,0.011522269807755947)
    extrinsics[("Color","Gyro")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.015078110620379448,-1.0675736120902002e-05,-0.00021772991749458015)
    extrinsics[("Color","Infrared 1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.06494206935167313,-1.0675736120902002e-05,-0.00021772991749458015)
    extrinsics[("Color","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.005520000122487545,0.005100000184029341,0.011739999987185001)
    extrinsics[("Depth","Accel")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.015078714117407799,4.601718956109835e-06,0.00017121469136327505)
    extrinsics[("Depth","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.005520000122487545,0.005100000184029341,0.011739999987185001)
    extrinsics[("Depth","Gyro")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Depth","Infrared 1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04986396059393883,0.0,0.0)
    extrinsics[("Depth","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Gyro","Accel")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.02056039869785309,-0.00510153453797102,-0.011584061197936535)
    extrinsics[("Gyro","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.005520000122487545,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Gyro","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.005520000122487545,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Gyro","Infrared 1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04434395954012871,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Gyro","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.005520000122487545,0.005100000184029341,0.011739999987185001)
    extrinsics[("Infrared 1","Accel")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.015078714117407799,4.601718956109835e-06,0.00017121469136327505)
    extrinsics[("Infrared 1","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared 1","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.005520000122487545,0.005100000184029341,0.011739999987185001)
    extrinsics[("Infrared 1","Gyro")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04986396059393883,0.0,0.0)
    extrinsics[("Infrared 1","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04434395954012871,0.005100000184029341,0.011739999987185001)
    extrinsics[("Infrared 2","Accel")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.06494243443012238,-1.5723688193247654e-05,1.7402038793079555e-05)
    extrinsics[("Infrared 2","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04986396059393883,0.0,0.0)
    extrinsics[("Infrared 2","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04434395954012871,0.005100000184029341,0.011739999987185001)
    extrinsics[("Infrared 2","Gyro")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04986396059393883,0.0,0.0)
    extrinsics[("Infrared 2","Infrared 1")] = extr

    return extrinsics

