# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import platform
import pyrealsense2 as rs
from rspy import test
from rspy import log
import time

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

previous_depth_frame_number = -1
previous_color_frame_number = -1
after_set_option = False


def get_allowed_drops(): 
    global after_set_option
    # On Linux, there is a known issue (RS5-7148) where up to 4 frame drops can occur
    # sequentially after setting control values during streaming... on Windows this
    # does not occur.
    if platform.system() == 'Linux' and after_set_option:
        return 4
    # Our KPI is to prevent sequential frame drops, therefore single frame drop is allowed.
    return 1

def set_new_value(sensor, option, value): 
    global after_set_option
    after_set_option = True
    sensor.set_option(option, value)
    time.sleep( 0.5 )  # collect frames
    after_set_option = False

def check_depth_frame_drops(frame):
    global previous_depth_frame_number
    allowed_drops = get_allowed_drops()
    test.check_frame_drops(frame, previous_depth_frame_number, allowed_drops)
    previous_depth_frame_number = frame.get_frame_number()

def check_color_frame_drops(frame):
    global previous_color_frame_number
    allowed_drops = get_allowed_drops()
    test.check_frame_drops(frame, previous_color_frame_number, allowed_drops)
    previous_color_frame_number = frame.get_frame_number()

# Use a profile that's common to both L500 and D400
depth_profile = next(p for p in
                depth_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.depth
                and p.format() == rs.format.z16
                and p.as_video_stream_profile().width() == 640
                and p.as_video_stream_profile().height() == 480)

color_profile = next(p for p in color_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.color
                and p.format() == rs.format.yuyv
                and p.as_video_stream_profile().width() == 640
                and p.as_video_stream_profile().height() == 480)

depth_sensor.open( depth_profile )
depth_sensor.start( check_depth_frame_drops )
color_sensor.open( color_profile )
color_sensor.start( check_color_frame_drops )


#############################################################################################
# Test #1

laser_power = rs.option.laser_power
current_laser_control = 10

test.start("Checking for frame drops when setting laser power several times")

for i in range(1,5): 
    new_value = current_laser_control + 10*i
    set_new_value(depth_sensor, laser_power, new_value)

test.finish()

# reset everything back
depth_sensor.set_option( rs.option.visual_preset, int(rs.l500_visual_preset.max_range) )



#############################################################################################
# Test #2

time.sleep(0.5)  # jic

depth_options = depth_sensor.get_supported_options()
color_options = color_sensor.get_supported_options()

test.start("Checking for frame drops when setting any option")

for option in depth_options:
    try:
        if depth_sensor.is_option_read_only(option): 
            continue
        old_value = depth_sensor.get_option( option )
        range = depth_sensor.get_option_range( option )
        new_value = range.min
        if old_value == new_value:
            new_value = range.max
        if not log.d( str(option), old_value, '->', new_value ):
            test.info( str(option), new_value, persistent = True )
        set_new_value( depth_sensor, option, new_value )
        depth_sensor.set_option( option, old_value )
    except: 
        test.unexpected_exception()
        test.abort()
    finally:
        test.reset_info( persistent = True )

for option in color_options:
    try:
        if color_sensor.is_option_read_only(option): 
            continue
        new_value = color_sensor.get_option_range(option).min
        set_new_value(color_sensor, option, new_value)
    except: 
        option_name = "Color sensor - " + str(option)
        test.info(option_name, new_value)
        test.unexpected_exception()
        test.abort()

test.finish()


#############################################################################################
depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

test.print_results_and_exit()
