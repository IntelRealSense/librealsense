# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device S585*

import pyrealsense2 as rs
from rspy import devices, log, test, file, repo


#############################################################################################
# Help Functions
#############################################################################################



#############################################################################################
# Tests
#############################################################################################

test.start("Init")
try:
    ctx = rs.context()
    dev = ctx.query_devices()[0]
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("get safety preset at index 1")
try:
    safety_preset_at_1 = dev.get_safety_preset_at_index(0)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("set safety preset at index 1")
try:
    new_safety_preset_at_1 = manupliate_sz(safety_preset_at_1)
    dev.set_safety_preset_at_index(1, new_safety_preset_at_1)
except:
    test.unexpected_exception()
test.finish()


#############################################################################################

test.start("set safety preset at index 0")
# should throw error ?
try:
    dev.set_safety_preset_at_index(0, new_safety_preset_at_1)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("read all safety presets ")
try:
    for i in range(64):
        current_safety_preset = dev.get_safety_preset_at_index(0)
		# print ?
except:
    test.unexpected_exception()
test.finish()


#############################################################################################

test.start("set safety preset at index 1 back to its original value")
try:
    dev.set_safety_preset_at_index(1, safety_preset_at_1)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################



test.print_results_and_exit()
