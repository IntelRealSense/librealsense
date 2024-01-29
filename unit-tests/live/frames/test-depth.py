# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D400*

import pyrealsense2 as rs
from rspy import test, log
from rspy import tests_wrapper as tw
import os
import math

# Start depth + color streams and go through the frame to make sure it is showing a depth image
# Color stream is only used to display the way the camera is facing
# Verify that the frame does indeed have variance - therefore it is showing a depth image

# In debug mode, an image with the frames found will be displayed and saved
# Make sure you have the required packages installed
DEBUG_MODE = False

# Defines how far in cm do pixels have to be, to be considered in a different distance
# for example, 10 for 10cm, will define the range 100-109 cm as one (as 100)
DETAIL_LEVEL = 10
FRAMES_TO_CHECK = 30

dev = test.find_first_device_or_exit()
tw.start_wrapper( dev )

cfg = rs.config()
cfg.enable_stream(rs.stream.depth, rs.format.z16, 30)
if DEBUG_MODE:
    cfg.enable_stream(rs.stream.color, rs.format.bgr8, 30)


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
        elif k == -1:  # normally -1 returned
            pass


def frames_to_image(depth, color, save, display, laser_enabled):
    """
    This function gets depth and color frames, transforms them to an image (numpy array)
    and then save and/or display

    If color frame is given, it will also concatenate it with the depth frame before doing the given action
    """
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
        file_name = f"output_stream{'_laser_on' if laser_enabled else '_laser_off'}.png"
        log.i("Saved image in", os.getcwd() + "\\" + file_name)
        cv2.imwrite(file_name, img)
    if display:
        display_image(img)


def get_frames(config, laser_enabled):
    pipeline = rs.pipeline()
    pipeline_profile = pipeline.start(config)

    sensor = pipeline_profile.get_device().first_depth_sensor()
    if laser_enabled and sensor.supports(rs.option.laser_power):
        sensor.set_option(rs.option.laser_power, sensor.get_option_range(rs.option.laser_power).max)
    sensor.set_option(rs.option.emitter_enabled, 1 if laser_enabled else 0)

    # to get a proper image, we sometimes need to wait a few frames, like when the camera is facing a light source
    frames = pipeline.wait_for_frames()
    for i in range(30):
        frames = pipeline.wait_for_frames()
    depth = frames.get_depth_frame()
    color = frames.get_color_frame()
    pipeline.stop()
    return depth, color


def round_to_units(num):
    """
    Input: Distance of a certain point, in meters
    Output: Distance according to the given detail level, in cm
    """
    in_cm = round(num, 2) * 100  # convert to cm
    return math.floor(in_cm / DETAIL_LEVEL) * DETAIL_LEVEL  # rounds the distance according to the given unit


def sort_dict(my_dict):
    my_keys = list(my_dict.keys())
    my_keys.sort()
    sorted_dict = {i: my_dict[i] for i in my_keys}
    return sorted_dict


def get_distances(depth):
    MAX_METERS = 10  # max distance that can be detected, in meters
    dists = {}
    total = 0
    for y in range(depth.get_height()):
        for x in range(depth.get_width()):
            dist = depth.get_distance(x, y)
            if dist >= MAX_METERS:  # out of bounds, assuming it is a junk value
                continue

            dist = round_to_units(dist)  # round according to DETAIL_LEVEL

            if dists.get(dist) is not None:
                dists[dist] += 1
            else:
                dists[dist] = 1
            total += 1

    dists = sort_dict(dists)  # for debug convenience
    log.d("Distances detected in frame are:", dists)
    return dists, total


def is_depth_meaningful(config, laser_enabled=True, save_image=False, show_image=False):
    """
    Checks if the camera is showing a frame with a meaningful depth.
    DETAIL_LEVEL is setting how close distances need to be, to be considered the same

    returns true if frame shows meaningful depth
    """

    depth, color = get_frames(config, laser_enabled)

    if not depth:
        log.f("Error getting depth frame")
        return False
    if DEBUG_MODE and not color:
        log.e("Error getting color frame")

    dists, total = get_distances(depth)

    # save or display image (only possible through manual debugging)
    if save_image or show_image:
        frames_to_image(depth, color, save_image, show_image, laser_enabled)

    # Goes over the distances found, and checks if any distance is the same on more than 90% of the pixels
    meaningful_depth = True
    for key in dists:
        if dists[key] > total*0.9:
            meaningful_depth = False
            break
    num_blank_pixels = dists[0]
    return meaningful_depth, num_blank_pixels


################################################################################################

test.start("Testing depth frame - laser ON -", dev.get_info(rs.camera_info.name))
is_there_depth = False
max_black_pixels = float('inf')

# we perform the check on a few different frames to make sure we get the best indication if we have depth
for frame_num in range(FRAMES_TO_CHECK):
    result, laser_black_pixels = is_depth_meaningful(cfg, laser_enabled=True, save_image=DEBUG_MODE, show_image=DEBUG_MODE)
    is_there_depth = is_there_depth or result  # we check if we found depth at any frame checked
    max_black_pixels = min(max_black_pixels, laser_black_pixels)

test.check(is_there_depth is True)
test.finish()

tw.stop_wrapper( dev )
test.print_results_and_exit()
