# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
import time

import pyrealsense2 as rs
from rspy.timer import Timer

MAX_TIME_TO_WAIT_FOR_OP_MODE = 2  # [sec]


def set_operational_mode(safety_sensor, required_mode, max_time=MAX_TIME_TO_WAIT_FOR_OP_MODE):
    wait_for_op_mode_timer = Timer(max_time)
    safety_sensor.set_option(rs.option.safety_mode, required_mode)
    wait_for_op_mode_timer.start()
    while not wait_for_op_mode_timer.has_expired():
        safety_op_mode = safety_sensor.get_option(rs.option.safety_mode)
        if safety_op_mode == required_mode:
            break
        time.sleep(0.1)
    return not wait_for_op_mode_timer.has_expired() and safety_op_mode == required_mode
