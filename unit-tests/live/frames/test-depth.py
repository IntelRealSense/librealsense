# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D400*
# test:device D585S

import pyrealsense2 as rs
from rspy import test, log
from rspy import tests_wrapper as tw
import numpy as np
import os
import time

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

dev, ctx = test.find_first_device_or_exit()
tw.start_wrapper( dev )

cfg = rs.config()
cfg.enable_stream(rs.stream.depth, rs.format.z16, 30)
if DEBUG_MODE:
    cfg.enable_stream(rs.stream.color, rs.format.bgr8, 30)


pipeline = rs.pipeline(ctx)
pipeline_profile = pipeline.start(cfg)
pipeline.wait_for_frames()
time.sleep(2)

def frames_to_image(depth, color, save, display):
    """
    This function gets depth and color frames, transforms them to an image (numpy array)
    and then save and/or display

    If color frame is given, it will also concatenate it with the depth frame before doing the given action
    """

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
        file_name = f"output_stream.png"
        log.i("Saved image in", os.getcwd() + "\\" + file_name)
        cv2.imwrite(file_name, img)
    if display:
        display_image(img)


def get_distances(depth_frame):
    MAX_METERS = 10  # max distance that can be detected, in meters

    depth_m = np.asanyarray(depth_frame.get_data()).astype(np.float32) * depth_frame.get_units()

    valid_mask = (depth_m < MAX_METERS)
    valid_depths = depth_m[valid_mask]  # ignore invalid pixels

    # convert to cm and round according to DETAIL_LEVEL
    rounded_depths = (np.floor(valid_depths * 100.0 / DETAIL_LEVEL) * DETAIL_LEVEL).astype(np.int32)

    unique_vals, counts = np.unique(rounded_depths, return_counts=True)
    
    dists = dict(zip(unique_vals.tolist(), counts.tolist()))
    total = valid_depths.size

    log.d("Distances detected in frame are:", dists)
    return dists, total


def is_depth_meaningful(save_image=False, show_image=False):
    """
    Checks if the camera is showing a frame with a meaningful depth.
    DETAIL_LEVEL is setting how close distances need to be, to be considered the same

    returns true if frame shows meaningful depth
    """
    frames = pipeline.wait_for_frames()
    depth = frames.get_depth_frame()
    color = frames.get_color_frame()
    if not depth:
        log.f("Error getting depth frame")
        return False
    if DEBUG_MODE and not color:
        log.e("Error getting color frame")

    dists, total = get_distances(depth)

    # save or display image (only possible through manual debugging)
    if save_image or show_image:
        frames_to_image(depth, color, save_image, show_image)

    # If any distance is the same on more than 90% of the pixels, there is no meaningful depth
    meaningful_depth = not any(v > total * 0.9 for v in dists.values())
    num_blank_pixels = dists[0]
    return meaningful_depth, num_blank_pixels


################################################################################################

test.start("Testing depth frame - laser ON -", dev.get_info(rs.camera_info.name))
# enable laser
sensor = pipeline_profile.get_device().first_depth_sensor()
if sensor.supports(rs.option.laser_power):
    sensor.set_option(rs.option.laser_power, sensor.get_option_range(rs.option.laser_power).max)
sensor.set_option(rs.option.emitter_enabled, 1)  # should be set to 0 for laser off

has_depth = False

# we check a few different frames to try and detect depth
for frame_num in range(FRAMES_TO_CHECK):
    has_depth, laser_black_pixels = is_depth_meaningful(save_image=DEBUG_MODE, show_image=DEBUG_MODE)
    if has_depth:
        break

test.check(has_depth is True)
test.finish()

if is_there_depth:
    # only the safety camera makes a clear difference between laser on and off in some cases
    if "D585S" in dev.get_info(rs.camera_info.name):
        test.start("Testing less black pixels present with the laser on")
        _, no_laser_black_pixels = is_depth_meaningful(cfg, laser_enabled=False, save_image=DEBUG_MODE, show_image=DEBUG_MODE)
        test.check(no_laser_black_pixels > max_black_pixels)
        test.finish()
else:
    log.i("Frame has no depth! ")

tw.stop_wrapper( dev )
test.print_results_and_exit()
