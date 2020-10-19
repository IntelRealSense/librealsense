import sys
pyrs = "C:/Users/mmirbach/git/librealsense/build/Debug"
py = "C:/Users/mmirbach/git/librealsense/unit-tests/py"
sys.path.append(pyrs)
sys.path.append(py)

import pyrealsense2 as rs, common as test, ac

# We set the enviroment variables to suit this test
test.set_env_vars({"RS2_AC_DISABLE_CONDITIONS":"1",
                   "RS2_AC_DISABLE_RETRIES":"1",
                   "RS2_AC_FORCE_BAD_RESULT":"1"
                   })

# rs.log_to_file( rs.log_severity.debug, "rs.log" )

dev = test.get_first_device()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

d2r = rs.device_calibration(dev)
d2r.register_calibration_change_callback( ac.list_status_cb )

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

#############################################################################################
# Test #1
test.start("Depth sensor is off, should get an error")
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration() 
except Exception as e:
    test.require_exception(e, RuntimeError, "not streaming")
else:
    test.require_no_reach() # No error accured, should have received a RuntimrError
test.require(ac.status_list_empty()) # No status changes are expected, list should remain empty
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
    test.require_equal_lists(ac.status_list, successful_calibration_status_list)
except Exception:
    test.require_no_reach()
try:
    # Since the sensor was closed before calibration started, it should have been returned to a 
    # closed state
    color_sensor.stop() 
except Exception as e:
    test.require_exception(e, RuntimeError, "tried to stop sensor without starting it")
else:
    test.require_no_reach()
test.finish()

#############################################################################################
# Test #3
test.start("Color sensor is on")
ac.reset_status_list()
color_sensor.open( cp )
color_sensor.start( lambda f: None )
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
    ac.trim_irrelevant_statuses(irrelevant_statuses)
    test.require_equal_lists(ac.status_list, successful_calibration_status_list)
except Exception:
    require_no_reach()
try:
    # This time the color sensor was on before calibration so it should remain on at the end
    color_sensor.stop() 
except Exception:
    test.require_no_reach()
test.finish()

#############################################################################################
# Test #4
test.start("2 AC triggers in a row")
ac.reset_status_list()
color_sensor.start( lambda f: None )
d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
except Exception as e: # Second trigger should throw exception
    test.require_exception(e, RuntimeError, "Camera Accuracy Health is already active")
else:
    test.require_no_reach()
ac.wait_for_calibration() # First trigger should continue and finish successfully
ac.trim_irrelevant_statuses(irrelevant_statuses)
test.require_equal_lists(ac.status_list, successful_calibration_status_list)
test.finish()

#############################################################################################
test.print_results_and_exit()