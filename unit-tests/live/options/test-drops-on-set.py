# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device D400* !D457

import platform
import pyrealsense2 as rs
from rspy import test
from rspy import log
import time

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()
product_line = dev.get_info(rs.camera_info.product_line)

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
    time.sleep(0.5)  # collect frames
    after_set_option = False


def check_depth_frame_drops(frame):
    global previous_depth_frame_number
    allowed_drops = get_allowed_drops()
    is_d400 = 0
    if product_line == "D400":
        is_d400 = 1
    test.check_frame_drops(frame, previous_depth_frame_number, allowed_drops, is_d400)
    previous_depth_frame_number = frame.get_frame_number()


def check_color_frame_drops(frame):
    global previous_color_frame_number
    allowed_drops = get_allowed_drops()
    test.check_frame_drops(frame, previous_color_frame_number, allowed_drops)
    previous_color_frame_number = frame.get_frame_number()


# Use a profile that's common to all cameras
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

depth_sensor.open(depth_profile)
depth_sensor.start(check_depth_frame_drops)
color_sensor.open(color_profile)
color_sensor.start(check_color_frame_drops)

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
if depth_sensor.supports(rs.option.visual_preset):
    if product_line == "D400":
        depth_sensor.set_option(rs.option.visual_preset, int(rs.rs400_visual_preset.default))


#############################################################################################

options_to_ignore = []

# ignore reasons:
# visual_preset       --> frame drops are expected during visual_preset change
# inter_cam_sync_mode --> frame drops are expected during inter_cam_sync_mode change
# emitter_frequency   --> Not allowed to be set during streaming
# auto_exposure_mode  --> Not allowed to be set during streaming
if product_line == "D400":
    options_to_ignore = [rs.option.visual_preset, rs.option.inter_cam_sync_mode, rs.option.emitter_frequency, rs.option.auto_exposure_mode]

def test_option_changes(sensor):
    global options_to_ignore
    options = sensor.get_supported_options()
    for option in options:
        try:
            if option in options_to_ignore:
                continue
            if sensor.is_option_read_only(option):
                continue
            old_value = sensor.get_option(option)
            range = sensor.get_option_range(option)
            new_value = range.min
            if old_value == new_value:
                new_value = range.max
            if not log.d(str(option), old_value, '->', new_value):
                test.info(str(option), new_value, persistent=True)
            set_new_value(sensor, option, new_value)
            sensor.set_option(option, old_value)
        except:
            test.unexpected_exception()
            break
        finally:
            test.reset_info(persistent=True)


#############################################################################################
time.sleep(0.5)  # jic
test.start("Checking frame drops when setting options on depth")
test_option_changes(depth_sensor)
test.finish()

#############################################################################################
time.sleep(0.5)  # jic
test.start("Checking frame drops when setting options on color")
test_option_changes( color_sensor )
test.finish()


#############################################################################################
depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

test.print_results_and_exit()
