## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#####################################################
##                   Frame Queues                  ##
#####################################################

# First import the library
import pyrealsense2 as rs
import time

# Implement two callbacks
def slow_callback(frame):
	n = frame.get_frame_number() 
	if n % 20 == 0:
		time.sleep(.5)
	print n
		
def slower_callback(frame):
	n = frame.get_frame_number() 
	if n % 20 == 0:
		time.sleep(1)
	print n

# Create a pipeline
pipeline = rs.pipeline()

#Create a config and configure the pipeline to stream
#  different resolutions of color and depth streams
config = rs.config()
config.enable_stream(rs.stream.depth, 640, 360, rs.format.z16, 30)
config.enable_stream(rs.stream.color, 640, 480, rs.format.bgr8, 30)

# Start streaming to the slow callback.
# This stream will lag, causing the occasional frame drop.
print("Slow callback")
pipeline.start(config, slow_callback)
pipeline.sleep(10)
pipeline.stop()

# Start streaming to the the slow callback by way of the frame queue.
# This stream will occasionally hiccup, but the frame_queue will prevent frame loss.
print("Slow callback + queue")
queue = rs.frame_queue(50)
pipeline.start(config, queue)
start = time.time()
while time.time() - start < 10:
	frames = queue.wait_for_frames()
	slow_callback(frames)
pipeline.stop()

# Start streaming to the the slower callback by way of the frame queue.
# This stream will drop frames because the delays are long enough that the backed up
# frames use the entire internal frame pool preventing the SDK from creating more frames.
print("Slower callback + queue")
queue = rs.frame_queue(50)
pipeline.start(config, queue)
start = time.time()
while time.time() - start < 10:
	frames = queue.wait_for_frames()
	slower_callback(frames)
pipeline.stop()

# Start streaming to the slower callback by way of a keeping frame queue.
# This stream will no longer drop frames because the frame queue tells the SDK
# to remove the frames it holds from the internal frame queue, allowing the SDK to
# allocate space for and create more frames .
print("Slower callback + keeping queue")
queue = rs.frame_queue(50, keep_frames=True)
pipeline.start(config, queue)
start = time.time()
while time.time() - start < 10:
	frames = queue.wait_for_frames()
	slower_callback(frames)
pipeline.stop()