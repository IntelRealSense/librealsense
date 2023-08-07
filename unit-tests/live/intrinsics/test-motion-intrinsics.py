# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy import test


device = test.find_first_device_or_exit()
motion_sensor = device.first_motion_sensor()

if motion_sensor:

    motion_profile_accel = next(p for p in motion_sensor.profiles if p.stream_type() == rs.stream.accel)
    if motion_profile_accel:
        motion_profile_accel = motion_profile_accel.as_motion_stream_profile()
        intrinsics_accel = motion_profile_accel.get_motion_intrinsics()

        # Test #1
        test.start('Check intrinsics in motion sensor - ACCEL profile')
        test.check(len(str(intrinsics_accel)) > 0)
        test.finish()

    motion_profile_gyro = next(p for p in motion_sensor.profiles if p.stream_type() == rs.stream.gyro)
    if motion_profile_gyro:
        motion_profile_gyro = motion_profile_gyro.as_motion_stream_profile()
        intrinsics_gyro = motion_profile_gyro.get_motion_intrinsics()

        # Test #2
        test.start('Check intrinsics in motion sensor - GYRO profile')
        test.check(len(str(intrinsics_gyro)) > 0)
        test.finish()
