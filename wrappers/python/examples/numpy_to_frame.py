# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 RealSense, Inc. All Rights Reserved.

import numpy as np
import pyrealsense2 as rs
import cv2

W = 640
H = 480
BPP = {'depth': 2, 'color': 3}  # Bytes per pixel for z16 and bgr8


def create_video_stream(mode):
    vs = rs.video_stream()
    if mode == 'depth':
        vs.type, vs.fmt = rs.stream.depth, rs.format.z16
    else:
        vs.type, vs.fmt = rs.stream.color, rs.format.bgr8
    vs.width, vs.height = W, H
    # other properties can be set here (e.g., fps, uid)
    return vs


def create_frame(np_frame, mode, stream_profile):
    frame = rs.software_video_frame()
    frame.pixels = np_frame.data
    frame.bpp = BPP[mode]
    frame.profile = stream_profile.as_video_stream_profile()
    frame.stride = W * BPP[mode]
    # other frame properties can be set here (e.g., timestamp, frame number)
    return frame


class NumpyToFrame:
    def __init__(self, mode='depth'):
        """
        mode: 'depth' or 'color'
        """
        self.mode = mode.lower()
        if self.mode not in BPP:
            raise ValueError("Invalid mode. Choose 'depth' or 'color'.")

        self.queue = rs.frame_queue(100)
        self.dev = rs.software_device()
        self.sensor = self.dev.add_sensor(self.mode.capitalize())  # "Depth" or "Color"
        self.stream = self.sensor.add_video_stream(create_video_stream(self.mode))

        self.sensor.open(self.stream)
        self.sensor.start(self.queue)

    def __del__(self):
        self.sensor.stop()
        self.sensor.close()

    def convert(self, numpy_array):
        # Inject the array into the sensor
        self.sensor.on_video_frame(create_frame(numpy_array, self.mode, self.stream))
        # Get the array as a frame from the frame queue
        frame = self.queue.wait_for_frame()
        return frame.as_depth_frame() if self.mode == 'depth' else frame.as_video_frame()


# =========================================================================
# Generally, the idea is to pass the numpy array through a sw device and stream it to get a frame , the flow is:
# Frame -> numpy array (and modifications) -> frame (using software device)
# =========================================================================
# Setup to get depth and color frames for the example
pipeline = rs.pipeline()
config = rs.config()
config.enable_stream(rs.stream.depth, W, H, rs.format.z16, 30)
config.enable_stream(rs.stream.color, W, H, rs.format.bgr8, 30)
pipeline.start(config)
frameset = pipeline.wait_for_frames()
depth_frame = frameset.get_depth_frame()
color_frame = frameset.get_color_frame()
pipeline.stop()
# =========================================================================
# Example for depth frame with horizontal flip
np_depth = np.asanyarray(depth_frame.get_data())
modified_depth = np.ascontiguousarray(np_depth[:, ::-1])

numpy_to_depth_frame = NumpyToFrame(mode='depth')
converted_depth_frame = numpy_to_depth_frame.convert(modified_depth)

# for depth, we can use the modified frame with calculate():
pc = rs.pointcloud()
pc.calculate(converted_depth_frame)
print("Depth conversion test:", np.array_equal(modified_depth, np.asanyarray(converted_depth_frame.get_data())))

# =========================================================================
# Example for color frame with horizontal flip
np_color = np.asanyarray(color_frame.get_data())
modified_color = np.ascontiguousarray(np_color[:, ::-1])

numpy_to_color_frame = NumpyToFrame(mode='color')
converted_color_frame = numpy_to_color_frame.convert(modified_color)
print("Color conversion test:", np.array_equal(modified_color, np.asanyarray(converted_color_frame.get_data())))

cv2.imshow("Color Frame Conversion - Original vs converted",
           cv2.hconcat([np_color, np.ones((np_color.shape[0], 50, np_color.shape[2]), dtype=np_color.dtype),
                        np.asanyarray(converted_color_frame.get_data())]))
cv2.waitKey(0)
