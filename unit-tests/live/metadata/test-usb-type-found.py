# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#test:device each(D400*) !D457


import pyrealsense2 as rs
from rspy import test


test.start("Testing USB type can be detected")

dev, _ = test.find_first_device_or_exit()

supports = dev.supports(rs.camera_info.usb_type_descriptor)
test.check(supports)
if supports:
    usb_type = dev.get_info(rs.camera_info.usb_type_descriptor)
    test.check(usb_type and not usb_type == "Undefined")

test.finish()
test.print_results_and_exit()
