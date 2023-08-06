# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
import time

# test:device each(D400*)

import pyrealsense2 as rs
from rspy import test


def close_resources(s):
    """
    Stop and close a sensor
    :param s: sensor of device
    """
    if len(s.get_active_streams()) > 0:
        s.stop()
        s.close()


print('f')
queue_capacity = 1
frame_queue = None
device = test.find_first_device_or_exit()

frame_queue = rs.frame_queue(queue_capacity, keep_frames=False)

sensor = device.first_motion_sensor()
profile = next(p for p in sensor.profiles if p.stream_type() == rs.stream.gyro or p.stream_type() == rs.stream.accel)

sensor.open(profile)
frame = frame_queue.wait_for_frame()

if not frame:
    print('not frame')
else:
    print('has frame')

close_resources(sensor)
