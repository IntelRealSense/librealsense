# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

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
    print(repr(md_value) + ": " + repr(val))

def check_safety_metadata(frame):
    for md_value in safety_metadata_values:
        check_md_value(frame, md_value)
        

test.start("Checking Safety Stream Metadata")

cfg = rs.config()
cfg.enable_stream(rs.stream.safety)

pipe = rs.pipeline()
profiles = pipe.start(cfg)

f = pipe.wait_for_frames()

check_safety_metadata(f)

pipe.stop()

test.finish()

test.print_results_and_exit()


