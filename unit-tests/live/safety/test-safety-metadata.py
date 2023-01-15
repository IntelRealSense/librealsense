# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#test:donotrun
#test:device D585S

import pyrealsense2 as rs
from rspy import test

safety_metadata_values = [rs.frame_metadata_value.frame_counter,
                          rs.frame_metadata_value.frame_timestamp,
                          rs.frame_metadata_value.safety_level1,
                          rs.frame_metadata_value.safety_level2]

def check_md_value(frame, md_value):
    test.check(frame.supports_frame_metadata(md_value))
    val = frame.get_frame_metadata(md_value)
    rs.log(rs.log_severity.info, repr(md_value) + ": " + repr(val))

def check_safety_metadata(frame):
    for md_value in safety_metadata_values:
        check_md_value(frame, md_value)

prev_counter = 0
prev_ts = 0

def check_counter_and_timestamp_increase(frame, fps):
    global prev_counter, prev_ts
    if prev_counter == 0 and prev_ts == 0:
        prev_counter = frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
        prev_ts = frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
    else:
        current_counter = frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
        current_ts = frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
        test.check(current_counter == prev_counter + 1)
        test.check((current_ts - prev_ts) / 1000 < 2 * 1000 / fps)
        prev_counter = current_counter
        prev_ts = current_ts



        
################# Checking metadata required fileds are received ##################
test.start("Checking Safety Stream Metadata received")

cfg = rs.config()
cfg.enable_stream(rs.stream.safety)
pipe = rs.pipeline()
pipe.start(cfg)
iterations = 0
while iterations < 20:
    iterations = iterations + 1
    f = pipe.wait_for_frames()
    check_safety_metadata(f)
pipe.stop()
test.finish()

################# Checking frame counter and timestamp increasing ##################
test.start("Checking Safety Stream Metadata frame counter and timestamp increasing")
cfg = rs.config()
fps = 30
cfg.enable_stream(rs.stream.safety, rs.format.raw8, fps)
pipe = rs.pipeline()
pipe.start(cfg)
iterations = 0
while iterations < 20:
    iterations = iterations + 1
    f = pipe.wait_for_frames()
    check_counter_and_timestamp_increase(f, fps)
pipe.stop()
test.finish()


test.print_results_and_exit()


