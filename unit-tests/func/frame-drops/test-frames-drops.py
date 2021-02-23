# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

""" 
The following tests check for frame drops when we use several streams with different frame rate (FPS).
Each frame has it's own latency, the syncer and the pipeline should be able to handle all frames with no frame drops.
"""

import pyrealsense2 as rs
from rspy import test
from rspy.timer import Timer
import time

WAIT_FOR_FRAMES_TIME = 20 #[sec]

dev = test.find_first_device_or_exit()

depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

previous_depth_frame_number = -1
previous_color_frame_number = -1

wait_frames_timer = Timer(WAIT_FOR_FRAMES_TIME)

def check_depth_frame_drops(frame):
    global previous_depth_frame_number
    test.check_frame_drops(frame, previous_depth_frame_number, 0)
    previous_depth_frame_number = frame.get_frame_number()

def check_color_frame_drops(frame):
    global previous_color_frame_number
    test.check_frame_drops(frame, previous_color_frame_number, 0)
    previous_color_frame_number = frame.get_frame_number()


# Use a profiles with different FPS
high_fps_depth_profile = next(p for p in
                depth_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.depth
                and p.format() == rs.format.z16)

low_fps_color_profile = next(p for p in color_sensor.profiles if p.fps() == 6
                and p.stream_type() == rs.stream.color
                and p.format() == rs.format.rgb8)
                
#############################################################################################
# Test #1

test.start("Checking for frame drops when using sensor API")

depth_sensor.open( high_fps_depth_profile )
depth_sensor.start( check_depth_frame_drops )

color_sensor.open( low_fps_color_profile )
color_sensor.start( check_color_frame_drops )

time.sleep(WAIT_FOR_FRAMES_TIME)

# Check that we received frames
test.check(previous_depth_frame_number > 0)
test.check(previous_color_frame_number > 0)

depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

test.finish()

#############################################################################################
time.sleep(3) # allow some time for the sensor to closed before starting next test 
#############################################################################################

# Test #2

test.start("Checking for frame drops when using sensor + syncer API")

previous_depth_frame_number = -1
previous_color_frame_number = -1

syncer = rs.syncer()

depth_sensor.open( high_fps_depth_profile )
depth_sensor.start( syncer )

color_sensor.open( low_fps_color_profile )
color_sensor.start( syncer )

frame = syncer.wait_for_frames()
wait_frames_timer.start()

while not wait_frames_timer.has_expired():
    frame = syncer.wait_for_frames()
    
    depth_frame = frame.get_depth_frame()
    color_frame = frame.get_color_frame()
    
    # The syncer can output only 1 frame (and not a frameset) if it wasn't synced
    if depth_frame:
        test.check_frame_drops(depth_frame, previous_depth_frame_number, 0, True)
        previous_depth_frame_number = depth_frame.get_frame_number()

    if color_frame:
        test.check_frame_drops(color_frame, previous_color_frame_number, 0, True)
        previous_color_frame_number = color_frame.get_frame_number()

# Check that we received frames
test.check(previous_depth_frame_number > 0)
test.check(previous_color_frame_number > 0)

depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

test.finish()

#############################################################################################
time.sleep(3) # allow some time for the sensor to closed before starting next test 
#############################################################################################

# Test #3

test.start("Checking for frame drops when using pipeline API")

previous_depth_frame_number = -1
previous_color_frame_number = -1

pipeline = rs.pipeline()
config = rs.config()

config.enable_stream(rs.stream.depth, rs.format.z16, 30)
config.enable_stream(rs.stream.color, rs.format.rgb8, 6)

pipeline.start(config)

frame = pipeline.wait_for_frames()

wait_frames_timer.start()

while not wait_frames_timer.has_expired():

    frame = pipeline.wait_for_frames()
    
    # The pipeline always returns a frameset
    test.check_frame_drops(frame.get_depth_frame(), previous_depth_frame_number, 0, True)
    test.check_frame_drops(frame.get_color_frame(), previous_color_frame_number, 0, True)
    
    previous_depth_frame_number = frame.get_depth_frame().get_frame_number()
    previous_color_frame_number = frame.get_color_frame().get_frame_number()

# Check that we received frames
test.check(previous_depth_frame_number > 0)
test.check(previous_color_frame_number > 0)

pipeline.stop()

test.finish()
#############################################################################################

test.print_results_and_exit()
