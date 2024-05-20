# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:device D585S

import pyrealsense2 as rs
from rspy import test
import time

# Constants
RUN_MODE     = 0 # RS2_SAFETY_MODE_RUN (RESUME)
STANDBY_MODE = 1 # RS2_SAFETY_MODE_STANDBY (PAUSE)
SERVICE_MODE = 2 # RS2_SAFETY_MODE_SERVICE (MAINTENANCE)

#############################################################################################
# Tests
#############################################################################################

test.start("Verify Safety Sensor Extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
test.finish()

#############################################################################################
test.start("Valid read from index 0")
safety_preset_at_zero = safety_sensor.get_safety_preset(0)
test.finish()

#############################################################################################
test.start("Valid read from index 1")
safety_preset_at_one = safety_sensor.get_safety_preset(1)
test.finish()

#############################################################################################
test.start("Switch to Service Mode")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.service))
test.finish()

#############################################################################################

test.start("Valid read and write from index 1 to 0")
safety_preset_at_one = safety_sensor.get_safety_preset(1)
safety_sensor.set_safety_preset(0, safety_preset_at_one)
test.finish()

#############################################################################################

test.start("Valid read and write from index 1 to 2")
safety_preset_at_one = safety_sensor.get_safety_preset(1)
safety_sensor.set_safety_preset(2, safety_preset_at_one)
test.finish()

#############################################################################################

test.start("Valid read and write from index 63")
safety_preset_at_63 = safety_sensor.get_safety_preset(63)
safety_sensor.set_safety_preset(63, safety_preset_at_63)
test.finish()

#############################################################################################

test.start("Invalid Read - index out of range")
try:
    sp = safety_sensor.get_safety_preset(64)
except:
    pass
else:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Invalid Write - index out of range")
try:
    safety_sensor.set_safety_preset(64, safety_preset_at_zero)
except:
    pass
else:
    test.unexpected_exception()
test.finish()

#############################################################################################

# switch back to Run safety mode
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.run)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.run))

test.print_results_and_exit()
