# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device D585S

from rspy import test, log
import pyrealsense2 as rs
import fps_helper

VGA_RESOLUTION = (640, 360)
HD_RESOLUTION = (1280, 720)


def get_sensors_and_profiles(device):
    """
    Returns an array of pairs of a (sensor, profile) for each of its profiles
    """
    # when we will be able to stream in service mode:
    # uncomment the lines about auto exposure and use tests_wrapper before and after the test
    sensor_profiles_arr = []
    for sensor in device.query_sensors():
        profile = None
        if sensor.is_depth_sensor():
            # if sensor.supports(rs.option.enable_auto_exposure):
            #     sensor.set_option(rs.option.enable_auto_exposure, 1)
            profile = fps_helper.get_profile(sensor, rs.stream.depth, VGA_RESOLUTION, 30)
        elif sensor.is_color_sensor():
            # if sensor.supports(rs.option.enable_auto_exposure):
            #     sensor.set_option(rs.option.enable_auto_exposure, 1)
            # if sensor.supports(rs.option.auto_exposure_priority):
            #     sensor.set_option(rs.option.auto_exposure_priority, 0)  # AE priority should be 0 for constant FPS
            profile = fps_helper.get_profile(sensor, rs.stream.color, HD_RESOLUTION, 30)
        elif sensor.is_motion_sensor():
            sensor_profiles_arr.append((sensor, fps_helper.get_profile(sensor, rs.stream.accel)))
            sensor_profiles_arr.append((sensor, fps_helper.get_profile(sensor, rs.stream.gyro)))
        elif sensor.is_safety_sensor():
            profile = fps_helper.get_profile(sensor, rs.stream.safety)
        elif sensor.name == "Depth Mapping Camera":
            sensor_profiles_arr.append((sensor, fps_helper.get_profile(sensor, rs.stream.labeled_point_cloud)))
            sensor_profiles_arr.append((sensor, fps_helper.get_profile(sensor, rs.stream.occupancy)))

        if profile is not None:
            sensor_profiles_arr.append((sensor, profile))
    return sensor_profiles_arr


dev = test.find_first_device_or_exit()
sensor_profiles_array = get_sensors_and_profiles(dev)

permutations_to_run = [["Depth", "Color"],
                       ["Depth", "Color", "Safety"],
                       ["Depth", "Color", "Safety", "Occupancy"],
                       ["Depth", "Color", "Safety", "Labeled Point Cloud"],
                       ["Depth", "Color", "Accel", "Gyro"]]
fps_helper.perform_fps_test(sensor_profiles_array, permutations_to_run)

test.print_results_and_exit()
