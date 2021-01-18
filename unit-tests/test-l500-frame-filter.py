import pyrealsense2 as rs, test
import time

test.start("TODO")

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
device = devices[0]
depth_sensor = device.first_depth_sensor()

try:
    dp = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth)
except RuntimeError as e:
    test.unexpected_exception()
    test.finish()

depth_frame_cnt = 0
non_depth_frame_cnt = 0

def depth_frames_counter(frame):
        
    if frame.get_profile().stream_type() == rs.stream.depth:
        global depth_frame_cnt
        depth_frame_cnt += 1
    else:
        global non_depth_frame_cnt
        non_depth_frame_cnt += 1

    
    
depth_sensor.open(dp)
depth_sensor.start(depth_frames_counter)
time.sleep(5)

test.check(depth_frame_cnt == 0)
test.check_equal(non_depth_frame_cnt, 0)
test.finish()

#############################################################################################
test.print_results_and_exit()