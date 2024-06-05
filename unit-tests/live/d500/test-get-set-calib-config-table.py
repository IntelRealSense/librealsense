# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device D500*

import pyrealsense2 as rs
from rspy import test, log


def generate_camera_position():
    # rotation matrix
    rx = rs.float3(0.0, 0.0, 1.0)
    ry = rs.float3(-1.0, 0.0, 0.0)
    rz = rs.float3(0.0, -1.0, 0.0)
    rotation = rs.float3x3(rx, ry, rz)
    # translation vector [m]
    translation = rs.float3(0.0, 0.0, 0.27)
    return rs.extrinsics_table(rotation, translation)


def generate_calib_config_table():
    calib_roi = rs.calibration_roi(0, 0, 0, 0, 0, 0, 0, 0)
    calib_config = rs.calibration_config()
    calib_config.calib_roi_num_of_segments = 0
    calib_config.roi = [calib_roi, calib_roi, calib_roi, calib_roi]
    calib_config.reserved1 = [0] * 12
    calib_config.camera_position = generate_camera_position()
    calib_config.reserved2 = [0] * 300
    calib_config.crypto_signature = [0] * 32
    calib_config.reserved3 = [0] * 39
    return calib_config


dev = test.find_first_device_or_exit()
ac_dev = dev.as_auto_calibrated_device()

#############################################################################################
with test.closure("Set / Get calib config table"):
    generated_calib_config = generate_calib_config_table()
    ac_dev.set_calibration_config(generated_calib_config)
    current_calib_config = ac_dev.get_calibration_config()
    test.check_equal(generated_calib_config, current_calib_config)

#############################################################################################

test.print_results_and_exit()