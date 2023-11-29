# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D400*
# This test check that A Factor of Disparity can be changed

import pyrealsense2 as rs
from rspy import test, log


def close_resources(sensor):
    """
    Stop and Close sensor.
    :sensor: sensor of device
    """
    if len(sensor.get_active_streams()) > 0:
        sensor.stop()
        sensor.close()


def test_amp_factor(am_device, a_factor: float):
    """

    """
    factor = am_device.get_amp_factor()
    factor.a_factor = a_factor
    am_device.set_amp_factor(factor)

    log.d('Checking A factor: ' + str(a_factor))
    test.check(am_device.get_amp_factor().a_factor - a_factor < 0.01)


device = test.find_first_device_or_exit()
depth_sensor = device.first_depth_sensor()
advance_mode_device = rs.rs400_advanced_mode(device)

if depth_sensor and advance_mode_device:

    depth_profile_depth = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth)
    depth_profile_infrared = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.infrared)

    test.start('Check that Disparity modulation receive values:')
    test_amp_factor(advance_mode_device, 0.05)
    test_amp_factor(advance_mode_device, 0.1)
    test_amp_factor(advance_mode_device, 0.15)
    test_amp_factor(advance_mode_device, 0.2)
    test_amp_factor(advance_mode_device, 0.0)

close_resources(depth_sensor)
test.finish()
test.print_results_and_exit()
