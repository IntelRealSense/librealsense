# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log, test


device_info = dds.message.device_info()
device_info.name = "Intel RealSense D435I"
device_info.serial = "036522070660"
device_info.product_line = "D400"
device_info.topic_root = "realsense/D435I_" + device_info.serial


def build( participant ):
    """
    Build a D435i device server for use in DDS
    """
    d435i = dds.device_server( participant, device_info.topic_root )
    d435i.init( build_streams(), build_options(), get_extrinsics() )
    return d435i


def build_streams():
    """
    Build the streams for a D435i device server
    """
    motion = motion_stream()
    depth = depth_stream()
    ir1 = ir_stream( 1 )
    ir2 = ir_stream( 2 )
    color = color_stream()
    return [color, depth, motion, ir1, ir2]


def build_options():
    return []


def gyro_stream_profiles():
    return [
        dds.motion_stream_profile( 200 ),
        dds.motion_stream_profile( 400 )
        ]


def motion_stream():
    stream = dds.motion_stream_server( "Motion", "Motion Module" )
    stream.init_profiles( gyro_stream_profiles(), 0 )
    stream.init_options( motion_module_options() )

    intr = dds.motion_intrinsics()
    intr.data = [[1.0,0.0,0.0,0.0],[0.0,1.0,0.0,0.0],[0.0,0.0,1.0,0.0]]
    intr.noise_variances = [0.0,0.0,0.0]
    intr.bias_variances = [0.0,0.0,0.0]
    stream.set_gyro_intrinsics( intr )

    intr = dds.motion_intrinsics()
    intr.data = [[1.0,0.0,0.0,0.0],[0.0,1.0,0.0,0.0],[0.0,0.0,1.0,0.0]]
    intr.noise_variances = [0.0,0.0,0.0]
    intr.bias_variances = [0.0,0.0,0.0]
    stream.set_accel_intrinsics( intr )

    return stream


def depth_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.z16, 1280,720 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 1280,720 ),
        dds.video_stream_profile( 6, dds.video_encoding.z16, 1280,720 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 848,480 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 848,480 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 848,480 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 848,480 ),
        dds.video_stream_profile( 6, dds.video_encoding.z16, 848,480 ),
        dds.video_stream_profile( 300, dds.video_encoding.z16, 848,100 ),
        dds.video_stream_profile( 100, dds.video_encoding.z16, 848,100 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 640,480 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 640,480 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 640,480 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 640,480 ),
        dds.video_stream_profile( 6, dds.video_encoding.z16, 640,480 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 640,360 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 640,360 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 640,360 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 640,360 ),
        dds.video_stream_profile( 6, dds.video_encoding.z16, 640,360 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 480,270 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 480,270 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 480,270 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 480,270 ),
        dds.video_stream_profile( 6, dds.video_encoding.z16, 480,270 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 424,240 ),
        dds.video_stream_profile( 60, dds.video_encoding.z16, 424,240 ),
        dds.video_stream_profile( 30, dds.video_encoding.z16, 424,240 ),
        dds.video_stream_profile( 15, dds.video_encoding.z16, 424,240 ),
        dds.video_stream_profile( 6, dds.video_encoding.z16, 424,240 ),
        dds.video_stream_profile( 300, dds.video_encoding.z16, 256,144 ),
        dds.video_stream_profile( 90, dds.video_encoding.z16, 256,144 )
        ]


def depth_stream():
    stream = dds.depth_stream_server( "Depth", "Stereo Module" )
    stream.init_profiles( depth_stream_profiles(), 5 )
    stream.init_options( stereo_module_options() )
    stream.set_intrinsics( depth_stream_intrinsics() )
    return stream


