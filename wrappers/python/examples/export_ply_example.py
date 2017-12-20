## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#####################################################
##              Align Depth to Color               ##
#####################################################

# First import the library
import pyrealsense2 as rs


# Declare pointcloud object, for calculating pointclouds and texture mappings
pc = rs.pointcloud()
# We want the points object to be persistent so we can display the last cloud when a frame drops
points = rs.points()

# Declare RealSense pipeline, encapsulating the actual device and sensors
pipe = rs.pipeline()
#Start streaming with default recommended configuration
pipe.start();

c = rs.colorizer()

# Streaming loop
try:
    while True:
        # Wait for the next set of frames from the camera
        frames = pipe.wait_for_frames();

        depth = frames.get_depth_frame();

        # Generate the pointcloud and texture mappings
        points = pc.calculate(depth);

        color = c.colorize(depth);

        # Tell pointcloud object to map to this color frame
        pc.map_to(color);

        export_to_ply("1.ply", color)

finally:
    pipeline.stop()