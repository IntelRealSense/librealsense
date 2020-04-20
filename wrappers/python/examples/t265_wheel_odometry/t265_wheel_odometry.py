#!/usr/bin/env python
# -*- coding: utf-8 -*-
## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.
from __future__ import print_function

"""
This example shows how to fuse wheel odometry measurements (in the form of 3D translational velocity measurements) on the T265 tracking camera to use them together with the (internal) visual and intertial measurements.
This functionality makes use of two API calls:
1. Configuring the wheel odometry by providing a json calibration file (in the format of the accompanying calibration file)
Please refer to the description of the calibration file format here: https://github.com/IntelRealSense/librealsense/blob/master/doc/t265.md#wheel-odometry-calibration-file-format.
2. Sending wheel odometry measurements (for every measurement) to the camera

Expected output:
For a static camera, the pose output is expected to move in the direction of the (artificial) wheel odometry measurements (taking into account the extrinsics in the calibration file).
The measurements are given a high weight/confidence, i.e. low measurement noise covariance, in the calibration file to make the effect visible.
If the camera is partially occluded the effect will be even more visible (also for a smaller wheel odometry confidence / higher measurement noise covariance) because of the lack of visual feedback. Please note that if the camera is *fully* occluded the pose estimation will switch to 3DOF, estimate only orientation, and prevent any changes in the position.
"""

import pyrealsense2 as rs

# load wheel odometry config before pipe.start(...)
# get profile/device/ wheel odometry sensor by profile = cfg.resolve(pipe)
pipe = rs.pipeline()
cfg = rs.config()
profile = cfg.resolve(pipe)
dev = profile.get_device()
tm2 = dev.as_tm2()

if(tm2):
    # tm2.first_wheel_odometer()?
    pose_sensor = tm2.first_pose_sensor()
    wheel_odometer = pose_sensor.as_wheel_odometer()

    # calibration to list of uint8
    f = open("calibration_odometry.json")
    chars = []
    for line in f:
       for c in line:
           chars.append(ord(c))  # char to uint8

    # load/configure wheel odometer
    wheel_odometer.load_wheel_odometery_config(chars)


    pipe.start()
    try:
        for _ in range(100):
            frames = pipe.wait_for_frames()
            pose = frames.get_pose_frame()
            if pose:
                data = pose.get_pose_data()
                print("Frame #{}".format(pose.frame_number))
                print("Position: {}".format(data.translation))

                # provide wheel odometry as vecocity measurement
                wo_sensor_id = 0  # indexed from 0, match to order in calibration file
                frame_num = 0  # not used
                v = rs.vector()
                v.x = 0.1  # m/s
                wheel_odometer.send_wheel_odometry(wo_sensor_id, frame_num, v)
    finally:
        pipe.stop()
