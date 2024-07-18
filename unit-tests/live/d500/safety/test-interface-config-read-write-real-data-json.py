# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import json
from rspy import tests_wrapper as tw

#############################################################################################
# Helper Functions
#############################################################################################


#############################################################################################
# Tests
#############################################################################################

test.start("Verify Safety Sensor Extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
test.finish()

tw.start_wrapper(dev)
#############################################################################################

test.start("Writing safety interface config, then reading and comparing safety interface config JSONs")

# Save previous safety preset to restore it at the end
previous_result = safety_sensor.get_safety_interface_config(rs.calib_location.flash)

# Safety interface config JSON String representation to be written
sic_json_str = '''
{
    "safety_interface_config":
    {
        "m12_safety_pins_configuration": 
        {
            "power":
            {
                "direction": 0,
                "functionality": 1
            },
            "ossd1_b":
            {
                "direction": 1,
                "functionality": 3
            },
            "ossd1_a":
            {
                "direction": 1,
                "functionality": 2
            },
            "gpio_0":
            {
                "direction": 0,
                "functionality": 16
            },
            "gpio_1":
            {
                "direction": 0,
                "functionality": 17
            },
            "gpio_2":
            {
                "direction": 0,
                "functionality": 18
            },
            "gpio_3":
            {
                "direction": 0,
                "functionality": 19
            },
            "preset3_a":
            {
                "direction": 0,
                "functionality": 12
            },
            "preset3_b":
            {
                "direction": 0,
                "functionality": 13
            },
            "preset4_a":
            {
                "direction": 0,
                "functionality": 14
            },
            "preset1_b":
            {
                "direction": 0,
                "functionality": 9
            },
            "preset1_a":
            {
                "direction": 0,
                "functionality": 8
            },
            "preset2_b":
            {
                "direction": 0,
                "functionality": 11
            },
            "gpio_4":
            {
                "direction": 1,
                "functionality": 21
            },
            "preset2_a":
            {
                "direction": 0,
                "functionality": 10
            },
            "preset4_b":
            {
                "direction": 0,
                "functionality": 15
            },
            "ground":
            {
                "direction": 0,
                "functionality": 0
            }
        },
        "gpio_stabilization_interval" : 150,
        "camera_position":
        {
            "rotation":
            [
                [ 0.0,  0.0,  1.0],
                [-1.0,  0.0,  0.0],
                [ 0.0, -1.0,  0.0]
            ],
            "translation": [0.0, 0.0, 0.27]
        },
        "occupancy_grid_params":
        {
            "grid_cell_seed" : 100,
            "close_range_quorum" : 150,
            "mid_range_quorum" : 100,
            "long_range_quorum" : 50
        },
        "smcu_arbitration_params":
        {
            "l_0_total_threshold": 100,
            "l_0_sustained_rate_threshold": 20,
            "l_1_total_threshold": 100,
            "l_1_sustained_rate_threshold": 20,
            "l_4_total_threshold": 10,
            "hkr_stl_timeout": 40,
            "mcu_stl_timeout": 40,
            "sustained_aicv_frame_drops": 50,
            "ossd_self_test_pulse_width": 23
        },
        "crypto_signature": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "reserved": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    }
}
'''

# Remove new-lines and spaces from JSON string (facing encoding issues without these lines)
sic_json_str = ''.join(line.strip() for line in sic_json_str.split('\n'))
sic_json_str = sic_json_str.replace(' ', '')

# Convert json string to Safety Preset struct
sic = safety_sensor.json_string_to_safety_interface_config(sic_json_str)

# write the above sic table to the device
safety_sensor.set_safety_interface_config(sic)

# read the sic table from the device
read_result = safety_sensor.get_safety_interface_config()

# verify the tables are equal
test.check_equal(read_result, sic)

# verify the JSON objects are equal (comparing JSON object because
# the JSON string can have different order of inner fields
read_result_json_str = safety_sensor.safety_interface_config_to_json_string(read_result)
sic_json_str = safety_sensor.safety_interface_config_to_json_string(sic)
test.check_equal(json.loads(sic_json_str), json.loads(read_result_json_str))

# restore original table
safety_sensor.set_safety_interface_config(previous_result)
test.finish()

#############################################################################################
tw.stop_wrapper(dev)
test.print_results_and_exit()
