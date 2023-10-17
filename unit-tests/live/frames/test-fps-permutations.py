# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:donotrun:!nightly
# test:timeout 750
# timeout - on the worst case, we're testing on D585S, which have 8 sensors, so:
# timeout = (8 choose 2) * 24 + 24 = 696
# 8 choose 2 tests to do (one for each pair), plus one for all sensors on, each test takes 24 secs

from rspy import test, log
import time
import itertools
import math


# To add a new test mode (ex. all singles or triplets) only add it below and on should_test
# add it in get_time_est_string for an initial time calculation
# run perform_fps_test(sensor_profiles_array, [mode])
ALL_PERMUTATIONS = 1
ALL_PAIRS = 2
ALL_SENSORS = 3

# global variable used to count on all the sensors simultaneously
count_frames = False

# tests parameters
TEST_ALL_COMBINATIONS = False
seconds_till_steady_state = 4
seconds_to_count_frames = 20


##########################################
# ---------- Helper Functions ---------- #
##########################################
def get_resolution(profile):
    return profile.as_video_stream_profile().width(), profile.as_video_stream_profile().height()


def check_fps_pair(fps, expected_fps):
    delta_Hz = expected_fps * 0.05  # Validation KPI is 5%
    return (fps <= (expected_fps + delta_Hz) and fps >= (expected_fps - delta_Hz))


def get_expected_fps(sensor_profiles_dict):
    """
    For every sensor, find the expected fps according to its profiles
    Return a dictionary between the sensor and its expected fps
    """
    expected_fps_dict = {}
    # print(sensor_profiles_dict)
    for key in sensor_profiles_dict:
        avg = 0
        for profile in sensor_profiles_dict[key]:
            avg += profile.fps()

        avg /= len(sensor_profiles_dict[key])
        expected_fps_dict[key] = avg
    return expected_fps_dict


def should_test(mode, permutation):
    return ((mode == ALL_PERMUTATIONS) or
            (mode == ALL_SENSORS and all(v == 1 for v in permutation)) or
            (mode == ALL_PAIRS and permutation.count(1) == 2))


def get_dict_for_permutation(sensor_profiles_arr, permutation):
    """
    Given an array of tuples (sensor, profile) and a permutation of the same length,
    return a sensor-profiles dictionary containing the relevant profiles
     - profile will be added only if there's 1 in the corresponding element in the permutation
    """
    partial_dict = {}
    for i, j in enumerate(permutation):
        if j == 1:
            sensor = sensor_profiles_arr[i][0]
            partial_dict[sensor] = partial_dict.get(sensor, []) + [sensor_profiles_arr[i][1]]
    return partial_dict


def get_time_est_string(num_profiles, modes):
    s = "Estimated time for test:"
    details_str = ""
    total_time = 0
    global seconds_to_count_frames
    global seconds_till_steady_state
    time_per_test = seconds_to_count_frames + seconds_till_steady_state
    for mode in modes:
        test_time = 0
        if mode == ALL_PERMUTATIONS:
            test_time = math.factorial(num_profiles) * time_per_test
            details_str += f"{math.factorial(num_profiles)} tests for all permutations"
        elif mode == ALL_PAIRS:
            test_time = math.comb(num_profiles, 2) * time_per_test
            details_str += f"{math.comb(num_profiles,2)} tests for all pairs"
        elif mode == ALL_SENSORS:
            test_time = time_per_test
            details_str += f"1 test for all sensors on"

        details_str += " + "
        total_time += test_time

    details_str = details_str[:-3]
    details_str = f"({details_str})"
    details_str += f" * {time_per_test} secs per test"
    s = f"{s} {total_time} secs ({details_str})"
    return s


def get_tested_profiles_string(sensor_profiles_dict):
    s = ""
    for sensor in sensor_profiles_dict:
        for profile in sensor_profiles_dict[sensor]:
            s += sensor.name + " / " + profile.stream_name() + " + "
    s = s[:-3]
    return s


#############################################
# ---------- Core Test Functions ---------- #
#############################################
def check_fps(expected_fps, fps_measured):
    all_fps_ok = True
    for key in expected_fps:
        res = check_fps_pair(expected_fps[key], fps_measured[key])
        if not res:
            all_fps_ok = False
            log.e(f"Expected {expected_fps[key]} fps, received {fps_measured[key]} fps in sensor {key.name}")
    return all_fps_ok


def generate_functions(sensor_profiles_dict):
    """
    Creates callable functions for each sensor to be triggered when a new frame arrives
    Used to count frames received for measuring fps
    """
    sensor_function_dict = {}
    for sensor_key in sensor_profiles_dict:
        def on_frame_received(frame, key=sensor_key):
            global count_frames
            if count_frames:
                sensor_profiles_dict[key] += 1

        sensor_function_dict[sensor_key] = on_frame_received
    return sensor_function_dict


