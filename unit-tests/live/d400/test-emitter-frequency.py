# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:device:jetson D457
#test:device:!jetson D455

import pyrealsense2 as rs
import pyrsutils as rsutils
from rspy import test, log, repo

device, _ = test.find_first_device_or_exit();
depth_sensor = device.first_depth_sensor()

fw_version = rsutils.version( device.get_info( rs.camera_info.firmware_version ))
if fw_version <= rsutils.version(5,14,0,0):
    log.i(f"FW version {fw_version} does not support EMITTER_FREQUENCY option, skipping test...")
    test.print_results_and_exit()


EMITTER_FREQUENCY_57_KHZ = 0.0
EMITTER_FREQUENCY_91_KHZ = 1.0
################################################################################################

test.start("Verify camera defaults")
device_name = device.get_info(rs.camera_info.name)
if "D455" in device_name:
    test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_57_KHZ)
elif "D457" in device_name:
    test.check_equal(depth_sensor.get_option(rs.option.emitter_frequency), EMITTER_FREQUENCY_91_KHZ)
else:
    test.fail("Unexpected device name found: " + device_name)
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
