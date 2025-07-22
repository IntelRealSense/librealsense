# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D400*
# This test checks that A Factor of Disparity can be changed

import pyrealsense2 as rs
from rspy import test, log


def test_amp_factor(am_device, input_factor_values: list):
    """
    This function set new A Factor value to advance mode device
    :am_device: advance mode device
    :input_factor_values: list of A Factor values
    """
    amp_factor = am_device.get_amp_factor()
    output_factor_values = []

    for factor_value in input_factor_values:
        amp_factor.a_factor = factor_value
        am_device.set_amp_factor(amp_factor)
        output_factor_values.append(am_device.get_amp_factor().a_factor)

    test.check_float_lists(input_factor_values, output_factor_values)


device, _ = test.find_first_device_or_exit()
advance_mode_device = rs.rs400_advanced_mode(device)

with test.closure('Verify set/get of Disparity modulation'):
    if advance_mode_device:
        a_factor_values = [0.05, 0.01]
        test_amp_factor(advance_mode_device, a_factor_values)
    else:
        log.d('Depth sensor or advanced mode not found.')
        test.fail()


test.print_results_and_exit()
