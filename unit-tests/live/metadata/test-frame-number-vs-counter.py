# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
import time

# test:device each(L500*)
# test:device each(D400*)
# test:device each(D500*)

import pyrealsense2 as rs
from rspy import test, log
from rspy.timer import Timer

waiting_for_test = False
MAX_TIME_TO_WAIT_FOR_FRAMES = 5  # [sec]


def check_frame_number_equal_to_counter(frame):
    global waiting_for_test
    frame_number = frame.get_frame_number()
    frame_counter = frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
    test.check_equal(frame_number, frame_counter)
    waiting_for_test = False


def stream_type_not_in_map(stream_profiles, stream_type):
    for sp in stream_profiles:
        if sp.stream_type() == stream_type:
            return False
    return True


def get_sensor_streams_profiles(sensor):
    stream_profiles = []
    for p in sensor.profiles:
        if stream_type_not_in_map(stream_profiles, p.stream_type()):
            v = p.as_video_stream_profile()
            if v:
                # SC - following line is needed because without it, infrared profile with res 1600x1300 is chosen,
                # and this profile won't stream in normal mode
                if v.width() == 640 or sensor.get_info(rs.camera_info.name) == "Depth Mapping Camera":
                    stream_profiles.append(p)
            else:
                stream_profiles.append(p)
    return stream_profiles


wait_for_frames_timer = Timer(MAX_TIME_TO_WAIT_FOR_FRAMES)

test.start("checking frame number and frame counter are synchronized")
dev = test.find_first_device_or_exit()

for sensor in dev.query_sensors():
    streams_in_sensor = get_sensor_streams_profiles(sensor)
    for stream_profile in streams_in_sensor:
        log.i("testing: " + sensor.get_info(rs.camera_info.name) + ", " + repr(stream_profile.stream_type()))
        sensor.open(stream_profile)
        sensor.start(check_frame_number_equal_to_counter)
        wait_for_frames_timer.start()
        waiting_for_test = True
        while waiting_for_test and not wait_for_frames_timer.has_expired():
            time.sleep(0.5)
        if wait_for_frames_timer.has_expired():
            log.i("timer expired: " + "no frame arrived before " + repr(MAX_TIME_TO_WAIT_FOR_FRAMES) + " sec")
            test.fail()
        sensor.stop()
        sensor.close()

test.finish()
test.print_results_and_exit()
