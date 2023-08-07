# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D455
# test:device D457

import pyrealsense2 as rs
from rspy import test, log


device = test.find_first_device_or_exit()
motion_sensor = device.first_motion_sensor()

# Test #1
test.start('Checking for the existence of motion sensor:')
test.check(motion_sensor)
test.finish()


if motion_sensor:

    motion_profile_accel = next(p for p in motion_sensor.profiles if p.stream_type() == rs.stream.accel)

    # Test #2
    test.start('Checking for the existence of motion ACCEL profile:')
    test.check(motion_profile_accel)
    test.finish()

    if motion_profile_accel:
        motion_profile_accel = motion_profile_accel.as_motion_stream_profile()
        intrinsics_accel = motion_profile_accel.get_motion_intrinsics()

        # Test #3
        test.start('Check intrinsics in motion sensor - ACCEL profile:')
        test.check(len(str(intrinsics_accel)) > 0)  # Checking if intrinsics has data
        log.d(str(intrinsics_accel))
        test.finish(str(intrinsics_accel))

    motion_profile_gyro = next(p for p in motion_sensor.profiles if p.stream_type() == rs.stream.gyro)

    # Test #4
    test.start('Checking for the existence of motion GYRO profile:')
    test.check(motion_profile_gyro)
    test.finish()

    if motion_profile_gyro:
        motion_profile_gyro = motion_profile_gyro.as_motion_stream_profile()
        intrinsics_gyro = motion_profile_gyro.get_motion_intrinsics()

        # Test #5
        test.start('Check intrinsics in motion sensor - GYRO profile:')
        test.check(len(str(intrinsics_gyro)) > 0)  # Checking if intrinsics has data
        log.d(intrinsics_gyro)
        test.finish()
