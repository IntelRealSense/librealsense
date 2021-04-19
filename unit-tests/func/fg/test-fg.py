# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#test:device L500*

import pyrealsense2 as rs
from rspy import test,ac

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
depth_sensor = devices[0].first_depth_sensor()

debug_sensor = rs.debug_stream_sensor(depth_sensor)
debug_profiles = debug_sensor.get_debug_stream_profiles()


#############################################################################################
test.start("FG isn't exposed by get_stream_profiles")

matches = list(p for p in depth_sensor.profiles if p.format() == rs.format.fg)
test.check(len(matches) == 0 )
test.finish()

#############################################################################################
test.start("FG exposed by debug_stream_sensor")

matches = list(p for p in debug_profiles if p.format() == rs.format.fg)
test.check(len(matches) > 0 )
test.finish()

#############################################################################################
test.start("streaming FG 800x600")

dp = next(p for p in debug_profiles if p.fps() == 30
                        and p.stream_type() == rs.stream.depth
                        and p.format() == rs.format.fg
                        and p.as_video_stream_profile().width() == 800
                        and p.as_video_stream_profile().height() == 600)
depth_sensor.open( dp )
lrs_queue = rs.frame_queue(capacity=10, keep_frames=False)
depth_sensor.start( lrs_queue )

try:
    lrs_frame = lrs_queue.wait_for_frame(5000)
    test.check_equal(lrs_frame.profile.format(), rs.format.fg)
except:
    test.unexpected_exception()
finally:
    debug_sensor.stop()
    debug_sensor.close()
test.finish()

#############################################################################################

test.print_results_and_exit()
