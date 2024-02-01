# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:device D585S

#test:donotrun:!nightly

import pyrealsense2 as rs
from rspy import test, log
import time

ITERATIONS_COUNT = 20

device = test.find_first_device_or_exit();
safety_sensor = device.first_safety_sensor()
#########################################################################
# since we see many regressions on operational mode switching we add a short stress test
with test.closure("Operational mode stress test"):
    for i in range( ITERATIONS_COUNT ):
        log.d("stress test iteration:", i)
        log.d("command service mode")
        safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)
        test.check_equal(safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.service))

        log.d("command standby mode")
        safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.standby)
        test.check_equal(safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.standby))

        log.d("command run mode")
        safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.run)
        test.check_equal(safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.run))

################################################################################################
test.print_results_and_exit()
