# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time
import sys
import os

# Start depth + color streams and go through the frame to make sure it is showing a depth image
# Color stream is only used to display the way the camera is facing
# Verify that the frame does indeed have variance - therefore it is showing a depth image

dev = test.find_first_device_or_exit()

cfg = rs.config()
cfg.enable_stream(rs.stream.depth, rs.format.z16, 30)
cfg.enable_stream(rs.stream.color, rs.format.rgb8, 30)


def display_image(img):
    """
    Display a given image and exits when the x button or esc key are pressed
    """
    import cv2

    window_title = "Output Stream"
    cv2.imshow(window_title, img)
    while cv2.getWindowProperty(window_title, cv2.WND_PROP_VISIBLE) > 0:
        k = cv2.waitKey(33)
        if k == 27:  # Esc key to stop
            cv2.destroyAllWindows()
            break
        elif k == -1:  # normally -1 returned,so don't print it
            pass


def frames_to_image(depth, color, save, display):
    import numpy as np
    import cv2

    colorizer = rs.colorizer()
    depth_image = np.asanyarray(colorizer.colorize(depth).get_data())
    img = depth_image

    if color:  # if color frame was successfully captured, merge it and the depth frame
        from scipy.ndimage import zoom
        color_image = np.asanyarray(color.get_data())
        depth_rows, _, _ = depth_image.shape
        color_rows, _, _ = color_image.shape
        # resize the image with the higher resolution to look like the smaller one
        if depth_rows < color_rows:
            color_image = zoom(color_image, (depth_rows / color_rows, depth_rows / color_rows, 1))
        elif color_rows < depth_rows:
            depth_image = zoom(depth_image, (color_rows / depth_rows, color_rows / depth_rows, 1))
        img = np.concatenate((depth_image, color_image), axis=1)

    if save:
        file_name = "output_stream.png"
        print("Saved image in", os.getcwd() + "\\" + file_name)
        cv2.imwrite(file_name, img)
    if display:
        display_image(img)


def get_frames(config, laser_enabled):
    pipeline = rs.pipeline()
    pipeline_profile = pipeline.start(config)

    sensor = pipeline_profile.get_device().first_depth_sensor()
    sensor.set_option(rs.option.emitter_enabled, 1 if laser_enabled else 0)

    # to get a proper image, we sometimes need to wait a few frames, like when the camera is facing a light source
    frames = pipeline.wait_for_frames()
    for i in range(30):
        frames = pipeline.wait_for_frames()
    depth = frames.get_depth_frame()
    color = frames.get_color_frame()
    pipeline.stop()
    return depth, color


def get_distances(depth):
    MAX_METERS = 10  # max distance that can be detected, in meters
    dists = {}
    total = 0
    for y in range(depth.get_height()):
        for x in range(depth.get_width()):
            dist = depth.get_distance(x, y)
            if dist >= MAX_METERS:  # out of bounds, assuming it is a junk value
                continue
            # we only distinguish between two pixels if they are different more than 10cm apart
            # we can get a more accurate measure if we round it less, say round(dist,2)*100 to get 1cm difference noted
            dist = int(round(dist, 1) * 10)
            if dists.get(dist) is not None:
                dists[dist] += 1
            else:
                dists[dist] = 1
            total += 1
    # print(dists)
    return dists, total


def is_depth_meaningful(config, laser_enabled=True, save_image=False, show_image=False):
    """
    Checks if the camera is showing a frame with a meaningful depth
    the higher the detail level is, the more it is sensitive to distance
    ex: 1 for a difference of 1 meter, 100 for a diff of 1cm

    returns true if frame shows meaningful depth
    """

    depth, color = get_frames(config, laser_enabled)

    if not depth:
        print("Error getting depth frame")
        return False
    if not color:
        print("Error getting color frame")

    dists, total = get_distances(depth)

    # save or display image (only possible through manual debugging)
    if save_image or show_image:
        frames_to_image(depth, color,save_image,show_image)

    meaningful_depth = True
    for key in dists:
        if dists[key] > total*0.9:
            meaningful_depth = False
            break
    return meaningful_depth


################################################################################################
test.start("Testing depth frame - laser ON -", dev.get_info(rs.camera_info.name))
# change file_name or show_image to save or display the image, accordingly
# make sure to have the packages listed if changed
res = is_depth_meaningful(cfg, True, False, False)
test.check(res is True)
test.finish()

################################################################################################

test.start("Testing depth frame - laser OFF -", dev.get_info(rs.camera_info.name))
res = is_depth_meaningful(cfg, False, False, False)
test.check(res is True)
test.finish()

################################################################################################

test.print_results_and_exit()
