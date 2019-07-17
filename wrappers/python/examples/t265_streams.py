#!/usr/bin/python
# -*- coding: utf-8 -*-
## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#####################################################
##           librealsense T265 streams             ##
#####################################################

# First import the library
import pyrealsense2 as rs

import cv2
import numpy as np

# Declare RealSense pipeline, encapsulating the actual device and sensors
pipe = rs.pipeline()

# Build config object and request pose data
cfg = rs.config()
cfg.enable_stream(rs.stream.pose)
cfg.enable_stream(rs.stream.fisheye, 1)
cfg.enable_stream(rs.stream.fisheye, 2)

# Start streaming with requested config
pipe.start(cfg)

try:
    WINDOW_TITLE = 'T265 Streams'
    fs_count = 0

    while(True):
        # Wait for the next set of frames from the camera
        frames = pipe.wait_for_frames()
        f1 = frames.get_fisheye_frame(1).as_video_frame()
        left_data = np.asanyarray(f1.get_data())
        f2 = frames.get_fisheye_frame(2).as_video_frame()
        right_data = np.asanyarray(f2.get_data())
        # Stack both images horizontally
        images = np.hstack((left_data, right_data))
        cv2.imshow(WINDOW_TITLE, images)

        pose = frames.get_pose_frame()
        if pose:
            # Print some of the pose data to the terminal
            data = pose.get_pose_data()
            print("Frame #{}".format(pose.frame_number))
            print("Position: {}".format(data.translation))
            print("Velocity: {}".format(data.velocity))
            print("Acceleration: {}".format(data.acceleration))
            print("Confidence: {}\n".format(data.tracker_confidence))

        fs_count = fs_count + 1
        key = cv2.waitKey(1)
        if key == ord('q'):
            break

finally:
    pipe.stop()

