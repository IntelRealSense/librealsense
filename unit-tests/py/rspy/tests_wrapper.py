# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# Some future models might need to wrap the tests in setup and teardown steps.
# Don't remove this file even if current implementation is empty...

import pyrealsense2 as rs

# Many operations, such as setting options, can take place only in safety service mode
def start_wrapper( dev = None ):
    return

def stop_wrapper( dev = None ):
    return
