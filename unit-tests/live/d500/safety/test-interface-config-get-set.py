# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
import random
import time

#test:device D585S

import pyrealsense2 as rs
from rspy import test
from safety_common import set_operational_mode

def generate_default_config_1():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_b)
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_a)
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
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

def generate_default_config_2():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_b)
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_a)
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
    cfg.preset1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_a)
    cfg.gpio_0 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_a)
    cfg.gpio_1 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_b)
    cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.maintenance)
    cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.device_ready)
    cfg.preset2_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_b)
    cfg.gpio_4 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.restart_interlock)
    cfg.preset2_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_a)
    cfg.preset4_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_b)
    cfg.ground = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.gnd)
    cfg.gpio_stabilization_interval = 30 # ms
    cfg.safety_zone_selection_overlap_time_period = 35 # 35 --> 3.5 sec
    return cfg

def generate_default_config_3():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_b)
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_a)
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
    cfg.preset1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_a)
    cfg.gpio_0 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_a)
    cfg.gpio_1 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_b)
    cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_b)
    cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_a)
    cfg.preset2_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_b)
    cfg.gpio_4 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.restart_interlock)
    cfg.preset2_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_a)
    cfg.preset4_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_b)
    cfg.ground = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.gnd)
    cfg.gpio_stabilization_interval = 30  # ms
    cfg.safety_zone_selection_overlap_time_period = 35  # 35 --> 3.5 sec
    return cfg

def generate_default_config_4():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_b)
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_a)
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
    cfg.preset1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_a)
    cfg.gpio_0 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.ossd2_a_feedback)
    cfg.gpio_1 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.ossd2_b_feedback)
    cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_b)
    cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_a)
    cfg.preset2_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_b)
    cfg.gpio_4 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.restart_interlock)
    cfg.preset2_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_a)
    cfg.preset4_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_b)
    cfg.ground = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.gnd)
    cfg.gpio_stabilization_interval = 30  # ms
    cfg.safety_zone_selection_overlap_time_period = 35  # 35 --> 3.5 sec
    return cfg

def generate_config():
    config_number = random.randint(1, 4)
    if config_number == 1:
        return generate_default_config_1()
    if config_number == 2:
        return generate_default_config_2()
    if config_number == 3:
        return generate_default_config_3()
    return generate_default_config_4()

def generate_bad_config():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.p24vdc) # intentional error
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.p24vdc) # intentional error
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
    cfg.preset1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_a)
    cfg.gpio_0 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.ossd2_a_feedback)
    cfg.gpio_1 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.ossd2_b_feedback)
    cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_b)
    cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_a)
    cfg.preset2_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_b)
    cfg.gpio_4 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.restart_interlock)
    cfg.preset2_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_a)
    cfg.preset4_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_b)
    cfg.ground = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.gnd)
    cfg.gpio_stabilization_interval = 30  # ms
    cfg.safety_zone_selection_overlap_time_period = 35  # 35 --> 3.5 sec
    return cfg

def print_config(config):
    print("config.power: " + repr(config.power.direction) + ", " + repr(config.power.functionality))
    print("config.ossd1_b: " + repr(config.ossd1_b.direction) + ", " + repr(config.ossd1_b.functionality))
    print("config.ossd1_a: " + repr(config.ossd1_a.direction) + ", " + repr(config.ossd1_a.functionality))
    print("config.preset3_a: " + repr(config.preset3_a.direction) + ", " + repr(config.preset3_a.functionality))
    print("config.preset3_b: " + repr(config.preset3_b.direction) + ", " + repr(config.preset3_b.functionality))
    print("config.preset4_a: " + repr(config.preset4_a.direction) + ", " + repr(config.preset4_a.functionality))
    print("config.preset1_b: " + repr(config.preset1_b.direction) + ", " + repr(config.preset1_b.functionality))
    print("config.preset1_a: " + repr(config.preset1_a.direction) + ", " + repr(config.preset1_a.functionality))
    print("config.gpio_0: " + repr(config.gpio_0.direction) + ", " + repr(config.gpio_0.functionality))
    print("config.gpio_1: " + repr(config.gpio_1.direction) + ", " + repr(config.gpio_1.functionality))
    print("config.gpio_3: " + repr(config.gpio_3.direction) + ", " + repr(config.gpio_3.functionality))
    print("config.gpio_2: " + repr(config.gpio_2.direction) + ", " + repr(config.gpio_2.functionality))
    print("config.preset2_b: " + repr(config.preset2_b.direction) + ", " + repr(config.preset2_b.functionality))
    print("config.gpio_4: " + repr(config.gpio_4.direction) + ", " + repr(config.gpio_4.functionality))
    print("config.preset2_a: " + repr(config.preset2_a.direction) + ", " + repr(config.preset2_a.functionality))
    print("config.preset4_b: " + repr(config.preset4_b.direction) + ", " + repr(config.preset4_b.functionality))
    print("config.ground: " + repr(config.ground.direction) + ", " + repr(config.ground.functionality))
    print("config.gpio_stabilization_interval: " + repr(config.gpio_stabilization_interval))
    print("config.safety_zone_selection_overlap_time_period: " + repr(config.safety_zone_selection_overlap_time_period))


