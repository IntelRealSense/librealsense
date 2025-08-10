# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.

# test:device D500*
# This test checks streaming y16 profile

import time
import pyrealsense2 as rs
from rspy import test, log
from rspy.timer import Timer
from rspy import tests_wrapper as tw

y16_streamed = False


def close_resources(sensor):
    """
    Stop and Close sensor.
    :sensor: sensor of device
    """
    if len(sensor.get_active_streams()) > 0:
        log.d("Close_resources: Stopping active streams")
        sensor.stop()
        sensor.close()


def frame_callback(frame):
    frame_profile = frame.get_profile()
    if frame_profile.format() == rs.format.y16:
        global y16_streamed
        y16_streamed = True


timer = Timer(5)

device, _ = test.find_first_device_or_exit()
safety_sensor = device.first_safety_sensor()
depth_sensor = device.first_depth_sensor()

tw.start_wrapper(device)

#########################################################################
test.start('Check that y16 is streaming')

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

    test.check( not timer.has_expired() )
    test.check( y16_streamed )
    close_resources(depth_sensor)

test.finish()

###########################################################################
tw.stop_wrapper(device)
test.print_results_and_exit()
