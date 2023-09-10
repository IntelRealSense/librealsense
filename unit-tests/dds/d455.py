# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test


device_info = dds.message.device_info()
device_info.name = "Intel RealSense D455"
device_info.serial = "114222251267"
device_info.product_line = "D400"
device_info.topic_root = "realdds/D455/" + device_info.serial


def build( participant ):
    """
    Build a D455 device server for use in DDS
    """
    motion = motion_stream()
    depth = depth_stream()
    ir1 = ir_stream( 1 )
    ir2 = ir_stream( 2 )
    color = color_stream()
    extrinsics = get_extrinsics()
    #
    d455 = dds.device_server( participant, device_info.topic_root )
    d455.init( [motion, depth, ir1, ir2, color], [], extrinsics )
    return d455


def gyro_stream_profiles():
    return [
        dds.motion_stream_profile( 200 ),
        dds.motion_stream_profile( 400 )
        ]


def motion_stream():
    stream = dds.motion_stream_server( "Motion", "Motion Module" )
    stream.init_profiles( gyro_stream_profiles(), 0 )
    stream.init_options( motion_module_options() )
    return stream


def colored_infrared_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.uyvy, 1280, 720 ),
        dds.video_stream_profile( 15, dds.video_encoding.uyvy, 1280, 720 ),
        dds.video_stream_profile( 5 , dds.video_encoding.uyvy, 1280, 720 ),
        dds.video_stream_profile( 90, dds.video_encoding.uyvy, 848, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.uyvy, 848, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.uyvy, 848, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.uyvy, 848, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.uyvy, 848, 480 ),
        dds.video_stream_profile( 100, dds.video_encoding.uyvy, 848, 100 ),
        dds.video_stream_profile( 90, dds.video_encoding.uyvy, 640, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.uyvy, 640, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.uyvy, 640, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.uyvy, 640, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.uyvy, 640, 480 ),
        dds.video_stream_profile( 90, dds.video_encoding.uyvy, 640, 360 ),
        dds.video_stream_profile( 60, dds.video_encoding.uyvy, 640, 360 ),
        dds.video_stream_profile( 30, dds.video_encoding.uyvy, 640, 360 ),
        dds.video_stream_profile( 15, dds.video_encoding.uyvy, 640, 360 ),
        dds.video_stream_profile( 5 , dds.video_encoding.uyvy, 640, 360 ),
        dds.video_stream_profile( 90, dds.video_encoding.uyvy, 480, 270 ),
        dds.video_stream_profile( 60, dds.video_encoding.uyvy, 480, 270 ),
        dds.video_stream_profile( 30, dds.video_encoding.uyvy, 480, 270 ),
        dds.video_stream_profile( 15, dds.video_encoding.uyvy, 480, 270 ),
        dds.video_stream_profile( 5 , dds.video_encoding.uyvy, 480, 270 ),
        dds.video_stream_profile( 90, dds.video_encoding.uyvy, 424, 240 ),
        dds.video_stream_profile( 60, dds.video_encoding.uyvy, 424, 240 ),
        dds.video_stream_profile( 30, dds.video_encoding.uyvy, 424, 240 ),
        dds.video_stream_profile( 15, dds.video_encoding.uyvy, 424, 240 ),
        dds.video_stream_profile( 5 , dds.video_encoding.uyvy, 424, 240 ),
        ]


def depth_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.z16, 1280, 720 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 1280, 720 ),
        dds.video_stream_profile( 5 , dds.video_encoding.z16, 1280, 720 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 848, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 848, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 848, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 848, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.z16, 848, 480 ),
        dds.video_stream_profile( 100, dds.video_encoding.z16, 848, 100 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 640, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 640, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 640, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 640, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.z16, 640, 480 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 640, 360 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 640, 360 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 640, 360 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 640, 360 ),
        dds.video_stream_profile( 5 , dds.video_encoding.z16, 640, 360 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 480, 270 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 480, 270 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 480, 270 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 480, 270 ),
        dds.video_stream_profile( 5 , dds.video_encoding.z16, 480, 270 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 424, 240 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 424, 240 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 424, 240 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 424, 240 ),
        dds.video_stream_profile( 5 , dds.video_encoding.z16, 424, 240 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 256, 144 )
        ]


def depth_stream():
    stream = dds.depth_stream_server( "Depth", "Stereo Module" )
    stream.init_profiles( depth_stream_profiles(), 5 )
    stream.init_options( stereo_module_options() )
    return stream


