# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device L500*
# test:device D400*

import pyrealsense2 as rs
from rspy import test


def check_firmware_not_compatible(updatable_device, old_fw_image):
    test.check(not updatable_device.check_firmware_compatibility(old_fw_image))

def check_firmware_compatible(updatable_device, old_fw_image):
    test.check(updatable_device.check_firmware_compatibility(old_fw_image))


ctx = rs.context()
dev = ctx.query_devices()[0]
updatable_device = dev.as_updatable()

#############################################################################################
test.start("checking firmware compatibility with device")
pid = dev.get_info(rs.camera_info.product_id)
if pid == '0B3A':  # D435I
    print("device D435i found")
    with open("Signed_Image_UVC_5_12_6_0.bin", 'rb') as binary_file:
        old_fw_image = bytearray(binary_file.read())
        check_firmware_not_compatible(updatable_device, old_fw_image)
    with open("Signed_Image_UVC_5_12_7_100.bin", 'rb') as binary_file:
        old_fw_image = bytearray(binary_file.read())
        check_firmware_compatible(updatable_device, old_fw_image)
elif pid == '0B07':  # D435
    print("device D435 found")
    with open("Signed_Image_UVC_5_8_14_0.bin", 'rb') as binary_file:
        old_fw_image = bytearray(binary_file.read())
        check_firmware_not_compatible(updatable_device, old_fw_image)
    with open("Signed_Image_UVC_5_8_15_0.bin", 'rb') as binary_file:
        old_fw_image = bytearray(binary_file.read())
        check_firmware_compatible(updatable_device, old_fw_image)
elif pid == '0B64':  # L515
    print("device L515 found")
    with open("Signed_Image_UVC_1_4_0_10.bin", 'rb') as binary_file:
        old_fw_image = bytearray(binary_file.read())
        check_firmware_not_compatible(updatable_device, old_fw_image)
    with open("Signed_Image_UVC_1_4_1_0.bin", 'rb') as binary_file:
        old_fw_image = bytearray(binary_file.read())
        check_firmware_compatible(updatable_device, old_fw_image)

test.finish()
test.print_results_and_exit()
