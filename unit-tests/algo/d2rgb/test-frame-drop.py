# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

import pyrealsense2 as rs
from rspy import test, ac

test.set_env_vars({"RS2_AC_DISABLE_CONDITIONS":"1",
                   "RS2_AC_DISABLE_RETRIES":"1",
                   "RS2_AC_FORCE_BAD_RESULT":"1",
                   "RS2_AC_LOG_TO_STDOUT":"1"
                   })

# rs.log_to_file( rs.log_severity.debug, "rs.log" )

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

# Resetting sensors to factory calibration
dcs = rs.calibrated_sensor(depth_sensor)

ccs = rs.calibrated_sensor(color_sensor)

d2r = rs.device_calibration(dev)
d2r.register_calibration_change_callback( ac.status_list_callback )

cp = next(p for p in color_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.color
                and p.format() == rs.format.yuyv
                and p.as_video_stream_profile().width() == 1280
                and p.as_video_stream_profile().height() == 720)

dp = next(p for p in
                depth_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.depth
                and p.format() == rs.format.z16
                and p.as_video_stream_profile().width() == 1024
                and p.as_video_stream_profile().height() == 768)

# This variable controls how many calibrations we do while testing for frame drops
n_cal = 5
# Variables for saving the previous color frame number and previous depth frame number
previous_color_frame_number = -1
previous_depth_frame_number = -1

# Functions that assert that each frame we receive has the frame number following the previous frame number
def color_frame_call_back(frame):
    global previous_color_frame_number
    test.check_frame_drops(frame, previous_color_frame_number)
    previous_color_frame_number = frame.get_frame_number()

def depth_frame_call_back(frame):
    global previous_depth_frame_number
    test.check_frame_drops(frame, previous_depth_frame_number)
    previous_depth_frame_number = frame.get_frame_number()

depth_sensor.open( dp )
depth_sensor.start( depth_frame_call_back )
color_sensor.open( cp )
color_sensor.start( color_frame_call_back )

#############################################################################################
# Test #1
test.start("Checking for frame drops in", n_cal, "calibrations")
for i in range(n_cal):
    try:
        dcs.reset_calibration()
        ccs.reset_calibration()
        d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
        ac.wait_for_calibration()
    except:
        test.unexpected_exception()
test.finish()

#############################################################################################
# Test #2
test.start("Checking for frame drops in a failed calibration")
ac.reset_status_list()
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    try:
        d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
        ac.wait_for_calibration()
    except Exception as e: # Second trigger should throw exception
        test.check_exception(e, RuntimeError, "Camera Accuracy Health is already active")
    else:
        test.unexpected_exception()
    ac.wait_for_calibration() # First trigger should continue and finish successfully
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

depth_sensor.stop()
depth_sensor.close()
color_sensor.stop()
color_sensor.close()

test.print_results_and_exit()
