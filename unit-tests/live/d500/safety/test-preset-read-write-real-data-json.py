# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:device D585S

import pyrealsense2 as rs
import random
from rspy import test, log
import time
import json


#############################################################################################
# Helper Functions
#############################################################################################

# Constants
RUN_MODE     = 0 # RS2_SAFETY_MODE_RUN (RESUME)
STANDBY_MODE = 1 # RS2_SAFETY_MODE_STANDBY (PAUSE)
SERVICE_MODE = 2 # RS2_SAFETY_MODE_SERVICE (MAINTENANCE)

#############################################################################################
# Tests
#############################################################################################

test.start("Verify Safety Sensor Extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
test.finish()

#############################################################################################

test.start("Switch to Service Mode")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.service))
test.finish()

#############################################################################################


test.start("Writing safety preset to random index, then reading and comparing safety presets JSONs")
index = random.randint(1, 63)
log.out( "writing to index = ", index )

# Save previous safety preset to restore it at the end
previous_result = safety_sensor.get_safety_preset(index)

# Safety Preset JSON String representation to be written
sp_json_str = """
{
    "safety_preset":
    {
        "platform_config": 
        {
            "transformation_link":
            {
                "rotation":
                [
                    [ 0.0,  0.0,  1.0],
                    [-1.0,  0.0,  0.0],
                    [ 0.0, -1.0,  0.0]
                ],
                "translation": [0.0, 0.0, 0.28]
            },
            "robot_height": 1.0
        },
        "safety_zones": 
        {
            "danger_zone":
            {
                "zone_polygon":
                {
                    "p0": {"x": 0.5, "y":  0.1},
                    "p1": {"x": 0.8, "y":  0.1},
                    "p2": {"x": 0.8, "y": -0.1},
                    "p3": {"x": 0.5, "y": -0.1}
                },
                "safety_trigger_confidence": 1
            },
            "warning_zone":
            {
                "zone_polygon":
                {
                    "p0": {"x": 0.8, "y":  0.1},
                    "p1": {"x": 1.2, "y":  0.1},
                    "p2": {"x": 1.2, "y": -0.1},
                    "p3": {"x": 0.8, "y": -0.1}
                },
                "safety_trigger_confidence": 1
            }
        },

        "masking_zones": 
        {
            "0":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0,   "j": 0},
                    "p1": {"i": 0,   "j": 320},
                    "p2": {"i": 200, "j": 320},
                    "p3": {"i": 200, "j": 0}
                }
            },
            "1":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            },
            "2":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            },
            "3":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            },
            "4":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            },
            "5":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            },
            "6":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            },
            "7":
            {
                "attributes": 1,
                "minimal_range": 0,
                "region_of_interests":
                {
                    "p0": {"i": 0, "j": 0},
                    "p1": {"i": 0, "j": 0},
                    "p2": {"i": 0, "j": 0},
                    "p3": {"i": 0, "j": 0}
                }
            }
        },

        "environment": 
        {
            "safety_trigger_duration" : 1,
            "linear_velocity" : 0,
            "angular_velocity" : 0,
            "payload_weight" : 0,
            "surface_inclination" : 15,
            "surface_height" : 0.05,
            "diagnostic_zone_fill_rate_threshold" : 255,
            "floor_fill_threshold" : 255,
            "depth_fill_threshold" : 255,
            "diagnostic_zone_height_median_threshold" : 255,
            "vision_hara_persistency" : 1,
            "crypto_signature" : [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        }
    }
}
"""

# Remove new-lines and spaces from JSON string (facing encoding issues without these lines)
sp_json_str = ''.join(line.strip() for line in sp_json_str.split('\n'))
sp_json_str = sp_json_str.replace(' ', '')

# Convert json string to Safety Preset struct
sp = safety_sensor.json_string_to_safety_preset(sp_json_str)

# write the above sp table to the device
safety_sensor.set_safety_preset(index, sp)

# read the table from the device
read_result = safety_sensor.get_safety_preset(index)

# verify the tables are equal
test.check_equal(read_result, sp)

# verify the JSON objects are equal (comparing JSON object because
# the JSON string can have different order of inner fields
read_result_json_str = safety_sensor.safety_preset_to_json_string(read_result)
sp_json_str = safety_sensor.safety_preset_to_json_string(sp)
test.check_equal(json.loads(sp_json_str), json.loads(read_result_json_str))

# restore original table
safety_sensor.set_safety_preset(index, previous_result)
test.finish()

#############################################################################################

# switch back to Run safety mode
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.run)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.run))
test.print_results_and_exit()
