# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

# test:device each(D400*)


import pyrealsense2 as rs
from rspy.stopwatch import Stopwatch
from rspy import test, log
import time
import platform


# Start depth + color streams and measure the time from stream opened until first frame arrived using pipeline API.
# Verify that the time do not exceeds the maximum time allowed
# Note - Using Windows Media Foundation to handle power management between USB actions take time (~27 ms)


# Set maximum delay for first frame according to product line
dev = test.find_first_device_or_exit()

# The device starts at D0 (Operational) state, allow time for it to get into idle state
time.sleep( 3 )

product_line = dev.get_info(rs.camera_info.product_line)
if product_line == "D400":
    max_delay_for_depth_frame = 1
    max_delay_for_color_frame = 1
else:
    log.f("This test support only D400 devices")


def time_to_first_frame(config):
    pipe = rs.pipeline()
    start_call_stopwatch = Stopwatch()
    pipe.start(config)
    pipe.wait_for_frames()
    delay = start_call_stopwatch.get_elapsed()
    pipe.stop()
    return delay


################################################################################################
test.start("Testing pipeline first depth frame delay on " + product_line + " device - " + platform.system() + " OS")
depth_cfg = rs.config()
depth_cfg.enable_stream(rs.stream.depth, rs.format.z16, 30)
frame_delay = time_to_first_frame(depth_cfg)
print("Delay from pipeline.start() until first depth frame is: {:.3f} [sec] max allowed is: {:.1f} [sec] ".format(frame_delay, max_delay_for_depth_frame))
test.check(frame_delay < max_delay_for_depth_frame)
test.finish()


################################################################################################
test.start("Testing pipeline first color frame delay on " + product_line + " device - " + platform.system() + " OS")
color_cfg = rs.config()
color_cfg.enable_stream(rs.stream.color, rs.format.rgb8, 30)
frame_delay = time_to_first_frame(color_cfg)
print("Delay from pipeline.start() until first color frame is: {:.3f} [sec] max allowed is: {:.1f} [sec] ".format(frame_delay, max_delay_for_color_frame))
test.check(frame_delay < max_delay_for_color_frame)
test.finish()


################################################################################################
test.print_results_and_exit()
