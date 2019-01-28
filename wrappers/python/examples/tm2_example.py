#!/usr/bin/python
# -*- coding: utf-8 -*-
## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#####################################################
##            librealsense TM2 example             ##
#####################################################

# First import the library
import pyrealsense2 as rs

# Importing json to help with notifications
import json
from time import sleep

def on_frame(f):
    if f.profile.stream_type() == rs.stream.pose:
        data = f.as_pose_frame().get_pose_data()
        print("Frame #{}".format(f.frame_number))
        print("Position: {}".format(data.translation))
        print("Velocity: {}".format(data.velocity))
        print("Acceleration: {}\n".format(data.acceleration))

try:
    # Create a context
    ctx = rs.context()

    # Wait until a TM2 device connects
    # We have to wait here, even if the device is already connected (since the library loads the device as usb device)
    print ('Waiting for all devices to connect...')
    found = False
    while not found:
        if len(ctx.devices) > 0:
            for d in ctx.devices:
                if d.get_info(rs.camera_info.product_id) in [str(0x0b37), str(0x0af3)]:
                    tm2 = d.as_tm2()
                    print ('Found TM2 device: ', \
                        d.get_info(rs.camera_info.name), ' ', \
                        d.get_info(rs.camera_info.serial_number))
                    found = True

    # Get the sensor from the device
    s = tm2.sensors[0]

    # Start the sensor
    s.open(s.profiles)
    s.start(on_frame)

    # Sleep for a while
    sleep(5)

    # Stop the sensor
    s.stop()
    s.close()
except:
    print ("Error while running example")