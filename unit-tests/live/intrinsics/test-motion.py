# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device:jetson D457
# test:device:!jetson D455
# This test check existence motion intrinsic data in accel and gyro profiles.

import pyrealsense2 as rs
from rspy import test, log

device = test.find_first_device_or_exit()
motion_sensor = device.first_motion_sensor()

test.start('Check intrinsics in motion sensor:')

test.check(motion_sensor)

if motion_sensor:

    motion_profile_accel = next(p for p in motion_sensor.profiles if p.stream_type() == rs.stream.accel)
    motion_profile_gyro = next(p for p in motion_sensor.profiles if p.stream_type() == rs.stream.gyro)

    test.check(motion_profile_accel and motion_profile_gyro)

    if motion_profile_accel:
        motion_profile_accel = motion_profile_accel.as_motion_stream_profile()
        intrinsics_accel = motion_profile_accel.get_motion_intrinsics()

        log.d(str(intrinsics_accel))
        test.check(len(str(intrinsics_accel)) > 0)  # Checking if intrinsics has data

    if motion_profile_gyro:
        motion_profile_gyro = motion_profile_gyro.as_motion_stream_profile()
        intrinsics_gyro = motion_profile_gyro.get_motion_intrinsics()

        log.d(intrinsics_gyro)
        test.check(len(str(intrinsics_gyro)) > 0)  # Checking if intrinsics has data

    test.finish()
    test.print_results_and_exit()
