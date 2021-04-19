# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

from rspy import test
test.set_env_vars({"RS2_AC_DISABLE_CONDITIONS":"0",
                   "RS2_AC_LOG_TO_STDOUT":"1"
                   })

import pyrealsense2 as rs
from rspy import ac

# rs.log_to_file( rs.log_severity.debug, "rs.log" )

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

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

depth_sensor.open( dp )
depth_sensor.start( lambda f: None )
color_sensor.open( cp )
color_sensor.start( lambda f: None )

#############################################################################################
test.start("Failing check_conditions function")
# If ambient light is RS2_DIGITAL_GAIN_LOW (2) receiver_gain must be 18
# If ambient light is RS2_DIGITAL_GAIN_HIGH (1) receiver_gain must be 9
old_receiver_gain = depth_sensor.get_option(rs.option.receiver_gain)
depth_sensor.set_option(rs.option.receiver_gain, 15)
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    test.unreachable()
    ac.wait_for_calibration()
except Exception as e:
    test.check_exception(e, RuntimeError)
else:
    test.unexpected_exception()
test.check_equal_lists(ac.status_list, [rs.calibration_status.bad_conditions])
depth_sensor.set_option(rs.option.receiver_gain, old_receiver_gain)
test.finish()
#############################################################################################

color_sensor.stop()
color_sensor.close()
depth_sensor.stop()
depth_sensor.close()

test.print_results_and_exit()
