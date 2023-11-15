# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
import time

#test:device D585S


import pyrealsense2 as rs
from rspy import test, log, devices

def generate_valid_table():
    cfg = rs.safety_interface_config()
    cfg.power = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.p24vdc)
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_b)
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd1_a)
    cfg.gpio_0 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_a)
    cfg.gpio_1 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select5_b)
    cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select6_a)
    cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select6_b)
    cfg.preset3_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_a)
    cfg.preset3_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select3_b)
    cfg.preset4_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_a)
    cfg.preset1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_b)
    cfg.preset1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select1_a)
    cfg.preset2_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_b)
    cfg.gpio_4 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.restart_interlock)
    cfg.preset2_a = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select2_a)
    cfg.preset4_b = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select4_b)
    cfg.ground = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.gnd)
    cfg.gpio_stabilization_interval = 150 # [ms] - SMCU only accept 150 for now
    cfg.safety_zone_selection_overlap_time_period = 0 # SMCU only accept 0 for now
    return cfg
 

def change_config(cfg):
    
    # we query and replace GPIO 2-3 to make sure we have a different table then the one we got.
    if cfg.gpio_2.functionality == rs.safety_pin_functionality.preset_select6_a:
      cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_a)
    else:
      cfg.gpio_2 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select6_a)
    
    if cfg.gpio_3.functionality == rs.safety_pin_functionality.preset_select6_b:
      cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.ossd2_b)
    else:
      cfg.gpio_3 = rs.safety_interface_config_pin(rs.safety_pin_direction.input, rs.safety_pin_functionality.preset_select6_b)

    return cfg

def generate_bad_config():
    cfg = generate_valid_table()
    cfg.ossd1_b = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.p24vdc) # intentional error
    cfg.ossd1_a = rs.safety_interface_config_pin(rs.safety_pin_direction.output, rs.safety_pin_functionality.p24vdc) # intentional error
    return cfg

def print_config(config):
    log.d("config.power: " + repr(config.power.direction) + ", " + repr(config.power.functionality))
    log.d("config.ossd1_b: " + repr(config.ossd1_b.direction) + ", " + repr(config.ossd1_b.functionality))
    log.d("config.ossd1_a: " + repr(config.ossd1_a.direction) + ", " + repr(config.ossd1_a.functionality))
    log.d("config.preset3_a: " + repr(config.preset3_a.direction) + ", " + repr(config.preset3_a.functionality))
    log.d("config.preset3_b: " + repr(config.preset3_b.direction) + ", " + repr(config.preset3_b.functionality))
    log.d("config.preset4_a: " + repr(config.preset4_a.direction) + ", " + repr(config.preset4_a.functionality))
    log.d("config.preset1_b: " + repr(config.preset1_b.direction) + ", " + repr(config.preset1_b.functionality))
    log.d("config.preset1_a: " + repr(config.preset1_a.direction) + ", " + repr(config.preset1_a.functionality))
    log.d("config.gpio_0: " + repr(config.gpio_0.direction) + ", " + repr(config.gpio_0.functionality))
    log.d("config.gpio_1: " + repr(config.gpio_1.direction) + ", " + repr(config.gpio_1.functionality))
    log.d("config.gpio_3: " + repr(config.gpio_3.direction) + ", " + repr(config.gpio_3.functionality))
    log.d("config.gpio_2: " + repr(config.gpio_2.direction) + ", " + repr(config.gpio_2.functionality))
    log.d("config.preset2_b: " + repr(config.preset2_b.direction) + ", " + repr(config.preset2_b.functionality))
    log.d("config.gpio_4: " + repr(config.gpio_4.direction) + ", " + repr(config.gpio_4.functionality))
    log.d("config.preset2_a: " + repr(config.preset2_a.direction) + ", " + repr(config.preset2_a.functionality))
    log.d("config.preset4_b: " + repr(config.preset4_b.direction) + ", " + repr(config.preset4_b.functionality))
    log.d("config.ground: " + repr(config.ground.direction) + ", " + repr(config.ground.functionality))
    log.d("config.gpio_stabilization_interval: " + repr(config.gpio_stabilization_interval))
    log.d("config.safety_zone_selection_overlap_time_period: " + repr(config.safety_zone_selection_overlap_time_period))


def check_pin_equal(first_pin, second_pin):
    test.check_equal(first_pin.direction, second_pin.direction)
    test.check_equal(first_pin.functionality, second_pin.functionality)
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

dev = test.find_first_device_or_exit()
safety_sensor = dev.first_safety_sensor()
original_mode = safety_sensor.get_option(rs.option.safety_mode)

#############################################################################################
test.start("Switch to Service Mode")
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.service))
test.finish()

#############################################################################################
test.start("Valid get/set scenario")

# The aim of the following try statement is:
# For devices that had never had Safety Interface Configuration (SIC) set it and fail the test so the user will know and exit with failure
try:
    # getting safety config
    initial_config = safety_sensor.get_safety_interface_config(rs.calib_location.ram)
except:
    log.w("Safety Interface Configuration was not available in this device, writing a valid table now, rerun the test to verify it pass")
    safety_sensor.set_safety_interface_config(generate_valid_table())
    test.abort()
   
# We read the table from the device, modify it and write it back
# This way we are sure that the write process worked ()   
config_we_write = change_config(initial_config)
# write changed config to the device
safety_sensor.set_safety_interface_config(config_we_write)

# read the config in the device
config_we_read = safety_sensor.get_safety_interface_config()


# Add debugging info
log.d("config we write:")
print_config(config_we_write)
log.d("config we read:")
print_config(config_we_read)

# checking the requested config has been written to the device
check_configurations_equal(config_we_write, config_we_read)

test.finish()



#############################################################################################
test.start("verify same table after camera reboot")

log.d( "Sending HW-reset command" )
dev.hardware_reset()

log.d( "sleep to give some time for the device to reconnect" )
time.sleep( devices.MAX_ENUMERATION_TIME )

log.d( "Fetching new device" )
dev = test.find_first_device_or_exit()
safety_sensor = dev.first_safety_sensor()

log.d( "Setting operational mode to service" )
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.service))

config_after_reboot = safety_sensor.get_safety_interface_config(rs.calib_location.flash)

# Add debugging info
log.d("config we write:")
print_config(config_we_write)
log.d("config after reboot:")
print_config(config_after_reboot)

# checking our last write stay the same after reboot
check_configurations_equal(config_we_write, config_after_reboot)

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
test.start("Setting bad config - checking error is received, and that config_1 is returned after get action")

# setting bad config
test.check_throws(lambda: safety_sensor.set_safety_interface_config(generate_bad_config()), RuntimeError)

# getting active config
current_config = safety_sensor.get_safety_interface_config()

# checking active config is the default one
check_configurations_equal(generate_valid_table(), current_config)
test.finish()
#############################################################################################

test.start("Restoring original table + safety mode")
# write initial config back to the device
safety_sensor.set_safety_interface_config(config_we_write)
safety_sensor.set_option(rs.option.safety_mode, original_mode)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), original_mode)
test.finish()

#############################################################################################

test.print_results_and_exit()
