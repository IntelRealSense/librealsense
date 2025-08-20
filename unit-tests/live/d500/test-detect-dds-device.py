# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 RealSense, Inc. All Rights Reserved.

#test:donotrun:!dds
#test:device D555

import pyrealsense2 as rs
from rspy import test, log

# this test will query for device, test check it is DDS and exit

with test.closure("Detect DDS device"):
    dev, _ = test.find_first_device_or_exit()
    if test.check(dev.supports(rs.camera_info.connection_type)):
        connection_type = dev.get_info(rs.camera_info.connection_type)
        test.check(connection_type == "DDS")

test.print_results_and_exit()
