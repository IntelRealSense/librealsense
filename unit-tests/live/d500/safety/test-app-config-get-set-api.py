# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D585S

from rspy import test, log
from rspy import tests_wrapper as tw

import json

app_config_json_str = '''
{
	"application_config":
	{
		"sip":
		{
			"immediate_mode_safety_features_selection": 0,
			"temporal_safety_features_selection": 0,
			"mechanisms_thresholds": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
			"mechanisms_sampling_interval": [0, 0, 0, 0, 0, 0, 0, 0],
			"tc_consecutives_failures_threshold": 0
		},
		"dev_rules_selection": 0,
		"depth_pipe_safety_checks_override": 0,
		"triggered_calib_safety_checks_override": 0,
		"smcu_bypass_directly_to_maintenance_mode": 0,
		"smcu_skip_spi_error": 0,
		"temp_thresholds":
		{
		    "ir_right": [0, 0, 0, 0],
			"ir_left": [0, 0, 0, 0],
			"apm_left": [0, 0, 0, 0],
			"apm_right": [0, 0, 0, 0],
			"hkr_core": [0, 0, 0, 0],
			"smcu_right": [0, 0, 0, 0],
			"sht4x": [0, 0, 0, 0],
			"imu": [0, 0, 0, 0]
		},
		"sht4x_humidity_threshold": 23,
		"voltage_thresholds":
		{
			"vdd3v3": 0,
			"vdd1v8": 0,
			"vdd1v2": 0,
			"vdd1v1": 0,
			"vdd0v8": 0,
			"vdd0v6": 0,
			"vdd5vo_u": 0,
			"vdd5vo_l": 0,
			"vdd0v8_ddr": 0
		},
		"developer_mode":
		{
			"hkr": 0,
			"smcu": 0,
			"hkr_simulated_lock_state": 0,
			"sc": 0
		},
        "depth_pipeline_config": 0,
        "depth_roi": 0,
        "ir_for_sip": 0,
        "peripherals_sensors_disable_mask": 0,
        "digital_signature": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
	}
}
'''

#############################################################################################
# Tests
#############################################################################################

dev, _ = test.find_first_device_or_exit()
safety_sensor = dev.first_safety_sensor()
tw.start_wrapper(dev)

#############################################################################################
test.start("get app config table")
original_app_config_as_json_str = safety_sensor.get_application_config()
log.d("app config successfully downloaded")
test.finish()

#############################################################################################
test.start("set app config table, and check its writing")
safety_sensor.set_application_config(app_config_json_str)
log.d("app config successfully uploaded")
# checking the app config table now in device is equal to the one uploaded
curr_config_table = safety_sensor.get_application_config()
test.check_equal(json.loads(curr_config_table), json.loads(app_config_json_str))
test.finish()

#############################################################################################
test.start("restoring config table")
safety_sensor.set_application_config(original_app_config_as_json_str)
log.d("app config successfully uploaded")
# checking the app config table now in device is equal to the one uploaded
curr_config_table = safety_sensor.get_application_config()
test.check_equal(json.loads(curr_config_table), json.loads(original_app_config_as_json_str))
test.finish()

#############################################################################################
tw.stop_wrapper(dev)
test.print_results_and_exit()
