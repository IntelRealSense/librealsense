#!/usr/bin/env python
# -*- coding: utf-8 -*-
## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.

##############################################
## librealsense T265 wheel odometry example ##
##############################################
import pyrealsense2 as rs

# before start: get device by cfg.resolve(pipe)
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


    # start
    pipe.start()
    for _ in range(100):
        frames = pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            data = pose.get_pose_data()
            print("Frame #{}".format(pose.frame_number))
            print("Position: {}".format(data.translation))

            # send wheel odometry measurement
            v = rs.vector()
            v.x = 0.1
            wheel_odometer.send_wheel_odometry(0,0,v)
    pipe.stop()
