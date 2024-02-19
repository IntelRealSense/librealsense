# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from rspy import test, log
import time

# global variable used to count on all the sensors simultaneously
count_frames = False
# tests parameters
TIME_FOR_STEADY_STATE = 3
TIME_TO_COUNT_FRAMES = 5


##########################################
# ---------- Helper Functions ---------- #
##########################################
def check_fps_pair(measured_fps, expected_fps):
    delta_Hz = expected_fps * 0.05  # Validation KPI is 5%
    return (measured_fps <= (expected_fps + delta_Hz) and measured_fps >= (expected_fps - delta_Hz))


def get_expected_fps_dict(sensor_profiles_dict):
    """
    Returns a dictionary between each stream name and its expected fps
    """
    expected_fps_dict = {profile.stream_name(): profile.fps()
                         for profiles in sensor_profiles_dict.values()
                         for profile in profiles}
    return expected_fps_dict


def get_dict_for_streams(sensor_profiles_arr, streams_to_test):
    sensor_profiles_dict = {}
    for stream_name in streams_to_test:
        for sensor, profile in sensor_profiles_arr:
            if stream_name in profile.stream_name():
                sensor_profiles_dict[sensor] = sensor_profiles_dict.get(sensor, []) + [profile]
    return sensor_profiles_dict


def get_time_est_string(profiles_array):
    global TIME_TO_COUNT_FRAMES, TIME_FOR_STEADY_STATE
    time_per_test = TIME_TO_COUNT_FRAMES + TIME_FOR_STEADY_STATE
    time_est_string = (f"Estimated time for test: {len(profiles_array) * time_per_test} secs "
                       f"({len(profiles_array)} tests * {time_per_test} secs per test)")
    return time_est_string


def get_tested_profiles_string(sensor_profiles_dict):
    tested_profile_string = ' + '.join(f"{sensor.name} / {profile.stream_name()}" for sensor, profiles
                                       in sensor_profiles_dict.items() for profile in profiles)
    return tested_profile_string


#############################################
# ---------- Core Test Functions ---------- #
#############################################
def check_fps_dict(measured_fps, expected_fps):
    all_fps_ok = True
    for profile_name in expected_fps:
        res = check_fps_pair(measured_fps[profile_name], expected_fps[profile_name])
        if not res:
            all_fps_ok = False
        log.d(f"Expected {expected_fps[profile_name]} fps, received {measured_fps[profile_name]} fps in profile"
              f" {profile_name}"
              f" {'(Pass)' if res else '(Fail)'}")
    return all_fps_ok


def generate_callbacks(sensor_profiles_dict, profile_name_fps_dict):
    """
    Creates callable functions for each sensor to be triggered when a new frame arrives
    Used to count frames received for measuring fps
    """
    def on_frame_received(frame):
        global count_frames
        if count_frames:
            profile_name = frame.profile.stream_name()
            profile_name_fps_dict[profile_name] += 1

    sensor_function_dict = {sensor_key: on_frame_received for sensor_key in sensor_profiles_dict}
    return sensor_function_dict


def measure_fps(sensor_profiles_dict):
    """
    Given a dictionary of sensors and profiles to test, activate all streams on the given profiles
    and measure fps
    Return a dictionary of profiles and the fps measured for them
    """
    global TIME_FOR_STEADY_STATE, TIME_TO_COUNT_FRAMES

    global count_frames
    count_frames = False

    # initialize fps dict
    profile_name_fps_dict = {profile.stream_name(): 0
                             for profiles in sensor_profiles_dict.values()
                             for profile in profiles}

    # generate sensor-callable dictionary
    funcs_dict = generate_callbacks(sensor_profiles_dict, profile_name_fps_dict)

    for sensor, profiles in sensor_profiles_dict.items():
        sensor.open(profiles)
        sensor.start(funcs_dict[sensor])

    # the core of the test - frames are counted during sleep when count_frames is on
    time.sleep(TIME_FOR_STEADY_STATE)
    count_frames = True  # Start counting frames
    time.sleep(TIME_TO_COUNT_FRAMES)
    count_frames = False  # Stop counting

    for sensor, profiles in sensor_profiles_dict.items():
        for profile in profiles:
            profile_name_fps_dict[profile.stream_name()] /= TIME_TO_COUNT_FRAMES

        sensor.stop()
        sensor.close()

    return profile_name_fps_dict


def get_test_details_str(sensor_profile_dict):
    test_details_str = ""
    for sensor, profiles in sensor_profile_dict.items():
        for profile in profiles:
            test_details_str += (f"Expected fps for profile {profile.stream_name()} on sensor "
                                 f"{sensor.name} is {profile.fps()} "
                                 f"on {get_resolution(profile)}\n")

    test_details_str = test_details_str.replace("on (0, 0)", "")  # remove no resolution for Motion Module profiles
    return test_details_str


############################################
# ----------- Public Functions ----------- #
############################################
def get_resolution(profile):
    return profile.as_video_stream_profile().width(), profile.as_video_stream_profile().height()


def perform_fps_test(sensor_profiles_arr, streams_combinations):
    """
    :param sensor_profiles_arr: an array of length N of tuples (sensor, profile) to test on
    :param streams_combinations: an array of combinations to run
                                 each combination is an array with stream names to test
    """
    log.d(get_time_est_string(streams_combinations))
    for streams_to_test in streams_combinations:
        partial_dict = get_dict_for_streams(sensor_profiles_arr, streams_to_test)
        with test.closure("Testing", get_tested_profiles_string(partial_dict)):
            expected_fps_dict = get_expected_fps_dict(partial_dict)
            log.d(get_test_details_str(partial_dict))
            fps_dict = measure_fps(partial_dict)
            test.check(check_fps_dict(fps_dict, expected_fps_dict))


def get_profile(sensor, stream, resolution=None, fps=None):
    return next((profile for profile in sensor.profiles if profile.stream_type() == stream
                and (resolution is None or get_resolution(profile) == resolution)
                and (fps is None or profile.fps() == fps)),
                None)  # return None if no profile found
