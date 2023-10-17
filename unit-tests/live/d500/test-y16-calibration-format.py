# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D500*
# This test checks streaming y16 profile

import time
import pyrealsense2 as rs
from rspy import test, log
from rspy.timer import Timer
from safety.safety_common import set_operational_mode

y16_streamed = False


def close_resources(sensor):
    """
    Stop and Close sensor.
    :sensor: sensor of device
    """
    if len(sensor.get_active_streams()) > 0:
        sensor.stop()
        sensor.close()


def frame_callback(frame):
    global y16_streamed
    y16_streamed = True


timer = Timer(5)

device = test.find_first_device_or_exit()
safety_sensor = device.first_safety_sensor()
depth_sensor = device.first_depth_sensor()


test.start("Switch to Service Mode")
original_mode = safety_sensor.get_option(rs.option.safety_mode)
test.check(set_operational_mode(safety_sensor, rs.safety_mode.service))
test.finish()

test.start('Check that y16 is streaming:')

profile_y16 = next(p for p in depth_sensor.profiles if p.format() == rs.format.y16)
test.check(profile_y16)
log.d(str(profile_y16))

if profile_y16:
    depth_sensor.open(profile_y16)
    depth_sensor.start(frame_callback)

    timer.start()
    while not timer.has_expired():
        if y16_streamed:
            break
        time.sleep(0.1)

    test.check(not timer.has_expired())

close_resources(depth_sensor)
test.finish()

test.start("Restoring original safety mode")
test.check(set_operational_mode(safety_sensor, original_mode))
test.finish()

test.print_results_and_exit()
