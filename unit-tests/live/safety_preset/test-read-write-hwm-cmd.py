# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device S585

import pyrealsense2 as rs
from rspy import devices, log, test, file, repo

#############################################################################################
# Tests
#############################################################################################

test.start("Init")
try:
    ctx = rs.context()
    dev = ctx.query_devices()[0]
    safety_sensor = dev.first_safety_sensor()
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Valid read from index 0 and valid write to index 1")
try:
    safety_preset_at_zero = safety_sensor.get_safety_preset(0)
    safety_sensor.set_safety_preset(1, safety_preset_at_zero)
    safety_preset_at_one = safety_sensor.get_safety_preset(1)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Valid read from index 0 and valid write to index 63")
try:
    safety_preset_at_zero = safety_sensor.get_safety_preset(0)
    safety_sensor.set_safety_preset(63, safety_preset_at_zero)
    safety_preset_at_63 = safety_sensor.get_safety_preset(63)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Invalid Read - index out of range")
try:
    sp = safety_sensor.get_safety_preset(64)
except:
    test.finish()
else:
    test.unexpected_exception()

#############################################################################################

test.start("Invalid Write - index 0 is readonly")
try:
    safety_sensor.set_safety_preset(0, safety_preset_at_zero)
except:
    test.finish()
else:
    test.unexpected_exception()

#############################################################################################

test.start("Invalid Write - index out of range")
try:
    safety_sensor.set_safety_preset(64, safety_preset_at_zero)
except:
    test.finish()
else:
    test.unexpected_exception()

#############################################################################################

test.print_results_and_exit()