def ir_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.y8, 1280, 800 ),
        dds.video_stream_profile( 25, dds.video_encoding.y16, 1280, 800 ),
        dds.video_stream_profile( 15, dds.video_encoding.y16, 1280, 800 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 1280, 800 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 1280, 720 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 1280, 720 ),
        dds.video_stream_profile( 5 , dds.video_encoding.y8, 1280, 720 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 848, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 848, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 848, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 848, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.y8, 848, 480 ),
        dds.video_stream_profile( 100, dds.video_encoding.y8, 848, 100 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 640, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 640, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 640, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 640, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.y8, 640, 480 ),
        dds.video_stream_profile( 25, dds.video_encoding.y16, 640, 400 ),
        dds.video_stream_profile( 15, dds.video_encoding.y16, 640, 400 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 640, 360 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 640, 360 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 640, 360 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 640, 360 ),
        dds.video_stream_profile( 5 , dds.video_encoding.y8, 640, 360 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 480, 270 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 480, 270 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 480, 270 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 480, 270 ),
        dds.video_stream_profile( 5 , dds.video_encoding.y8, 480, 270 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 424, 240 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 424, 240 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 424, 240 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 424, 240 ),
        dds.video_stream_profile( 5 , dds.video_encoding.y8, 424, 240 )
        ]


def ir_stream( number ):
    stream = dds.ir_stream_server( f"Infrared_{number}", "Stereo Module" )
    stream.init_profiles( ir_stream_profiles(), 9 )
    #stream.init_options( stereo_module_options() )
    return stream


def color_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.byr2, 1280, 800 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 1280, 800 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 1280, 800 ),
        dds.video_stream_profile( 10, dds.video_encoding.yuyv, 1280, 800 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 1280, 800 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 1280, 720 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 1280, 720 ),
        dds.video_stream_profile( 10, dds.video_encoding.yuyv, 1280, 720 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 1280, 720 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 848, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 848, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 848, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 848, 480 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 640, 480 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 640, 480 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 640, 480 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 640, 480 ),
        dds.video_stream_profile( 90, dds.video_encoding.yuyv, 640, 360 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 640, 360 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 640, 360 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 640, 360 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 640, 360 ),
        dds.video_stream_profile( 90, dds.video_encoding.yuyv, 480, 270 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 480, 270 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 480, 270 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 480, 270 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 480, 270 ),
        dds.video_stream_profile( 90, dds.video_encoding.yuyv, 424, 240 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 424, 240 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 424, 240 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 424, 240 ),
        dds.video_stream_profile( 5 , dds.video_encoding.yuyv, 424, 240 )
        ]


def color_stream():
    stream = dds.color_stream_server( "Color",  "RGB Camera" )
    stream.init_profiles( color_stream_profiles(), 10 )
    stream.init_options( rgb_camera_options() )
    return stream


