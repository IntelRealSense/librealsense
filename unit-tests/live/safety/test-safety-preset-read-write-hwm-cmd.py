# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun  ## line to be removed when we connect D585S to our LibCI
#test:donotrun:!nightly
#test:device D585S

import pyrealsense2 as rs
from rspy import test

#############################################################################################
# Tests
#############################################################################################

test.start("Verify Safety Sensor Extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
test.finish()

#############################################################################################

test.start("Valid read from index 0 and valid write to index 1")
safety_preset_at_zero = safety_sensor.get_safety_preset(0)
safety_sensor.set_safety_preset(1, safety_preset_at_zero)
safety_preset_at_one = safety_sensor.get_safety_preset(1)
test.finish()

#############################################################################################

test.start("Valid read from index 0 and valid write to index 63")
safety_preset_at_zero = safety_sensor.get_safety_preset(0)
safety_sensor.set_safety_preset(63, safety_preset_at_zero)
safety_preset_at_63 = safety_sensor.get_safety_preset(63)
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

test.start("Invalid Write - index 0 is readonly")
try:
    safety_sensor.set_safety_preset(0, safety_preset_at_zero)
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

test.print_results_and_exit()
