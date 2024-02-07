# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device D400*

# Note, this test depends on files deployed on LibCI machines (Windows + Linux)!

import pyrealsense2 as rs
from rspy import test, libci
import os

fw_dir = os.path.join( libci.home, 'data/FW', '' )
d400_fw_min_version_1 = 'Signed_Image_UVC_5_8_15_0.bin'
d400_fw_min_version_2 = 'Signed_Image_UVC_5_12_7_100.bin'
d400_fw_min_version_3 = 'Signed_Image_UVC_5_12_12_100.bin'
d400_fw_min_version_4 = 'Signed_Image_UVC_5_13_0_50.bin'
d400_fw_min_version_1_prev = 'Signed_Image_UVC_5_8_14_0.bin'
d400_fw_min_version_2_prev = 'Signed_Image_UVC_5_12_6_0.bin'
d400_fw_min_version_3_prev = 'Signed_Image_UVC_5_12_11_0.bin'
d400_fw_min_version_4_prev = 'Signed_Image_UVC_5_12_15_150.bin'

pid_to_min_fw_version = {  # D400 product line:
    '0AD1': d400_fw_min_version_1,  # D400
    '0AD2': d400_fw_min_version_1,  # D410
    '0AD3': d400_fw_min_version_1,  # D415
    '0AD4': d400_fw_min_version_1,  # D430
    '0AD5': d400_fw_min_version_1,  # D430_MM
    '0AD6': d400_fw_min_version_1,  # USB2
    '0ADB': d400_fw_min_version_1,  # RECOVERY
    '0ADC': d400_fw_min_version_1,  # USB2_RECOVERY
    '0AF2': d400_fw_min_version_1,  # D400_IMU
    '0AF6': d400_fw_min_version_1,  # D420
    '0AFE': d400_fw_min_version_1,  # D420_MM
    '0AFF': d400_fw_min_version_1,  # D410_MM
    '0B00': d400_fw_min_version_1,  # D400_MM
    '0B01': d400_fw_min_version_1,  # D430_MM_RGB
    '0B03': d400_fw_min_version_1,  # D460
    '0B07': d400_fw_min_version_1,  # D435
    '0B0C': d400_fw_min_version_1,  # D405U
    '0B3A': d400_fw_min_version_2,  # D435I
    '0B49': d400_fw_min_version_1,  # D416
    '0B4B': d400_fw_min_version_1,  # D430I
    '0B52': d400_fw_min_version_1,  # D416_RGB
    '0B5B': d400_fw_min_version_3,  # D405
    '0B5C': d400_fw_min_version_4   # D455
}

pid_to_max_fw_version = {
}

fw_previous_version = {d400_fw_min_version_1: d400_fw_min_version_1_prev,
                       d400_fw_min_version_2: d400_fw_min_version_2_prev,
                       d400_fw_min_version_3: d400_fw_min_version_3_prev,
                       d400_fw_min_version_4: d400_fw_min_version_4_prev
                       }

fw_next_version = {
}

def check_firmware_not_compatible(updatable_device, fw_image):
    test.check(not updatable_device.check_firmware_compatibility(fw_image))


def check_firmware_compatible(updatable_device, fw_image):
    test.check(updatable_device.check_firmware_compatibility(fw_image))

def get_fw_version_path(product_line_dir, fw_version):
    return fw_dir + product_line_dir + fw_version

ctx = rs.context()
dev = ctx.query_devices()[0]
updatable_device = dev.as_updatable()
product_line_dir = dev.get_info(rs.camera_info.product_line) + '/'

#############################################################################################
test.start("checking firmware compatibility with device")
# test scenario:
# get min fw for device, check compatibility, check one before is not compatible
# get max fw for device, check compatibility, check one after is not compatible
# skip any case that is not applicable
pid = dev.get_info(rs.camera_info.product_id)
print(dev.get_info(rs.camera_info.name) + " found")

if pid in pid_to_min_fw_version:
    min_fw_version = pid_to_min_fw_version[pid]
    min_fw_version_path = get_fw_version_path(product_line_dir, min_fw_version)
    print("fw min version: " + min_fw_version)
    with open(min_fw_version_path, 'rb') as binary_file:
        fw_image = bytearray(binary_file.read())
        check_firmware_compatible(updatable_device, fw_image)

    # Negative
    if min_fw_version in fw_previous_version:
        one_before_min_fw_version = fw_previous_version[min_fw_version]
        one_before_min_fw_version_path = get_fw_version_path(product_line_dir, one_before_min_fw_version)
        print("firware version defined as non-compatible: " + one_before_min_fw_version)
        with open(one_before_min_fw_version_path, 'rb') as binary_file:
            fw_image = bytearray(binary_file.read())
            check_firmware_not_compatible(updatable_device, fw_image)
    else:
        print("no previous version found")
else:
    print("No min fw version found")

if pid in pid_to_max_fw_version:
    max_fw_version = pid_to_max_fw_version[pid]
    max_fw_version_path = get_fw_version_path(product_line_dir, max_fw_version)
    print("fw max version: " + max_fw_version)
    with open(max_fw_version_path, 'rb') as binary_file:
        fw_image = bytearray(binary_file.read())
        check_firmware_compatible(updatable_device, fw_image)

    if max_fw_version in fw_next_version:
        one_after_max_fw_version = fw_next_version[max_fw_version]
        one_after_max_fw_version_path = get_fw_version_path(product_line_dir, one_after_max_fw_version)
        print("fw max version: " + max_fw_version + ", one after: " + one_after_max_fw_version)
        with open(one_after_max_fw_version_path, 'rb') as binary_file:
            fw_image = bytearray(binary_file.read())
            check_firmware_not_compatible(updatable_device, fw_image)
    else:
        print("No next fw version found")
else:
    print("No max fw version found")

test.finish()
test.print_results_and_exit()
