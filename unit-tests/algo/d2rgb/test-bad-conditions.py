
# add these 2 lines to run independently and not through run-unit-tests.py
path = "C:/Users/mmirbach/git/librealsense/build/RelWithDebInfo"
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

depth_sensor.open( dp )
depth_sensor.start( lambda f: None )
color_sensor.open( cp )
color_sensor.start( lambda f: None )

#############################################################################################
test.start("Failing check_conditions function")
depth_sensor.set_option(rs.option.ambient_light, 2)
depth_sensor.set_option(rs.option.receiver_gain, 0)
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
except Exception as e:
    test.require_exception(e, RuntimeError)
else:
    test.require_no_reach()
test.require_equal_lists(ac.status_list, [rs.calibration_status.bad_conditions])
test.finish_case()
#############################################################################################
test.print_results_and_exit()