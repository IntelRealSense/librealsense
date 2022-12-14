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

test.start("Valid Get/Set")
try:
    if safety_sensor.supports(rs.option.active_safety_preset_index):

        # default index at start should be 0
        current_index = safety_sensor.get_option(rs.option.active_safety_preset_index)
        test.check_equal(current_index, 0)

        safety_sensor.set_option(rs.option.active_safety_preset_index, 1)
        current_index = safety_sensor.get_option(rs.option.active_safety_preset_index)
        test.check_equal(current_index, 1)

        safety_sensor.set_option(rs.option.active_safety_preset_index, 20)
        current_index = safety_sensor.get_option(rs.option.active_safety_preset_index)
        test.check_equal(current_index, 20)

        current_index = safety_sensor.get_option(rs.option.active_safety_preset_index)
        safety_sensor.set_option(rs.option.active_safety_preset_index, 63)
        test.check_equal(current_index, 63)
    else:
        raise Exception
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Invalid Set - index out of range")
try:
    if safety_sensor.supports(rs.option.active_safety_preset_index):
        safety_sensor.set_option(rs.option.active_safety_preset_index, 64)
    else:
        raise Exception
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.print_results_and_exit()
