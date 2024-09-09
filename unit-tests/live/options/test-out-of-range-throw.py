# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test
from rspy import tests_wrapper as tw


def check_min_max_throw(sensor):
    options_to_check = [rs.option.exposure, rs.option.enable_auto_exposure]
    for option in options_to_check:
        if not sensor.supports(option):
            continue
        option_range = sensor.get_option_range(option)
        # below min
        test.check_throws(lambda: sensor.set_option(option, option_range.min - 1), RuntimeError,
                          "out of range value for argument \"value\"")
        # above max
        test.check_throws(lambda: sensor.set_option(option, option_range.max + 1), RuntimeError,
                          "out of range value for argument \"value\"")


dev, _ = test.find_first_device_or_exit()
tw.start_wrapper(dev)
with test.closure("Options out of Range throwing exception"):
    sensors = dev.query_sensors()
    for sensor in sensors:
        check_min_max_throw(sensor)

tw.stop_wrapper(dev)

test.print_results_and_exit()
