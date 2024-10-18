# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device:jetson D457
# test:donotrun:!jetson 

import pyrealsense2 as rs
from rspy import test
import time
gyro_frame_count = 0
accel_frame_count = 0

def frame_callback( f ):
    global gyro_frame_count,accel_frame_count
    stream_type = f.get_profile().stream_type()
    if stream_type == rs.stream.gyro:
        gyro_frame_count += 1
        test.check_equal(f.get_frame_number(),gyro_frame_count)
    elif stream_type == rs.stream.accel:
        accel_frame_count += 1
        test.check_equal(f.get_frame_number(), accel_frame_count)

################################################################################################
with test.closure("frame index - mipi IMU "):
    seconds_to_count_frames = 10
    dev, _ = test.find_first_device_or_exit()
    sensor = dev.first_motion_sensor()
    motion_profile_accel = next(p for p in sensor.profiles if p.stream_type() == rs.stream.accel and p.fps() == 100)
    motion_profile_gyro = next(p for p in sensor.profiles if p.stream_type() == rs.stream.gyro and p.fps()==100)
    sensor.open( [motion_profile_accel, motion_profile_gyro] )
    sensor.start( frame_callback )
    time.sleep(seconds_to_count_frames) # Time to count frames
    sensor.stop()
    sensor.close()
    test.check(gyro_frame_count > 0)
    test.check(accel_frame_count > 0)

test.print_results_and_exit()