def measure_fps(sensor_profiles_dict):
    """
    Given a dictionary of sensors and profiles to test, activate all sensors on the given profiles
    and measure fps
    Return a dictionary of sensors and the fps measured for them
    """
    global seconds_till_steady_state
    global seconds_to_count_frames

    global count_frames
    count_frames = False

    # initialize fps dict
    sensor_fps_dict = {}
    for key in sensor_profiles_dict:
        sensor_fps_dict[key] = 0

    # generate sensor-callable dictionary
    funcs_dict = generate_functions(sensor_fps_dict)

    for key in sensor_profiles_dict:
        sensor = key
        profiles = sensor_profiles_dict[key]
        sensor.open(profiles)
        sensor.start(funcs_dict[key])

    # the core of the test - frames are counted during sleep when count_frames is on
    time.sleep(seconds_till_steady_state)
    count_frames = True  # Start counting frames
    time.sleep(seconds_to_count_frames)
    count_frames = False  # Stop counting

    for key in sensor_profiles_dict:
        sensor_fps_dict[key] /= len(sensor_profiles_dict[key])  # number of profiles on the sensor
        sensor_fps_dict[key] /= seconds_to_count_frames
        # now for each sensor we have the average fps received

        sensor = key
        sensor.stop()
        sensor.close()

    return sensor_fps_dict


def get_test_details_str(sensor_profile_dict, expected_fps_dict):
    s = ""
    for sensor_key in sensor_profile_dict:
        if len(sensor_profile_dict[sensor_key]) > 1:
            s += f"Expected average fps for {sensor_key.name} is {expected_fps_dict[sensor_key]}:\n"
            for profile in sensor_profile_dict[sensor_key]:
                s += f"Profile {profile.stream_name()} expects {profile.fps()} fps on {get_resolution(profile)}\n"
        else:
            s += (f"Expected fps for sensor {sensor_key.name} on profile "
                  f"{sensor_profile_dict[sensor_key][0].stream_name()} is {expected_fps_dict[sensor_key]}"
                  f" on {get_resolution(sensor_profile_dict[sensor_key][0])}\n")
    s = s.replace("on (0, 0)", "")  # remove no resolution for Motion Module profiles
    s += "***************"
    return s


def perform_fps_test(sensor_profiles_arr, modes):

    log.d(get_time_est_string(len(sensor_profiles_arr), modes))

    for mode in modes:
        perms = list(itertools.product([0, 1], repeat=len(sensor_profiles_arr)))
        for perm in perms:
            # print(perm)
            if all(v == 0 for v in perm):
                continue
            if should_test(mode, perm):
                partial_dict = get_dict_for_permutation(sensor_profiles_arr, perm)
                test.start("Testing", get_tested_profiles_string(partial_dict))
                expected_fps = get_expected_fps(partial_dict)
                log.d(get_test_details_str(partial_dict, expected_fps))
                fps_dict = measure_fps(partial_dict)
                test.check(check_fps(expected_fps, fps_dict))
                test.finish()


####################################################
# ---------- Test Preparation Functions ---------- #
####################################################
def get_profiles_by_resolution(sensor, resolution, fps=None):
    # resolution is a required parameter because on a sensor all active profiles must have the same resolution
    profiles = []
    p_streams_added = []
    for p in sensor.get_stream_profiles():
        if get_resolution(p) == resolution:
            if fps is None or p.fps() == fps:
                if p.stream_type() not in p_streams_added:  # can't open same stream twice
                    profiles.append(p)
                    p_streams_added.append(p.stream_type())
    return profiles


def get_mutual_resolution(sensor):
    profile_resolutions_dict = {}  # a map between a stream type and all of its possible resolutions
    possible_combinations = []
    for profile in sensor.get_stream_profiles():
        stream_type = profile.stream_type()
        resolution = get_resolution(profile)
        fps = profile.fps()

        # d[key] = d.get(key, []) + [value] -> adds to the dictionary or appends if it exists
        profile_resolutions_dict[stream_type] = profile_resolutions_dict.get(stream_type, []) + [resolution]

        if (resolution, fps) not in possible_combinations:
            possible_combinations.append((resolution, fps))

    possible_combinations.sort(reverse=True)  # sort by resolution first, then by fps, so the best res and fps are first

    # first, try to find a resolution and fps that all profiles have
    for option in possible_combinations:
        profiles = get_profiles_by_resolution(sensor, option[0], option[1])
        if len(profiles) == len(profile_resolutions_dict):
            return profiles

    # if none found, try to find a resolution that all profiles have, on any fps (fps will be taken as an average later)
    for option in possible_combinations:
        profiles = get_profiles_by_resolution(sensor, option[0], None)
        if len(profiles) == len(profile_resolutions_dict):
            return profiles

    # if reached here, then we couldn't find a resolution that all profiles have, so we can't test them together :(
    log.f("Can't run test, sensor", sensor.name, "doesn't have a resolution for all profiles")
    return []


def get_sensors_and_profiles(device):
    """
    Returns an array of tuples of a (sensor, profile) for each of its profiles
    """
    sensor_profiles_arr = []
    for sensor in device.query_sensors():
        profiles = get_mutual_resolution(sensor)
        for profile in profiles:
            sensor_profiles_arr.append((sensor, profile))

    return sensor_profiles_arr


#####################################################################################################

dev = test.find_first_device_or_exit()
sensor_profiles_array = get_sensors_and_profiles(dev)

if TEST_ALL_COMBINATIONS:
    test_modes = [ALL_PERMUTATIONS]
else:
    test_modes = [ALL_PAIRS, ALL_SENSORS]

perform_fps_test(sensor_profiles_array, test_modes)

#####################################################################################################

test.print_results_and_exit()
