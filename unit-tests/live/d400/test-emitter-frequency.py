# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:device D455
#test:device D457

import pyrealsense2 as rs
from rspy import test

ctx = rs.context()
device = test.find_first_device_or_exit();
depth_sensor = device.first_depth_sensor()

EMITTER_FREQUENCY_57_KHZ = 0.0
EMITTER_FREQUENCY_91_KHZ = 1.0
################################################################################################

test.start("Verify camera defaults")
device_nane = device.get_info(rs.camera_info.name)
if "D455" in device_nane:
    test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_57_KHZ)
elif "D457" in device_nane:
    test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_91_KHZ)
else:
    test.fail("Unexpected device name found: " + device_nane)
test.finish()

################################################################################################

test.start("Test Set On/Off during idle mode")
depth_sensor.set_option(rs.option.emitter_frequency,EMITTER_FREQUENCY_57_KHZ)
test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_57_KHZ)
depth_sensor.set_option(rs.option.emitter_frequency,EMITTER_FREQUENCY_91_KHZ)
test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_91_KHZ)
test.finish()

################################################################################################

test.start("Test Set On/Off during streaming mode is not allowed")
# Reset option to 57 [KHZ]
depth_sensor.set_option(rs.option.emitter_frequency,EMITTER_FREQUENCY_57_KHZ)
test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_57_KHZ)
depth_profile = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth)
depth_sensor.open(depth_profile)
depth_sensor.start(lambda x: None)
try:
    depth_sensor.set_option(rs.option.emitter_frequency,EMITTER_FREQUENCY_91_KHZ)
    test.fail("Exception was expected while setting emitter frequency during streaming depth sensor")
except:
    test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_57_KHZ)

depth_sensor.stop()
depth_sensor.close()
test.finish()

################################################################################################
test.print_results_and_exit()