def check_pin_equal(first_pin, second_pin):
    test.check(first_pin.direction, second_pin.direction)
    test.check(first_pin.functionality, second_pin.functionality)
def check_configurations_equal(first_config, second_config) :
    check_pin_equal(first_config.power, second_config.power)
    check_pin_equal(first_config.ossd1_b, second_config.ossd1_b)
    check_pin_equal(first_config.ossd1_a, second_config.ossd1_a)
    check_pin_equal(first_config.preset3_a, second_config.preset3_a)
    check_pin_equal(first_config.preset3_b, second_config.preset3_b)
    check_pin_equal(first_config.preset4_a, second_config.preset4_a)
    check_pin_equal(first_config.preset1_b, second_config.preset1_b)
    check_pin_equal(first_config.preset1_a, second_config.preset1_a)
    check_pin_equal(first_config.gpio_0, second_config.gpio_0)
    check_pin_equal(first_config.gpio_1, second_config.gpio_1)
    check_pin_equal(first_config.gpio_3, second_config.gpio_3)
    check_pin_equal(first_config.gpio_2, second_config.gpio_2)
    check_pin_equal(first_config.preset2_b, second_config.preset2_b)
    check_pin_equal(first_config.gpio_4, second_config.gpio_4)
    check_pin_equal(first_config.preset2_a, second_config.preset2_a)
    check_pin_equal(first_config.preset4_b, second_config.preset4_b)
    check_pin_equal(first_config.ground, second_config.ground)
    test.check_equal(first_config.gpio_stabilization_interval, second_config.gpio_stabilization_interval)
    test.check_equal(first_config.safety_zone_selection_overlap_time_period, second_config.safety_zone_selection_overlap_time_period)


#############################################################################################
# Tests
#############################################################################################

ctx = rs.context()
dev = test.find_first_device_or_exit()
safety_sensor = dev.first_safety_sensor()
original_mode = safety_sensor.get_option(rs.option.safety_mode)

#############################################################################################
test.start("Switch to Service Mode")
test.check(set_operational_mode(safety_sensor, rs.safety_mode.service))
test.finish()

#############################################################################################
test.start("Valid get/set scenario")

# preparing default config
default_config = generate_config()

config_to_be_restored = False

# The aim of the following try statement is:
# - For devices that had never had Safety Interface Configuration (SIC) set:
#    setting SIC for the first time
# - For devices that have already SIC set:
#    it saves the current SIC, and restores it at the end of the test.
try:
    # getting safety config
    safety_config_to_restore = safety_sensor.get_safety_interface_config(rs.calib_location.ram)
except:
    print("Safety Interface Configuration was not available in this device")
else:
    config_to_be_restored = True
finally:
    # write default config to the device
    safety_sensor.set_safety_interface_config(default_config)

    # read the config in the device
    config_to_check = safety_sensor.get_safety_interface_config()

    # checking the requested config has been written to the device
    # uncomment following lines for debugging
    #print("default config:")
    #print_config(default_config)
    #print("config_to_check:")
    #print_config(config_to_check)
    check_configurations_equal(default_config, config_to_check)

    # restore original config
    if safety_config_to_restore:
        safety_sensor.set_safety_interface_config(safety_config_to_restore)

test.finish()

#############################################################################################
test.start("Setting bad config - checking error is received, and that config_1 is returned after get action")
# setting bad config
test.check_throws(lambda: safety_sensor.set_safety_interface_config(generate_bad_config()), RuntimeError)

# getting active config
current_config = safety_sensor.get_safety_interface_config()

# checking active config is the default one
check_configurations_equal(generate_default_config_1(), current_config)

test.finish()

#############################################################################################
test.start("Checking config is the same in flash and in ram")
# getting config from ram
config_from_ram = safety_sensor.get_safety_interface_config()

# getting config from flash
config_from_flash = safety_sensor.get_safety_interface_config(rs.calib_location.flash)

# checking config is the same in flash and in ram
check_configurations_equal(config_from_ram, config_from_flash)

test.finish()

#############################################################################################
test.start("Restoring original safety mode")
test.check(set_operational_mode(safety_sensor, original_mode))
test.finish()

#############################################################################################

test.print_results_and_exit()