def ir_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.y8, 1280,800 ),
        dds.video_stream_profile( 25, dds.video_encoding.y16, 1280,800 ),
        dds.video_stream_profile( 15, dds.video_encoding.y16, 1280,800 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 1280,800 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 1280,720 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 1280,720 ),
        dds.video_stream_profile( 6, dds.video_encoding.y8, 1280,720 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 848,480 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 848,480 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 848,480 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 848,480 ),
        dds.video_stream_profile( 6, dds.video_encoding.y8, 848,480 ),
        dds.video_stream_profile( 300, dds.video_encoding.y8, 848,100 ),
        dds.video_stream_profile( 100, dds.video_encoding.y8, 848,100 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 640,480 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 640,480 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 640,480 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 640,480 ),
        dds.video_stream_profile( 6, dds.video_encoding.y8, 640,480 ),
        dds.video_stream_profile( 25, dds.video_encoding.y16, 640,400 ),
        dds.video_stream_profile( 15, dds.video_encoding.y16, 640,400 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 640,360 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 640,360 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 640,360 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 640,360 ),
        dds.video_stream_profile( 6, dds.video_encoding.y8, 640,360 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 480,270 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 480,270 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 480,270 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 480,270 ),
        dds.video_stream_profile( 6, dds.video_encoding.y8, 480,270 ),
        dds.video_stream_profile( 90, dds.video_encoding.y8, 424,240 ),
        dds.video_stream_profile( 60, dds.video_encoding.y8, 424,240 ),
        dds.video_stream_profile( 30, dds.video_encoding.y8, 424,240 ),
        dds.video_stream_profile( 15, dds.video_encoding.y8, 424,240 ),
        dds.video_stream_profile( 6, dds.video_encoding.y8, 424,240 )
        ]


def ir_stream( number ):
    stream = dds.ir_stream_server( "Infrared_" + str(number), "Stereo Module" )
    stream.init_profiles( ir_stream_profiles(), 9 )
    stream.init_options( stereo_module_options() )
    stream.set_intrinsics( ir_stream_intrinsics() )
    return stream


def color_stream_profiles():
    return [
        dds.video_stream_profile( 30, dds.video_encoding.byr2, 1920,1080 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 1920,1080 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 1920,1080 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 1920,1080 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 1280,720 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 1280,720 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 1280,720 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 960,540 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 960,540 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 960,540 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 960,540 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 848,480 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 848,480 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 848,480 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 848,480 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 640,480 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 640,480 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 640,480 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 640,480 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 640,360 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 640,360 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 640,360 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 640,360 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 424,240 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 424,240 ),
        dds.video_stream_profile( 15, dds.video_encoding.yuyv, 424,240 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 424,240 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 320,240 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 320,240 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 320,240 ),
        dds.video_stream_profile( 60, dds.video_encoding.yuyv, 320,180 ),
        dds.video_stream_profile( 30, dds.video_encoding.yuyv, 320,180 ),
        dds.video_stream_profile( 6, dds.video_encoding.yuyv, 320,180 )
        ]


def color_stream():
    stream = dds.color_stream_server( "Color",  "RGB Camera" )
    stream.init_profiles( color_stream_profiles(), 8 )
    stream.init_options( rgb_camera_options() )
    stream.set_intrinsics( color_stream_intrinsics() )
    return stream


