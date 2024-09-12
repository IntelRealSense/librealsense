# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device each(D400*)
#test:priority 1
#test:flag windows

import pyrealsense2 as rs
from rspy import test, log

test.start("checking metadata is enabled")
dev = test.find_first_device_or_exit()
test.check( dev.is_metadata_enabled() )

test.finish()
test.print_results_and_exit()
