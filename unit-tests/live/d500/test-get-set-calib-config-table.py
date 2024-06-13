# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:donotrun

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
    calib_config.reserved1 = [3] * 12
    calib_config.camera_position = generate_camera_position()
    calib_config.reserved2 = [0] * 300
    calib_config.crypto_signature = [0] * 32
    calib_config.reserved3 = [0] * 39
    return calib_config


def is_equal_roi(first, second):
    for i in range(0, 4):
        for j in range(0, 4):
            for k in range(0, 2):
                if first[i].mask_pixel[j][k] != second[i].mask_pixel[j][k]:
                    return False
    return True


def is_float3_equal(first, second):
    return first.x == second.x and first.y == second.y and first.z == second.z


def is_float3x3_equal(first, second):
    return (is_float3_equal(first.x, second.x) and is_float3_equal(first.y, second.y)
            and is_float3_equal(first.z, second.z))


def is_equal_extrinsics_table(first, second):
    if not is_float3x3_equal(first.rotation, second.rotation):
        return False
    if not is_float3_equal(first.translation, second.translation):
        return False
    return True


def is_equal_calib_configs(first, second):
    if first.calib_roi_num_of_segments != second.calib_roi_num_of_segments:
        print("calib_roi_num_of_segments is different")
        return False
    if not is_equal_roi(first.roi, second.roi):
        print("roi is different")
        return False
    if first.reserved1 != second.reserved1:
        print("reserved1 is different")
        return False
    if not is_equal_extrinsics_table(first.camera_position, second.camera_position):
        print("camera_position is different")
        return False
    if first.reserved2 != second.reserved2:
        print("calib_roi_num_of_segments is different")
        return False
    if first.crypto_signature != second.crypto_signature:
        print("crypto_signature is different")
        return False
    if first.reserved3 != second.reserved3:
        print("reserved3 is different")
        return False
    return True


dev = test.find_first_device_or_exit()
ac_dev = dev.as_auto_calibrated_device()

#############################################################################################
with test.closure("Set / Get calib config table"):
    generated_calib_config = generate_calib_config_table()
    ac_dev.set_calibration_config(generated_calib_config)
    current_calib_config = ac_dev.get_calibration_config()
    test.check(is_equal_calib_configs(generated_calib_config, current_calib_config))

#############################################################################################

test.print_results_and_exit()
