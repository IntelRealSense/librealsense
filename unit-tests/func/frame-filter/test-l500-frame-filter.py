import pyrealsense2 as rs, test
import time

# The L515 device opens the IR steam with the depth stream even if the user did not asked for it (for improving the depth quality), 
# The frame-filter role is to  make sure only requested frames will get to the user.
# This test is devided into 2 test.
#   1. User ask for depth only - make sure he gets depth only frames
#   2. User ask for depth + IR - make sure he gets depth + IR frames

devices = test.find_devices_by_product_line_or_exit(rs.product_line.L500)
device = devices[0]
depth_sensor = device.first_depth_sensor()

try:
    dp = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.depth)
    irp = next(p for p in depth_sensor.profiles if p.stream_type() == rs.stream.infrared)
except RuntimeError as e:
    test.unexpected_exception()
    test.finish()

depth_frame_cnt = 0
ir_frame_cnt = 0


def frames_counter(frame):
    if frame.get_profile().stream_type() == rs.stream.depth:
        global depth_frame_cnt
        depth_frame_cnt += 1
    elif frame.get_profile().stream_type() == rs.stream.infrared:
        global ir_frame_cnt
        ir_frame_cnt += 1

# Test Part 1
test.start("Validate frame filter - depth only")
depth_sensor.open(dp)
depth_sensor.start(frames_counter)
time.sleep(10) # wait for frames to arrive

test.check(depth_frame_cnt != 0)
test.check_equal(ir_frame_cnt, 0)
depth_sensor.stop()
depth_sensor.close()

test.finish()

depth_frame_cnt = 0
ir_frame_cnt = 0

# Test Part 2
test.start("Validate frame filter - depth + ir")
depth_sensor.open([dp, irp])
depth_sensor.start(frames_counter)
time.sleep(10) # wait for frames to arrive

test.check(depth_frame_cnt != 0)
test.check(ir_frame_cnt != 0)
test.finish()


test.print_results_and_exit()