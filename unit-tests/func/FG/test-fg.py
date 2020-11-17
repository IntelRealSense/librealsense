import pyrealsense2 as rs, test, ac

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()

debug_sensor = rs.debug_streaming_sensor(depth_sensor)
debug_profiles = debug_sensor.get_debug_stream_profiles()

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
    lrs_frame = lrs_queue.wait_for_frame(150000)
    debug_sensor.stop()
    debug_sensor.close()
    test.check_equal(lrs_frame.profile.format(), rs.format.fg)
except:
    test.unexpected_exception()
test.finish()
#############################################################################################
test.start("streaming FG 1280x720")

dp = next(p for p in debug_profiles if p.fps() == 30
                        and p.stream_type() == rs.stream.depth
                        and p.format() == rs.format.fg
                        and p.as_video_stream_profile().width() == 1280
                        and p.as_video_stream_profile().height() == 720)
depth_sensor.open( dp )
lrs_queue = rs.frame_queue(capacity=10, keep_frames=False)
depth_sensor.start( lrs_queue )

try:
    lrs_frame = lrs_queue.wait_for_frame(150000)
    debug_sensor.stop()
    debug_sensor.close()
    test.check_equal(lrs_frame.profile.format(), rs.format.fg)
except:
    test.unexpected_exception()
test.finish()
#############################################################################################

test.print_results_and_exit()