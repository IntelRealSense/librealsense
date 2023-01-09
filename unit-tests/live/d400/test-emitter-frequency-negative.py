# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun:jetson
#test:device each(D400*) !D455

import pyrealsense2 as rs
from rspy import test

ctx = rs.context()
device = test.find_first_device_or_exit()
depth_sensor = device.first_depth_sensor()

################################################################################################
test.start("emitter frequency is not supported on legacy devices")
test.check_equal(depth_sensor.supports(rs.option.emitter_frequency), False)
test.finish()
################################################################################################
test.print_results_and_exit()