def stereo_module_options():
    options = []

    option = dds.option( "Exposure", "Stereo Module" )
    option.set_value( 8500 )
    option.set_range( dds.option_range( 1, 200000, 1, 8500 ) )
    option.set_description( "Depth Exposure (usec)" )
    options.append( option )
    option = dds.option( "Gain", "Stereo Module" )
    option.set_value( 16 )
    option.set_range( dds.option_range( 16, 248, 1, 16 ) )
    option.set_description( "UVC image gain" )
    options.append( option )
    option = dds.option( "Enable Auto Exposure", "Stereo Module" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable Auto Exposure" )
    options.append( option )
    option = dds.option( "Visual Preset", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 5, 1, 0 ) )
    option.set_description( "Advanced-Mode Preset" )
    options.append( option )
    option = dds.option( "Laser Power", "Stereo Module" )
    option.set_value( 150 )
    option.set_range( dds.option_range( 0, 360, 30, 150 ) )
    option.set_description( "Manual laser power in mw. applicable only when laser power mode is set to Manual" )
    options.append( option )
    option = dds.option( "Emitter Enabled", "Stereo Module" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 2, 1, 1 ) )
    option.set_description( "Emitter select, 0-disable all emitters, 1-enable laser, 2-enable laser auto (opt), 3-enable LED (opt)" )
    options.append( option )
    option = dds.option( "Frames Queue Size", "Stereo Module" )
    option.set_value( 16 )
    option.set_range( dds.option_range( 0, 32, 1, 16 ) )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.option( "Error Polling Enabled", "Stereo Module" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Enable / disable polling of camera internal errors" )
    options.append( option )
    option = dds.option( "Output Trigger Enabled", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Generate trigger from the camera to external device once per frame" )
    options.append( option )
    option = dds.option( "Depth Units", "Stereo Module" )
    option.set_value( 0.001 )
    option.set_range( dds.option_range( 1e-06, 0.01, 1e-06, 0.001 ) )
    option.set_description( "Number of meters represented by a single depth unit" )
    options.append( option )
    option = dds.option( "Stereo Baseline", "Stereo Module" )
    option.set_value( 49.864 )
    option.set_range( dds.option_range( 49.864, 49.864, 0, 49.864 ) )
    option.set_description( "Distance in mm between the stereo imagers" )
    options.append( option )
    option = dds.option( "Inter Cam Sync Mode", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 260, 1, 0 ) )
    option.set_description( "Inter-camera synchronization mode: 0:Default, 1:Master, 2:Slave, 3:Full Salve, 4-258:Genlock with burst count of 1-255 frames for each trigger, 259 and 260 for two frames per trigger with laser ON-OFF and OFF-ON." )
    options.append( option )
    option = dds.option( "Emitter On Off", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Alternating emitter pattern, toggled on/off on per-frame basis" )
    options.append( option )
    option = dds.option( "Global Time Enabled", "Stereo Module" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )
    option = dds.option( "Emitter Always On", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Emitter always on mode: 0:disabled(default), 1:enabled" )
    options.append( option )
    option = dds.option( "Hdr Enabled", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Sequence Name", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 3, 1, 1 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Sequence Size", "Stereo Module" )
    option.set_value( 2 )
    option.set_range( dds.option_range( 2, 2, 1, 2 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Sequence Id", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 2, 1, 0 ) )
    option.set_description( "HDR Option" )
    options.append( option )
    option = dds.option( "Auto Exposure Limit", "Stereo Module" )
    option.set_value( 200000 )
    option.set_range( dds.option_range( 1, 200000, 1, 8500 ) )
    option.set_description( "Exposure limit is in microseconds. If the requested exposure limit is greater than frame time, it will be set to frame time at runtime. Setting will not take effect until next streaming session." )
    options.append( option )
    option = dds.option( "Auto Gain Limit", "Stereo Module" )
    option.set_value( 248 )
    option.set_range( dds.option_range( 16, 248, 1, 16 ) )
    option.set_description( "Gain limits ranges from 16 to 248. If the requested gain limit is less than 16, it will be set to 16. If the requested gain limit is greater than 248, it will be set to 248. Setting will not take effect until next streaming session." )
    options.append( option )
    option = dds.option( "Auto Exposure Limit Toggle", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Toggle Auto-Exposure Limit" )
    options.append( option )
    option = dds.option( "Auto Gain Limit Toggle", "Stereo Module" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Toggle Auto-Gain Limit" )
    options.append( option )

    return options


def rgb_camera_options():
    options = []

    option = dds.option( "Backlight Compensation", "RGB Camera" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Enable / disable backlight compensation" )
    options.append( option )
    option = dds.option( "Brightness", "RGB Camera" )
    option.set_value( 0 )
    option.set_range( dds.option_range( -64, 64, 1, 0 ) )
    option.set_description( "UVC image brightness" )
    options.append( option )
    option = dds.option( "Contrast", "RGB Camera" )
    option.set_value( 50 )
    option.set_range( dds.option_range( 0, 100, 1, 50 ) )
    option.set_description( "UVC image contrast" )
    options.append( option )
    option = dds.option( "Exposure", "RGB Camera" )
    option.set_value( 156 )
    option.set_range( dds.option_range( 1, 10000, 1, 156 ) )
    option.set_description( "Controls exposure time of color camera. Setting any value will disable auto exposure" )
    options.append( option )
    option = dds.option( "Gain", "RGB Camera" )
    option.set_value( 6 )
    option.set_range( dds.option_range( 0, 128, 1, 64 ) )
    option.set_description( "UVC image gain" )
    options.append( option )
    option = dds.option( "Gamma", "RGB Camera" )
    option.set_value( 300 )
    option.set_range( dds.option_range( 100, 500, 1, 300 ) )
    option.set_description( "UVC image gamma setting" )
    options.append( option )
    option = dds.option( "Hue", "RGB Camera" )
    option.set_value( 0 )
    option.set_range( dds.option_range( -180, 180, 1, 0 ) )
    option.set_description( "UVC image hue" )
    options.append( option )
    option = dds.option( "Saturation", "RGB Camera" )
    option.set_value( 64 )
    option.set_range( dds.option_range( 0, 100, 1, 64 ) )
    option.set_description( "UVC image saturation setting" )
    options.append( option )
    option = dds.option( "Sharpness", "RGB Camera" )
    option.set_value( 50 )
    option.set_range( dds.option_range( 0, 100, 1, 50 ) )
    option.set_description( "UVC image sharpness setting" )
    options.append( option )
    option = dds.option( "White Balance", "RGB Camera" )
    option.set_value( 4600 )
    option.set_range( dds.option_range( 2800, 6500, 10, 4600 ) )
    option.set_description( "Controls white balance of color image. Setting any value will disable auto white balance" )
    options.append( option )
    option = dds.option( "Enable Auto Exposure", "RGB Camera" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable / disable auto-exposure" )
    options.append( option )
    option = dds.option( "Enable Auto White Balance", "RGB Camera" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable / disable auto-white-balance" )
    options.append( option )
    option = dds.option( "Frames Queue Size", "RGB Camera" )
    option.set_value( 16 )
    option.set_range( dds.option_range( 0, 32, 1, 16 ) )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.option( "Power Line Frequency", "RGB Camera" )
    option.set_value( 3 )
    option.set_range( dds.option_range( 0, 3, 1, 3 ) )
    option.set_description( "Power Line Frequency" )
    options.append( option )
    option = dds.option( "Auto Exposure Priority", "RGB Camera" )
    option.set_value( 0 )
    option.set_range( dds.option_range( 0, 1, 1, 0 ) )
    option.set_description( "Restrict Auto-Exposure to enforce constant FPS rate. Turn ON to remove the restrictions (may result in FPS drop)" )
    options.append( option )
    option = dds.option( "Global Time Enabled", "RGB Camera" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )

    return options


def motion_module_options():
    options = []

    option = dds.option( "Frames Queue Size", "Motion Module" )
    option.set_value( 16 )
    option.set_range( dds.option_range( 0, 32, 1, 16 ) )
    option.set_description( "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase latency, and vice versa" )
    options.append( option )
    option = dds.option( "Enable Motion Correction", "Motion Module" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable Automatic Motion Data Correction" )
    options.append( option )
    option = dds.option( "Global Time Enabled", "Motion Module" )
    option.set_value( 1 )
    option.set_range( dds.option_range( 0, 1, 1, 1 ) )
    option.set_description( "Enable/Disable global timestamp" )
    options.append( option )

    return options


def color_stream_intrinsics():
    intrinsics = []

    intr = dds.video_intrinsics();
    intr.width = 320
    intr.height = 180
    intr.principal_point_x = 161.7417755126953
    intr.principal_point_y = 90.47455596923828
    intr.focal_lenght_x = 227.0221710205078
    intr.focal_lenght_y = 227.1049346923828
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 320
    intr.height = 240
    intr.principal_point_x = 162.32237243652344
    intr.principal_point_y = 120.63274383544922
    intr.focal_lenght_x = 302.69622802734375
    intr.focal_lenght_y = 302.80657958984375
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 424
    intr.height = 240
    intr.principal_point_x = 214.32235717773438
    intr.principal_point_y = 120.63274383544922
    intr.focal_lenght_x = 302.69622802734375
    intr.focal_lenght_y = 302.80657958984375
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 640
    intr.height = 360
    intr.principal_point_x = 323.4835510253906
    intr.principal_point_y = 180.94911193847656
    intr.focal_lenght_x = 454.0443420410156
    intr.focal_lenght_y = 454.2098693847656
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 640
    intr.height = 480
    intr.principal_point_x = 324.6447448730469
    intr.principal_point_y = 241.26548767089844
    intr.focal_lenght_x = 605.3924560546875
    intr.focal_lenght_y = 605.6131591796875
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 848
    intr.height = 480
    intr.principal_point_x = 428.64471435546875
    intr.principal_point_y = 241.26548767089844
    intr.focal_lenght_x = 605.3924560546875
    intr.focal_lenght_y = 605.6131591796875
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 960
    intr.height = 540
    intr.principal_point_x = 485.2253112792969
    intr.principal_point_y = 271.4236755371094
    intr.focal_lenght_x = 681.0665283203125
    intr.focal_lenght_y = 681.3148193359375
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 1280
    intr.height = 720
    intr.principal_point_x = 646.9671020507813
    intr.principal_point_y = 361.8982238769531
    intr.focal_lenght_x = 908.0886840820313
    intr.focal_lenght_y = 908.4197387695313
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 1920
    intr.height = 1080
    intr.principal_point_x = 970.4506225585938
    intr.principal_point_y = 542.8473510742188
    intr.focal_lenght_x = 1362.133056640625
    intr.focal_lenght_y = 1362.629638671875
    intr.distortion_model = 2
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    return set( intrinsics )


def depth_ir_common_intrinsics():
    intrinsics = []

    intr = dds.video_intrinsics();
    intr.width = 424
    intr.height = 240
    intr.principal_point_x = 212.0788116455078
    intr.principal_point_y = 119.07991790771484
    intr.focal_lenght_x = 209.13233947753906
    intr.focal_lenght_y = 209.13233947753906
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 480
    intr.height = 270
    intr.principal_point_x = 240.08921813964844
    intr.principal_point_y = 134.00367736816406
    intr.focal_lenght_x = 236.7535858154297
    intr.focal_lenght_y = 236.7535858154297
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 640
    intr.height = 360
    intr.principal_point_x = 320.11895751953125
    intr.principal_point_y = 178.67156982421875
    intr.focal_lenght_x = 315.67144775390625
    intr.focal_lenght_y = 315.67144775390625
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 640
    intr.height = 480
    intr.principal_point_x = 320.14276123046875
    intr.principal_point_y = 238.4058837890625
    intr.focal_lenght_x = 378.80572509765625
    intr.focal_lenght_y = 378.80572509765625
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 848
    intr.height = 100
    intr.principal_point_x = 424.1576232910156
    intr.principal_point_y = 48.239837646484375
    intr.focal_lenght_x = 418.2646789550781
    intr.focal_lenght_y = 418.2646789550781
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 848
    intr.height = 480
    intr.principal_point_x = 424.1576232910156
    intr.principal_point_y = 238.23983764648438
    intr.focal_lenght_x = 418.2646789550781
    intr.focal_lenght_y = 418.2646789550781
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intr = dds.video_intrinsics();
    intr.width = 1280
    intr.height = 720
    intr.principal_point_x = 640.2379150390625
    intr.principal_point_y = 357.3431396484375
    intr.focal_lenght_x = 631.3428955078125
    intr.focal_lenght_y = 631.3428955078125
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    return intrinsics


def depth_stream_intrinsics():
    intrinsics = []

    intr = dds.video_intrinsics();
    intr.width = 256
    intr.height = 144
    intr.principal_point_x = 128.2379150390625
    intr.principal_point_y = 69.3431396484375
    intr.focal_lenght_x = 631.3428955078125
    intr.focal_lenght_y = 631.3428955078125
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    intrinsics.extend( depth_ir_common_intrinsics() )

    return set( intrinsics )


def ir_stream_intrinsics():
    intrinsics = depth_ir_common_intrinsics()

    intr = dds.video_intrinsics();
    intr.width = 1280
    intr.height = 800
    intr.principal_point_x = 640.2379150390625
    intr.principal_point_y = 397.3431396484375
    intr.focal_lenght_x = 631.3428955078125
    intr.focal_lenght_y = 631.3428955078125
    intr.distortion_model = 4
    intr.distortion_coeffs = [0.0,0.0,0.0,0.0,0.0]
    intrinsics.append( intr )

    return set( intrinsics )


def get_extrinsics():
    extrinsics = {}

    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.015078110620379448,-1.0675736120902002e-05,-0.00021772991749458015)
    extrinsics[("Color","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.02059810981154442,0.0050893244333565235,0.011522269807755947)
    extrinsics[("Color","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.015078110620379448,-1.0675736120902002e-05,-0.00021772991749458015)
    extrinsics[("Color","Infrared_1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,0.00040659401565790176,0.0030847808811813593,-0.0004076171899214387,0.9999998807907104,0.0003310548490844667,-0.00308464583940804,-0.0003323106502648443,0.9999951720237732)
    extr.translation = (-0.06494206935167313,-1.0675736120902002e-05,-0.00021772991749458015)
    extrinsics[("Color","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.015078714117407799,4.601718956109835e-06,0.00017121469136327505)
    extrinsics[("Depth","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.005520000122487545,0.005100000184029341,0.011739999987185001)
    extrinsics[("Depth","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Depth","Infrared_1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04986396059393883,0.0,0.0)
    extrinsics[("Depth","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.02056039869785309,-0.00510153453797102,-0.011584061197936535)
    extrinsics[("Motion","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.005520000122487545,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Motion","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.005520000122487545,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Motion","Infrared_1")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04434395954012871,-0.005100000184029341,-0.011739999987185001)
    extrinsics[("Motion","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.015078714117407799,4.601718956109835e-06,0.00017121469136327505)
    extrinsics[("Infrared_1","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.0,0.0,0.0)
    extrinsics[("Infrared_1","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.005520000122487545,0.005100000184029341,0.011739999987185001)
    extrinsics[("Infrared_1","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (-0.04986396059393883,0.0,0.0)
    extrinsics[("Infrared_1","Infrared_2")] = extr
    extr = dds.extrinsics();
    extr.rotation = (0.9999951720237732,-0.0004076171899214387,-0.00308464583940804,0.00040659401565790176,0.9999998807907104,-0.0003323106502648443,0.0030847808811813593,0.0003310548490844667,0.9999951720237732)
    extr.translation = (0.06494243443012238,-1.5723688193247654e-05,1.7402038793079555e-05)
    extrinsics[("Infrared_2","Color")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04986396059393883,0.0,0.0)
    extrinsics[("Infrared_2","Depth")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04434395954012871,0.005100000184029341,0.011739999987185001)
    extrinsics[("Infrared_2","Motion")] = extr
    extr = dds.extrinsics();
    extr.rotation = (1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0)
    extr.translation = (0.04986396059393883,0.0,0.0)
    extrinsics[("Infrared_2","Infrared_1")] = extr

    return extrinsics

