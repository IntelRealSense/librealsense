# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:device D585S

import pyrealsense2 as rs
from rspy import test

def generate_default_config():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_b)
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_a)
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_a)
    cfg.gpio_0 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_a)
    cfg.gpio_1 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_b)
    cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select6_b)
    cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select6_a)
    cfg.preset2_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_b)
    cfg.gpio_4 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.restart_interlock)
    cfg.preset2_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_a)
    cfg.preset4_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_b)
    cfg.ground = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.gnd)
    cfg.gpio_stabilization_interval = 30 # ms
    cfg.safety_zone_selection_overlap_time_period = 35 # 35 --> 3.5 sec
    return cfg


#############################################################################################
# Tests
#############################################################################################

ctx = rs.context()
dev = test.find_first_device_or_exit()
safety_sensor = dev.first_safety_sensor()

print("getting original safety mode")
original_mode = safety_sensor.get_option(rs.option.safety_mode)
print("switching to service mode")
safety_sensor.set_option(rs.option.safety_mode, 2) # service mode

#############################################################################################
test.start("Valid get/set scenario")

# getting safety config
safety_config_to_restore = safety_sensor.get_safety_interface_config()

# preparing default config
default_config = generate_default_config()

# write default config to the device
safety_sensor.set_safety_interface_config(default_config)

# read the config in the device
config_to_check = safety_sensor.get_safety_interface_config()

# checking the requested config has been written to the device
test.check_equal(default_config, config_to_check)

# restore original config
safety_sensor.set_safety_interface_config(safety_config_to_restore)

test.finish()

#############################################################################################

print("restoring original safety mode")
safety_sensor.set_option(rs.option.safety_mode, original_mode) # service mode

test.print_results_and_exit()
