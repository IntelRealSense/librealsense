# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

from rspy import test
test.set_env_vars({"RS2_AC_DISABLE_CONDITIONS":"1",
                   "RS2_AC_DISABLE_RETRIES":"1",
                   "RS2_AC_FORCE_BAD_RESULT":"1",
                   "RS2_AC_LOG_TO_STDOUT":"1"
                   #,"RS2_AC_IGNORE_LIMITERS":"1"
                   })

import pyrealsense2 as rs
from rspy import ac

# rs.log_to_file( rs.log_severity.debug, "rs.log" )

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

# Resetting sensors to factory calibration
dcs = rs.calibrated_sensor(depth_sensor)
dcs.reset_calibration()

ccs = rs.calibrated_sensor(color_sensor)
ccs.reset_calibration()

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

# This is the expected sequence of statuses for a successful calibration
successful_calibration_status_list = [rs.calibration_status.triggered,
                                      rs.calibration_status.special_frame,
                                      rs.calibration_status.started,
                                      rs.calibration_status.successful]

# Possible statuses that are irrelevant for this test
irrelevant_statuses = [rs.calibration_status.retry,
                       rs.calibration_status.scene_invalid,
                       rs.calibration_status.bad_result]

def filter_special_frames( list ):
    """
    Removes consecutive special frame statuses from the status list since we have a built-in
    retry mechanism and more than one request can be made.
    E.g., [triggered, special_frame, special_frame, started, successful]
    """
    i = 1
    while i < len(list):
        if list[i - 1] == list[i] == rs.calibration_status.special_frame:
            del list[i]
        else:
            i += 1

#############################################################################################
# Test #1
test.start("Depth sensor is off, should get an error")
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
except Exception as e:
    test.check_exception(e, RuntimeError, "not streaming")
else:
    test.unexpected_exception() # No error Occurred, should have received a RuntimeError
test.check(ac.status_list_is_empty()) # No status changes are expected, list should remain empty
test.finish()

#############################################################################################
# Test #2
test.start("Color sensor is off, calibration should succeed")
ac.reset_status_list() # Deleting previous test
depth_sensor.open( dp )
depth_sensor.start( lambda f: None )
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
    ac.trim_irrelevant_statuses(irrelevant_statuses)
    filter_special_frames( ac.status_list )
    test.check_equal_lists(ac.status_list, successful_calibration_status_list)
except Exception:
    test.unexpected_exception()
try:
    # Since the sensor was closed before calibration started, it should have been returned to a
    # closed state
    color_sensor.stop()
except Exception as e:
    test.check_exception(e, RuntimeError, "tried to stop sensor without starting it")
else:
    test.unexpected_exception()
# Leave the depth sensor open for the next test
test.finish()

#############################################################################################
# Test #3
test.start("Color sensor is on")
ac.reset_status_list()
dcs.reset_calibration()
ccs.reset_calibration()
color_sensor.open( cp )
color_sensor.start( lambda f: None )
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
    ac.trim_irrelevant_statuses(irrelevant_statuses)
    filter_special_frames( ac.status_list )
    test.check_equal_lists(ac.status_list, successful_calibration_status_list)
except:
    test.unexpected_exception()
try:
    # This time the color sensor was on before calibration so it should remain on at the end
    color_sensor.stop()
except:
    test.unexpected_exception()
# Leave the depth sensor open for the next test
test.finish()

#############################################################################################
# Test #4
test.start("2 AC triggers in a row")
ac.reset_status_list()
dcs.reset_calibration()
ccs.reset_calibration()
color_sensor.start( lambda f: None )
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
    ac.trim_irrelevant_statuses(irrelevant_statuses)
    filter_special_frames( ac.status_list )
    test.check_equal_lists(ac.status_list, successful_calibration_status_list)
except:
    test.unexpected_exception()
color_sensor.stop()
color_sensor.close()
depth_sensor.stop()
depth_sensor.close()
test.finish()

#############################################################################################
test.print_results_and_exit()
