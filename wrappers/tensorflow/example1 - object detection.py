import pyrealsense2 as rs
import numpy as np
import cv2
import tensorflow as tf
from tensorflow import keras
from skimage import img_as_uint
import time, sys

W = 848
H = 480

# Configure depth and color streams
pipeline = rs.pipeline()
config = rs.config()
config.enable_stream(rs.stream.depth, W, H, rs.format.z16, 30)
config.enable_stream(rs.stream.color, W, H, rs.format.bgr8, 30)


print("[INFO] start streaming...")
pipeline.start(config)

aligned_stream = rs.align(rs.stream.color)  #align between color and depth
point_cloud = rs.pointcloud()

print("[INFO] loading model...")
PB_PATH = r"C:\Users\nyassin\Downloads\faster_rcnn_inception_v2_coco_2018_01_28.tar\faster_rcnn_inception_v2_coco_2018_01_28\saved_model"
model = keras.models.load_model(PB_PATH)

while True:
    frames = pipeline.wait_for_frames()
    frames = aligned_stream.process(frames)
    depth_frame = frames.get_depth_frame()
    color_frame = frames.get_color_frame()
    points = point_cloud.calculate(depth_frame)
    verts = np.asanyarray(points.get_vertices()).view(np.float32).reshape(-1, W, 3)  # xyz

    # Convert images to numpy arrays
    depth_image = np.asanyarray(depth_frame.get_data())
    color_image = np.asanyarray(color_frame.get_data())


    color_image = np.asanyarray(color_frame.get_data())
    scaled_size = (int(W), int(H))
    #net.setInput(cv2.dnn.blobFromImage(color_image, size=scaled_size, swapRB=True, crop=False))
    #detections = net.forward()
    detections = model.predict(color_image)

    colors_list = []
    for i in range(61):
        colors_list.append(tuple(np.random.choice(range(256), size=3)))

    print("[INFO] drawing bounding box on detected objects...")
    print("[INFO] each detected object has a unique color")
    for detection in detections[0,0]:
        score = float(detection[2])
        idx = int(detection[1])

        if score > 0.4 :
            left = detection[3] * W
            top = detection[4] * H
            right = detection[5] * W
            bottom = detection[6] * H
            width = right - left
            height = bottom - top

            bbox = (int(left), int(top), int(width), int(height))

            p1 = (int(bbox[0]), int(bbox[1]))
            p2 = (int(bbox[0] + bbox[2]), int(bbox[1] + bbox[3]))
            # draw box
            r, g, b = colors_list[idx%61]
            cv2.rectangle(color_image, p1, p2, (int(r), int(g), int(b)), 2, 1)


    print("[INFO] displaying image ...")
    cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
    cv2.imshow('RealSense', color_image)
    cv2.waitKey(1)

print("[INFO] stop streaming ...")
pipeline.stop()