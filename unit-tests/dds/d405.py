# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test


device_info = dds.device_info()
device_info.name = "Intel RealSense D405"
device_info.serial = "123622270732"
device_info.product_line = "D400"
device_info.topic_root = "realdds/D405/" + device_info.serial


def build( participant ):
    """
    Build a D405 device server for use in DDS
    """
    depth = depth_stream()
    ir0 = colored_infrared_stream()
    ir1 = ir_stream( 1 )
    ir2 = ir_stream( 2 )
    color = color_stream()
    extrinsics = get_extrinsics()
    #
    d405 = dds.device_server( participant, device_info.topic_root )
    d405.init( [ir0, ir1, ir2, color, depth], [], extrinsics )
    return d405


def colored_infrared_stream():
    stream = dds.ir_stream_server( "Infrared", "Stereo Module" )
    stream.init_profiles( colored_infrared_stream_profiles(), 0 )
    stream.init_options( stereo_module_options() )
    return stream


def colored_infrared_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.stream_format("UYVY"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("UYVY"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("UYVY"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("BGRA"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGBA"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB2"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB8"), 1280, 720 ),
        dds.video_stream_profile( 90, dds.stream_format("UYVY"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("UYVY"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("UYVY"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("UYVY"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("UYVY"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("UYVY"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("UYVY"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("UYVY"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("UYVY"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("UYVY"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("UYVY"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("UYVY"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("UYVY"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("UYVY"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("UYVY"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("UYVY"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("UYVY"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("UYVY"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("UYVY"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("UYVY"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("UYVY"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("UYVY"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("UYVY"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("UYVY"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("UYVY"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("RGB8"), 424, 240 )
        ]                                                  


def depth_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("Z16"), 1280, 720 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("Z16"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("Z16"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("Z16"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("Z16"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("Z16"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("Z16"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("Z16"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("Z16"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("Z16"), 256, 144 )
        ]


def depth_stream():
    stream = dds.depth_stream_server( "Depth", "Stereo Module" )
    stream.init_profiles( depth_stream_profiles(), 5 )
    stream.init_options( stereo_module_options() )
    return stream


def ir_stream_profiles():
    return [
        dds.video_stream_profile( 25, dds.stream_format("Y16"),  1288, 808 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  1288, 808 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 1280, 720 ),
        dds.video_stream_profile( 5,  dds.stream_format("GREY"), 1280, 720 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 848, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("GREY"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 640, 480 ),
        dds.video_stream_profile( 5,  dds.stream_format("GREY"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 640, 360 ),
        dds.video_stream_profile( 5,  dds.stream_format("GREY"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 480, 270 ),
        dds.video_stream_profile( 5,  dds.stream_format("GREY"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("GREY"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("GREY"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("GREY"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("GREY"), 424, 240 ),
        dds.video_stream_profile( 5,  dds.stream_format("GREY"), 424, 240 )
        ]


def ir_stream( number ):
    stream = dds.ir_stream_server( "Infrared " + str(number), "Stereo Module" )
    stream.init_profiles( ir_stream_profiles(), 0 )
    stream.init_options( stereo_module_options() )
    return stream


def color_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"),  1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 1280, 720 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 1280, 720 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 1280, 720 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB8"), 1280, 720 ),
        dds.video_stream_profile( 5 , dds.stream_format("Y16"),  1280, 720 ),
        dds.video_stream_profile( 5 , dds.stream_format("BGRA"), 1280, 720 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGBA"), 1280, 720 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB2"), 1280, 720 ),
        dds.video_stream_profile( 5 , dds.stream_format("YUYV"), 1280, 720 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("Y16"),  848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("YUYV"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"),  848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"),  848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 848, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB8"), 848, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("Y16"),  848, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("BGRA"), 848, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGBA"), 848, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB2"), 848, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("YUYV"), 848, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("Y16"),  640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("YUYV"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"),  640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"),  640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 640, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB8"), 640, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("Y16"),  640, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("BGRA"), 640, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGBA"), 640, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB2"), 640, 480 ),
        dds.video_stream_profile( 5 , dds.stream_format("YUYV"), 640, 480 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("Y16"),  640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("YUYV"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"),  640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"),  640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 640, 360 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB8"), 640, 360 ),
        dds.video_stream_profile( 5 , dds.stream_format("Y16"),  640, 360 ),
        dds.video_stream_profile( 5 , dds.stream_format("BGRA"), 640, 360 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGBA"), 640, 360 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB2"), 640, 360 ),
        dds.video_stream_profile( 5 , dds.stream_format("YUYV"), 640, 360 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("Y16"),  480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("YUYV"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"),  480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"),  480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 480, 270 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB8"), 480, 270 ),
        dds.video_stream_profile( 5 , dds.stream_format("Y16"),  480, 270 ),
        dds.video_stream_profile( 5 , dds.stream_format("BGRA"), 480, 270 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGBA"), 480, 270 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB2"), 480, 270 ),
        dds.video_stream_profile( 5 , dds.stream_format("YUYV"), 480, 270 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("Y16"),  424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 90, dds.stream_format("YUYV"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("Y16"),  424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 60, dds.stream_format("YUYV"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("Y16"),  424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 30, dds.stream_format("YUYV"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("Y16"),  424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 15, dds.stream_format("YUYV"), 424, 240 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB8"), 424, 240 ),
        dds.video_stream_profile( 5 , dds.stream_format("Y16"),  424, 240 ),
        dds.video_stream_profile( 5 , dds.stream_format("BGRA"), 424, 240 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGBA"), 424, 240 ),
        dds.video_stream_profile( 5 , dds.stream_format("RGB2"), 424, 240 ),
        dds.video_stream_profile( 5 , dds.stream_format("YUYV"), 424, 240 )
        ]


def color_stream():
    stream = dds.color_stream_server( "Color",  "Stereo Module" )
    stream.init_profiles( color_stream_profiles(), 30 )
    stream.init_options( stereo_module_options() )
    return stream

def stereo_module_options():
    options = []
    option_range = dds.dds_option_range()

    option = dds.dds_option( "Backlight Compensation", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "Enable / disable backlight compensation" )
    options.append( option )
    option = dds.dds_option( "Brightness", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = -64
    option_range.max = 64
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "UVC image brightness" )
    options.append( option )
    option = dds.dds_option( "Contrast", "Stereo Module" )
    option.set_value( 50 )
    option_range.min = 0
    option_range.max = 100
    option_range.step = 1
    option_range.default_value = 50
    option.set_range( option_range )
    option.set_description( "UVC image contrast" )
    options.append( option )
    option = dds.dds_option( "Exposure", "Stereo Module" )
    option.set_value( 33000 )
    option_range.min = 1
    option_range.max = 165000
    option_range.step = 1
    option_range.default_value = 33000
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
    option = dds.dds_option( "Gamma", "Stereo Module" )
    option.set_value( 300 )
    option_range.min = 100
    option_range.max = 500
    option_range.step = 1
    option_range.default_value = 300
    option.set_range( option_range )
    option.set_description( "UVC image gamma setting" )
    options.append( option )
    option = dds.dds_option( "Hue", "Stereo Module" )
    option.set_value( 0 )
    option_range.min = -180
    option_range.max = 180
    option_range.step = 1
    option_range.default_value = 0
    option.set_range( option_range )
    option.set_description( "UVC image hue" )
    options.append( option )
    option = dds.dds_option( "Saturation", "Stereo Module" )
    option.set_value( 64 )
    option_range.min = 0
    option_range.max = 100
    option_range.step = 1
    option_range.default_value = 64
    option.set_range( option_range )
    option.set_description( "UVC image saturation setting" )
    options.append( option )
    option = dds.dds_option( "Sharpness", "Stereo Module" )
    option.set_value( 50 )
    option_range.min = 0
    option_range.max = 100
    option_range.step = 1
    option_range.default_value = 50
    option.set_range( option_range )
    option.set_description( "UVC image sharpness setting" )
    options.append( option )
    option = dds.dds_option( "White Balance", "Stereo Module" )
    option.set_value( 4600 )
    option_range.min = 2800
    option_range.max = 6500
    option_range.step = 10
    option_range.default_value = 4600
    option.set_range( option_range )
    option.set_description( "Controls white balance of color image. Setting any value will disable auto white balance" )
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
    option = dds.dds_option( "Enable Auto White Balance", "Stereo Module" )
    option.set_value( 1 )
    option_range.min = 0
    option_range.max = 1
    option_range.step = 1
    option_range.default_value = 1
    option.set_range( option_range )
    option.set_description( "Enable / disable auto-white-balance" )
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
    option = dds.dds_option( "Frames Queue Size", "Stereo Module" )
    option.set_value( 16 )
    option_range.min = 0
    option_range.max = 32
    option_range.step = 1
    option_range.default_value = 16
    option.set_range( option_range )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.dds_option( "Power Line Frequency", "Stereo Module" )
    option.set_value( 3 )
    option_range.min = 0
    option_range.max = 3
    option_range.step = 1
    option_range.default_value = 3
    option.set_range( option_range )
    option.set_description( "Power Line Frequency" )
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
    option.set_value( 0.0001 )
    option_range.min = 1e-06
    option_range.max = 0.01
    option_range.step = 1e-06
    option_range.default_value = 0.001
    option.set_range( option_range )
    option.set_description( "Number of meters represented by a single depth unit" )
    options.append( option )
    option = dds.dds_option( "Stereo Baseline", "Stereo Module" )
    option.set_value( 17.9998 )
    option_range.min = 17.9998
    option_range.max = 17.9998
    option_range.step = 0
    option_range.default_value = 17.9998
    option.set_range( option_range )
    option.set_description( "Distance in mm between the stereo imagers" )
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
    option.set_value( 165000 )
    option_range.min = 1
    option_range.max = 165000
    option_range.step = 1
    option_range.default_value = 33000
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

def get_extrinsics():
    extrinsics = {}

    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,0.0014127078466117382,0.0053943851962685585,-0.001408747979439795,0.9999987483024597,-0.0007378021255135536,-0.005395420826971531,0.000730191299226135,0.9999851584434509)
    extr.translation = (6.110243703005835e-05,1.1573569281608798e-05,6.581118213944137e-06)
    extrinsics[("Color","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,0.0014127078466117382,0.0053943851962685585,-0.001408747979439795,0.9999987483024597,-0.0007378021255135536,-0.005395420826971531,0.000730191299226135,0.9999851584434509)
    extr.translation = (6.110243703005835e-05,1.1573569281608798e-05,6.581118213944137e-06)
    extrinsics[("Color","Infrared") ] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,0.0014127078466117382,0.0053943851962685585,-0.001408747979439795,0.9999987483024597,-0.0007378021255135536,-0.005395420826971531,0.000730191299226135,0.9999851584434509)
    extr.translation = (6.110243703005835e-05,1.1573569281608798e-05,6.581118213944137e-06)
    extrinsics[("Color","Infrared 1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,0.0014127078466117382,0.0053943851962685585,-0.001408747979439795,0.9999987483024597,-0.0007378021255135536,-0.005395420826971531,0.000730191299226135,0.9999851584434509)
    extr.translation = (-0.017938679084181786,1.1573569281608798e-05,6.581118213944137e-06)
    extrinsics[("Color","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,-0.001408747979439795,-0.005395420826971531,0.0014127078466117382,0.9999987483024597,0.000730191299226135,0.0053943851962685585,-0.0007378021255135536,0.9999851584434509)
    extr.translation = (-6.115333235356957e-05,-1.1482620720926207e-05,-6.2597978285339195e-06)
    extrinsics[("Depth","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Depth","Infrared")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Depth","Infrared 1")] = extr 
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.017999781295657158,0.0,0.0)
    extrinsics[("Depth","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,-0.001408747979439795,-0.005395420826971531,0.0014127078466117382,0.9999987483024597,0.000730191299226135,0.0053943851962685585,-0.0007378021255135536,0.9999851584434509)
    extr.translation = (-6.115333235356957e-05,-1.1482620720926207e-05,-6.2597978285339195e-06)
    extrinsics[("Infrared","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared","Infrared 1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.017999781295657158,0.0,0.0)
    extrinsics[("Infrared","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,-0.001408747979439795,-0.005395420826971531,0.0014127078466117382,0.9999987483024597,0.000730191299226135,0.0053943851962685585,-0.0007378021255135536,0.9999851584434509)
    extr.translation = (-6.115333235356957e-05,-1.1482620720926207e-05,-6.2597978285339195e-06)
    extrinsics[("Infrared 1","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared 1","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared 1","Infrared")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.017999781295657158,0.0,0.0)
    extrinsics[("Infrared 1","Infrared 2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999844431877136,-0.001408747979439795,-0.005395420826971531,0.0014127078466117382,0.9999987483024597,0.000730191299226135,0.0053943851962685585,-0.0007378021255135536,0.9999851584434509)
    extr.translation = (0.01793834939599037,-3.683977774926461e-05,-0.00010337619460187852)
    extrinsics[("Infrared 2","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.017999781295657158,0.0,0.0)
    extrinsics[("Infrared 2","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.017999781295657158,0.0,0.0)
    extrinsics[("Infrared 2","Infrared")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.017999781295657158,0.0,0.0)
    extrinsics[("Infrared 2","Infrared 1")] = extr
    
    return extrinsics

