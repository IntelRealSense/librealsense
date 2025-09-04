# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.
import time

# test:device D585S

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


wait_for_frames_timer = Timer(MAX_TIME_TO_WAIT_FOR_FRAMES)

test.start("checking frame number and frame counter are synchronized")
dev, _ = test.find_first_device_or_exit()

depth_mapping_sensor = next(s for s in dev.query_sensors() if s.get_info(rs.camera_info.name) == "Depth Mapping Camera")
occupancy_stream_profile = next(p for p in depth_mapping_sensor.profiles if p.stream_type() == rs.stream.occupancy)
labeled_points_stream_profile = next(p for p in depth_mapping_sensor.profiles if p.stream_type() == rs.stream.labeled_point_cloud)
safety_sensor = dev.first_safety_sensor()
safety_stream_profile = next(p for p in safety_sensor.profiles if p.stream_type() == rs.stream.safety)

sensors_and_stream_profiles = [[depth_mapping_sensor, occupancy_stream_profile],
                               [depth_mapping_sensor, labeled_points_stream_profile],
                               [safety_sensor, safety_stream_profile]]

for ssp in sensors_and_stream_profiles:
    sensor = ssp[0]
    stream_profile = ssp[1]
    log.i("testing: " + sensor.get_info(rs.camera_info.name) + ", " + repr(stream_profile.stream_type()))
    sensor.open(stream_profile)
    sensor.start(check_frame_number_equal_to_counter)
    wait_for_frames_timer.start()
    waiting_for_test = True
    while waiting_for_test and not wait_for_frames_timer.has_expired():
        time.sleep(0.5)
    test.check(not wait_for_frames_timer.has_expired())
    sensor.stop()
    sensor.close()

test.finish()
test.print_results_and_exit()
