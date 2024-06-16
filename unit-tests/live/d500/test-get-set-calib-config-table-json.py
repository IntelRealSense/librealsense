# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import json

#############################################################################################
# Tests
#############################################################################################

test.start("Verify auto calibrated device extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
ac_dev = dev.as_auto_calibrated_device()
test.finish()

#############################################################################################

test.start("Writing calibration config, then reading and comparing calibration config JSONs")

# save current calibration config in order to restore it at the end
previous_result = ac_dev.get_calibration_config()

# calibration config JSON String to be written
calib_config_json_str = '''
{
    "calibration_config":
    {
        "calib_roi_num_of_segments": 0,
        "roi": [ [[0,0], [0,0], [0,0], [0,0]], [[0,0], [0,0], [0,0], [0,0]], [[0,0], [0,0], [0,0], [0,0]], [[0,0], [0,0], [0,0], [0,0]] ],
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
        "crypto_signature": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    }
}
'''

# generate calibration config struct from JSON string
new_calib_config = ac_dev.json_string_to_calibration_config(calib_config_json_str)

# write the above new_calib_config table to the device
ac_dev.set_calibration_config(new_calib_config)

# read the current calib config table from the device
read_result = ac_dev.get_calibration_config()

# verify the tables are equal
test.check_equal(read_result, new_calib_config)

# verify the JSON objects are equal (comparing JSON object because
# the JSON string can have different order of inner fields
read_result_json_str = ac_dev.calibration_config_to_json_string(read_result)
new_calib_config_json_str = ac_dev.calibration_config_to_json_string(new_calib_config)
test.check_equal(json.loads(new_calib_config_json_str), json.loads(read_result_json_str))

# restore original calibration config table
ac_dev.set_calibration_config(previous_result)

test.finish()

#############################################################################################

test.print_results_and_exit()
