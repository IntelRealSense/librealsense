# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.

import pyrealsense2 as rs
from rspy import log, test

prev_counter = 0
prev_ts = 0

def reset_data():
    global prev_counter, prev_ts
    prev_counter = 0
    prev_ts = 0

# if you call it in more than 1 test remember to first call 'reset_date()'
def check_counter_and_timestamp_increase(frame, fps):
    global prev_counter, prev_ts
    if prev_counter == 0 and prev_ts == 0:
        prev_counter = frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
        prev_ts = frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
    else:
        current_counter = frame.get_frame_metadata(rs.frame_metadata_value.frame_counter)
        current_ts = frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
        test.info("prev_counter", prev_counter)
        test.info("current_counter", current_counter)
        test.check(current_counter > prev_counter) # D500 has a skip frames mechanism on low fps meaning no sequential frame numbers
        test.check((current_ts - prev_ts) / 1000 < 2 * 1000 / fps)
        prev_counter = current_counter
        prev_ts = current_ts
        
def check_md_value(frame, md_value):
    test.check(frame.supports_frame_metadata(md_value))
    val = frame.get_frame_metadata(md_value)
    log.d(repr(md_value) + ": " + repr(val))