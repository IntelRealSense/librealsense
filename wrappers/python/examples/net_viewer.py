## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2021 Intel Corporation. All Rights Reserved.

###############################################
##      Network viewer                       ##
###############################################

import sys
import numpy as np
import cv2
import pyrealsense2 as rs
import pyrealsense2_net as rsnet


if len(sys.argv) == 1:
    print( 'syntax: python net_viewer <server-ip-address>' )
    sys.exit(1)

ip = sys.argv[1]

ctx = rs.context()
print ('Connecting to ' + ip)
dev = rsnet.net_device(ip)
print ('Connected')
print ('Using device 0,', dev.get_info(rs.camera_info.name), ' Serial number: ', dev.get_info(rs.camera_info.serial_number))
dev.add_to(ctx)
pipeline = rs.pipeline(ctx)


# Start streaming
print ('Start streaming, press ESC to quit...')
pipeline.start()

try:
    while True:

        # Wait for a coherent pair of frames: depth and color
        frames = pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()
        if not depth_frame or not color_frame:
            continue

        # Convert images to numpy arrays
        depth_image = np.asanyarray(depth_frame.get_data())
        color_image = np.asanyarray(color_frame.get_data())

        # Apply colormap on depth image (image must be converted to 8-bit per pixel first)
        depth_colormap = cv2.applyColorMap(cv2.convertScaleAbs(depth_image, alpha=0.03), cv2.COLORMAP_JET)

        depth_colormap_dim = depth_colormap.shape
        color_colormap_dim = color_image.shape

        # If depth and color resolutions are different, resize color image to match depth image for display
        if depth_colormap_dim != color_colormap_dim:
            resized_color_image = cv2.resize(color_image, dsize=(depth_colormap_dim[1], depth_colormap_dim[0]), interpolation=cv2.INTER_AREA)
            images = np.hstack((resized_color_image, depth_colormap))
        else:
            images = np.hstack((color_image, depth_colormap))

        # Show images
        cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
        cv2.imshow('RealSense', images)
        k = cv2.waitKey(1) & 0xFF
        if k == 27:    # Escape
            cv2.destroyAllWindows()
            break

finally:
    # Stop streaming
    pipeline.stop()

print ("Finished")
