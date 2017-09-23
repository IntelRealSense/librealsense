import pyrealsense2 as rs
import numpy as np
import cv2

if __name__ == '__main__':
    pipeline = rs.pipeline()
    pipeline.enable_stream(rs.stream.depth, 0, 640, 480, rs.format.z16, 30)
    pipeline.enable_stream(rs.stream.color, 0, 640, 480, rs.format.bgr8, 30)
    pipeline.start()
    try:
        while True:
            frames = pipeline.wait_for_frames()
            depth_frame = frames.get_depth_frame()
            color_frame = frames.get_color_frame()
            if not depth_frame or not color_frame:
                continue
            depth_image = np.asanyarray(depth_frame.get_data())
            color_image = np.asanyarray(color_frame.get_data())
            cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
            both = np.hstack((color_image, cv2.applyColorMap(cv2.convertScaleAbs(depth_image, None, 0.5, 0), cv2.COLORMAP_JET)))
            cv2.imshow('RealSense', both)
            cv2.waitKey(1)
    finally:
        pipeline.stop()