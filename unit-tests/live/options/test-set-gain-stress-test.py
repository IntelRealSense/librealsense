# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test, log
import time
import datetime

# Test multiple set_pu commands checking that the set control event polling works as expected.
# We expect no exception thrown -  See [DSO-17185]

test_iterations = 200
gain_values = [16,74,132,190,248]

################################################################################################
test.start("Stress test for setting a PU (gain) option")

dev = test.find_first_device_or_exit()
time.sleep( 3 ) # The device starts at D0 (Operational) state, allow time for it to get into idle state
depth_ir_sensor = dev.first_depth_sensor()

for i in range(test_iterations):
    log.d("{} Itearion {} {}".format("=" * 50, i, "=" * 50))

    # Reset Control values
    log.d("Resetting Controls...")
    depth_ir_sensor.set_option(rs.option.enable_auto_exposure, 0)
    depth_ir_sensor.set_option(rs.option.exposure, 1)
    depth_ir_sensor.set_option(rs.option.gain, 248)
    log.d("Resetting Controls Done")

    time.sleep(0.1)
    for val in gain_values:
        log.d("Setting Gain To: {}".format(val))
        depth_ir_sensor.set_option(rs.option.gain, val)
        get_val = depth_ir_sensor.get_option(rs.option.gain)
        test.check(val == get_val)
        log.d("Gain Set To: {}".format(get_val))

        time.sleep(0.1)

test.finish()

################################################################################################
test.print_results_and_exit()
