# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:device each(D400*)
# test:device each(D500*)
# donotrun D500*

import pyrealsense2 as rs
from rspy import test, log


def close_resources(s):
    """
    Stop and close a sensor
    :param s: sensor of device
    """
    if len(s.get_active_streams()) > 0:
        s.stop()
        s.close()


def is_contain_profile(profiles: dict, new_profile) -> bool:
    """
    Check if a given stream type exists in a dictionary
    :param profiles: Dictionary of profiles and sensors
    :param new_profile: Profile that we want to check if it is in a dictionary
    :returns: True if a type of profile is already in the list otherwise False
    """
    if new_profile:
        for pr in profiles.keys():
            if pr.stream_type() == new_profile.stream_type():
                return True
    return False


def is_frame_support_metadata(frame, metadata) -> bool:
    """
    Check if a metadata supporting by a frame
    :param frame: a frame from device
    :param metadata: a metadata of frame
    :return: true if metadata supporting by frame otherwise false
    """
    return frame and frame.supports_frame_metadata(frame_metadata=metadata)


def append_testing_profiles(dev) -> None:
    """
    Fill dictionary of testing profiles and his sensor
    :param dev: Realsense device
    :return: None
    """
    global testing_profiles

    for s in dev.sensors:
        for p in s.profiles:
            if not is_contain_profile(testing_profiles, p):
                testing_profiles[p] = s


def is_value_keep_increasing(metadata_value, number_frames_to_test=50) -> bool:
    """
    Check that a given counter in metadata increases.
    :param metadata_value: that we need to check
    :param number_frames_to_test: amount frames that we want to test
    :return: true if the counter value keep increasing otherwise false
    """
    prev_metadata_value = -1
    global frame_queue

    while number_frames_to_test > 0:
        f = frame_queue.wait_for_frame()
        current_value = f.get_frame_metadata(metadata_value)

        if prev_metadata_value >= current_value:
            return False

        number_frames_to_test -= 1

    return True


queue_capacity = 1
frame_queue = rs.frame_queue(queue_capacity, keep_frames=False)
device = test.find_first_device_or_exit()

# We're using dictionary because we need save a profile and his sensor.
# The key value is profile and value is his sensor.
testing_profiles = {}

append_testing_profiles(device)

for profile, sensor in testing_profiles.items():
    sensor.open(profile)
    sensor.start(frame_queue)

    # Test #1
    if is_frame_support_metadata(frame_queue.wait_for_frame(), rs.frame_metadata_value.frame_counter):
        test.start('Verifying increasing counter for profile ', profile)
        test.check(is_value_keep_increasing(rs.frame_metadata_value.frame_counter))
        test.finish()

    # Test #2
    if is_frame_support_metadata(frame_queue.wait_for_frame(), rs.frame_metadata_value.frame_timestamp):
        test.start('Verifying increasing time for profile ', profile)
        test.check(is_value_keep_increasing(rs.frame_metadata_value.frame_timestamp))
        test.finish()

    close_resources(sensor)

test.print_results_and_exit()
