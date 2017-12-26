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

def on_notification(n):
    print n
    if n.category == rs.notification_category.hardware_event:
        try:
            event = json.loads(n.serialized_data)
            if event and event['Event Type'] == 'Controller Event' \
                and event['Data']['Sub Type'] == 'Discovery':
                addr = event['Data']['Data']['MAC']
                print 'Connecting to mac_str...'
                try:
                    tm2.connect_controller(addr)  # expecting tm2 = The device as tm2
                except:
                    print 'Failed to connect to controller ', mac_str
        except:
            print 'Serialized data is not in JSON format (', \
                n.serialized_data, ')'


# Ignore frames arriving from the sensor (just to showcase controller usage)

def on_frame(f):
    pass

try:
    # Create a context
    ctx = rs.context()

    # Wait until a TM2 device connects
    # We have to wait here, even if the device is already connected (since the library loads the device as usb device)
    print 'Waiting for all devices to connect...'
    found = False
    while not found:
        if len(ctx.devices) > 0:
            for d in ctx.devices:
                if d.get_info(rs.camera_info.product_id) == '2803':
                    tm2 = d.as_tm2()
                    print 'Found TM2 device: ', \
                        d.get_info(rs.camera_info.name), ' ', \
                        d.get_info(rs.camera_info.serial_number)
                    found = True

    # Get the sensor from the device
    s = tm2.sensors[0]

    # Register to notifications (To get notified of controller events)
    s.set_notifications_callback(on_notification)

    # Start the sensor
    s.open(s.profiles)
    s.start(on_frame)

    # Sleep for a while
    sleep(10)

    # Stop the sensor
    s.stop()
    s.close()
except:
    print "Error while running example"