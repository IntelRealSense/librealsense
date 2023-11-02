# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import test


test.start("Testing USB type can be detected")

dev = test.find_first_device_or_exit()

test.check(dev.supports(rs.camera_info.usb_type_descriptor))

test.finish()
test.print_results_and_exit()
