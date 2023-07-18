# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:device D585S

import pyrealsense2 as rs
from rspy import test

#############################################################################################
# Tests
#############################################################################################

ctx = rs.context()
dev = test.find_first_device_or_exit()
safety_sensor = dev.first_safety_sensor()

#############################################################################################
test.start("Valid get/set scenario")

safety_interface_config = safety_sensor.get_safety_interface_config()
previous_config = safety_interface_config

# modifying the config and setting it
safety_interface_config.gpio_4.direction = 1
safety_sensor.set_safety_interface_config(safety_interface_config)

# checking the get value equals to the config set before
new_safety_interface_config = safety_sensor.get_safety_interface_config()
test.check_equal(new_safety_interface_config, safety_interface_config)

# restore original config
safety_sensor.set_safety_interface_config(previous_config)

test.finish()

#############################################################################################

test.print_results_and_exit()