def stereo_module_options():
    options = []

    option = dds.option( "Exposure" )
    option.set_value( 33000 )
    option.set_range( dds.option_range( 1, 200000, 1, 33000 ) )
    option.set_description( "Depth Exposure (usec)" )
    options.append( option )
    option = dds.option( "Gain" )
    option.set_value( 16 )
    option.set_range( dds.option_range( 16, 248, 1, 16 ) )
    option.set_description( "UVC image gain" )
    options.append( option )
    option = dds.option( "Enable Auto Exposure" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable Auto Exposure" )
    options.append( option )
    option = dds.option( "Visual Preset" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 5, 1, 0 ) )
    option.set_description( "Advanced-Mode Preset" )
    options.append( option )
    option = dds.option( "Laser Power" )
    option.set_value( 150 )
    option.set_range( dds.option_range( 0, 360, 30, 150 ) )
    option.set_description( "Manual laser power in mw. applicable only when laser power mode is set to Manual" )
    options.append( option )
    option = dds.option( "Emitter Enabled" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 2, 1, 1 ) )
    option.set_description( "Emitter select, 0-disable all emitters, 1-enable laser, 2-enable laser auto (opt), 3-enable LED (opt)" )
    options.append( option )
    option = dds.option( "Error Polling Enabled" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Enable / disable polling of camera internal errors" )
    options.append( option )
    option = dds.option( "Output Trigger Enabled" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Generate trigger from the camera to external device once per frame" )
    options.append( option )
    option = dds.option( "Depth Units" )
    option.set_value( 0.001 )
    option.set_range( dds.option_range( 1e-06, 0.01, 1e-06, 0.001 ) )
    option.set_description( "Number of meters represented by a single depth unit" )
    options.append( option )
    option = dds.option( "Stereo Baseline" )
    option.set_value( 94.9609 )
    option.set_range( dds.option_range( 94.9609, 94.9609, 0, 94.9609 ) )
    option.set_description( "Distance in mm between the stereo imagers" )
    options.append( option )
    option = dds.option( "Inter Cam Sync Mode" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 260, 1, 0 ) )
    option.set_description( "Inter-camera synchronization mode: 0:Default, 1:Master, 2:Slave, 3:Full Salve, 4-258:Genlock with burst count of 1-255 frames for each trigger, 259 and 260 for two frames per trigger with laser ON-OFF and OFF-ON." )
    options.append( option )
    option = dds.option( "Emitter On Off" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Alternating emitter pattern, toggled on/off on per-frame basis" )
    options.append( option )
    option = dds.option( "Global Time Enabled" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )
    option = dds.option( "Emitter Always On" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Emitter always on mode: 0:disabled(default), 1:enabled" )
    options.append( option )
    option = dds.option( "Thermal Compensation" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Toggle thermal compensation adjustments mechanism" )
    options.append( option )
    option = dds.option( "Hdr Enabled" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Sequence Name" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 3, 1, 1 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Sequence Size" )
    option.set_value( 2 )
    option.set_range( dds.option_range( 2, 2, 1, 2 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Sequence Id" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 2, 1, 0 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Auto Exposure Limit" )
    option.set_value( 200000 )
    option.set_range( dds.option_range( 1, 200000, 1, 33000 ) )
    option.set_description( "Exposure limit is in microseconds. If the requested exposure limit is greater than frame time, it will be set to frame time at runtime. Setting will not take effect until next streaming session." )
    options.append( option )
    option = dds.option( "Auto Gain Limit" )
    option.set_value( 248 )
    option.set_range( dds.option_range( 16, 248, 1, 16 ) )
    option.set_description( "Gain limits ranges from 16 to 248. If the requested gain limit is less than 16, it will be set to 16. If the requested gain limit is greater than 248, it will be set to 248. Setting will not take effect until next streaming session." )
    options.append( option )
    option = dds.option( "Auto Exposure Limit Toggle" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Toggle Auto-Exposure Limit" )
    options.append( option )
    option = dds.option( "Auto Gain Limit Toggle" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Toggle Auto-Gain Limit" )
    options.append( option )

    return options


def rgb_camera_options():
    options = []

    option = dds.option( "Backlight Compensation" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Enable / disable backlight compensation" )
    options.append( option )
    option = dds.option( "Brightness" )
    option.set_value( 0 )
    option.set_range( dds.option_range( -64, 64, 1, 0 ) )
    option.set_description( "UVC image brightness" )
    options.append( option )
    option = dds.option( "Contrast" )
    option.set_value( 50 )
    option.set_range( dds.option_range( 0, 100, 1, 50 ) )
    option.set_description( "UVC image contrast" )
    options.append( option )
    option = dds.option( "Exposure" )
    option.set_value( 156 )
    option.set_range( dds.option_range( 1, 10000, 1, 156 ) )
    option.set_description( "Controls exposure time of color camera. Setting any value will disable auto exposure" )
    options.append( option )
    option = dds.option( "Gain" )
    option.set_value( 64 )
    option.set_range( dds.option_range( 0, 128, 1, 64 ) )
    option.set_description( "UVC image gain" )
    options.append( option )
    option = dds.option( "Gamma" )
    option.set_value( 300 )
    option.set_range( dds.option_range( 100, 500, 1, 300 ) )
    option.set_description( "UVC image gamma setting" )
    options.append( option )
    option = dds.option( "Hue" )
    option.set_value( 0 )
    option.set_range( dds.option_range( -180, 180, 1, 0 ) )
    option.set_description( "UVC image hue" )
    options.append( option )
    option = dds.option( "Saturation" )
    option.set_value( 64 )
    option.set_range( dds.option_range( 0, 100, 1, 64 ) )
    option.set_description( "UVC image saturation setting" )
    options.append( option )
    option = dds.option( "Sharpness" )
    option.set_value( 50 )
    option.set_range( dds.option_range( 0, 100, 1, 50 ) )
    option.set_description( "UVC image sharpness setting" )
    options.append( option )
    option = dds.option( "White Balance" )
    option.set_value( 4600 )
    option.set_range( dds.option_range( 2800, 6500, 10, 4600 ) )
    option.set_description( "Controls white balance of color image. Setting any value will disable auto white balance" )
    options.append( option )
    option = dds.option( "Enable Auto Exposure" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable / disable auto-exposure" )
    options.append( option )
    option = dds.option( "Enable Auto White Balance" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable / disable auto-white-balance" )
    options.append( option )
    option = dds.option( "Power Line Frequency" )
    option.set_value( 3 )
    option.set_range( dds.option_range( 0, 3, 1, 3 ) )
    option.set_description( "Power Line Frequency" )
    options.append( option )
    option = dds.option( "Auto Exposure Priority" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Restrict Auto-Exposure to enforce constant FPS rate. Turn ON to remove the restrictions (may result in FPS drop)" )
    options.append( option )
    option = dds.option( "Global Time Enabled" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )

    return options


def motion_module_options():
    options = []

    option = dds.option( "Enable Motion Correction" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable Automatic Motion Data Correction" )
    options.append( option )
    option = dds.option( "Global Time Enabled" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )

    return options


def get_extrinsics():
    extrinsics = {}
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,-0.0011767612304538488,0.0007593693444505334,0.0011775976745411754,0.9999986886978149,-0.0011019925586879253,-0.0007580715464428067,0.0011028856970369816,0.9999991059303284)
    extr.translation = (0.05909481644630432,9.327199222752824e-05,-0.00016530796710867435)
    extrinsics[("Color","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,-0.0011767612304538488,0.0007593693444505334,0.0011775976745411754,0.9999986886978149,-0.0011019925586879253,-0.0007580715464428067,0.0011028856970369816,0.9999991059303284)
    extr.translation = (0.028874816372990608,0.007493271958082914,0.015854692086577415)
    extrinsics[("Color","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,-0.0011767612304538488,0.0007593693444505334,0.0011775976745411754,0.9999986886978149,-0.0011019925586879253,-0.0007580715464428067,0.0011028856970369816,0.9999991059303284)
    extr.translation = (0.05909481644630432,9.327199222752824e-05,-0.00016530796710867435)
    extrinsics[("Color","Infrared_1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,-0.0011767612304538488,0.0007593693444505334,0.0011775976745411754,0.9999986886978149,-0.0011019925586879253,-0.0007580715464428067,0.0011028856970369816,0.9999991059303284)
    extr.translation = (-0.035866111516952515,9.327199222752824e-05,-0.00016530796710867435)
    extrinsics[("Color","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,0.0011775976745411754,-0.0007580715464428067,-0.0011767612304538488,0.9999986886978149,0.0011028856970369816,0.0007593693444505334,-0.0011019925586879253,0.9999991059303284)
    extr.translation = (-0.05909452587366104,-0.0001630439655855298,0.00021000305423513055)
    extrinsics[("Depth","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.030220000073313713,0.007400000002235174,0.016019999980926514)
    extrinsics[("Depth","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Depth","Infrared_1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.09496092796325684,0.0,0.0)
    extrinsics[("Depth","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,0.0011775976745411754,-0.0007580715464428067,-0.0011767612304538488,0.9999986886978149,0.0011028856970369816,0.0007593693444505334,-0.0011019925586879253,0.9999991059303284)
    extr.translation = (-0.02887801080942154,-0.007509792689234018,-0.015841051936149597)
    extrinsics[("Motion","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.030220000073313713,-0.007400000002235174,-0.016019999980926514)
    extrinsics[("Motion","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.030220000073313713,-0.007400000002235174,-0.016019999980926514)
    extrinsics[("Motion","Infrared_1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.06474092602729797,-0.007400000002235174,-0.016019999980926514)
    extrinsics[("Motion","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,0.0011775976745411754,-0.0007580715464428067,-0.0011767612304538488,0.9999986886978149,0.0011028856970369816,0.0007593693444505334,-0.0011019925586879253,0.9999991059303284)
    extr.translation = (-0.05909452587366104,-0.0001630439655855298,0.00021000305423513055)
    extrinsics[("Infrared_1","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared_1","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.030220000073313713,0.007400000002235174,0.016019999980926514)
    extrinsics[("Infrared_1","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.09496092796325684,0.0,0.0)
    extrinsics[("Infrared_1","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999990463256836,0.0011775976745411754,-0.0007580715464428067,-0.0011767612304538488,0.9999986886978149,0.0011028856970369816,0.0007593693444505334,-0.0011019925586879253,0.9999991059303284)
    extr.translation = (0.03586631268262863,-5.1218194130342454e-05,0.00013801587920170277)
    extrinsics[("Infrared_2","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.09496092796325684,0.0,0.0)
    extrinsics[("Infrared_2","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.06474092602729797,0.007400000002235174,0.016019999980926514)
    extrinsics[("Infrared_2","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.09496092796325684,0.0,0.0)
    extrinsics[("Infrared_2","Infrared_1")] = extr

    return extrinsics

