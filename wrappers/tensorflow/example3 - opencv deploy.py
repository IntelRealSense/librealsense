import pyrealsense2 as rs
import numpy as np
import cv2

W = 848
H = 480

# Configure depth and color streams
pipeline = rs.pipeline()
config = rs.config()
config.enable_stream(rs.stream.depth, W, H, rs.format.z16, 30)
config.enable_stream(rs.stream.color, W, H, rs.format.bgr8, 30)


# Start streaming
# add prints ..
pipeline.start(config)

a = rs.align(rs.stream.color) # align between color and depth
pc = rs.pointcloud()

print("[INFO] loading model...")
net = cv2.dnn.readNetFromTensorflow(r"C:\work\git\tensorflow\model\frozen_inference_graph.pb", r"C:\work\git\tensorflow\model\faster_rcnn_inception_v2_coco_2018_01_28.pbtxt.txt")
while True:
    frames = pipeline.wait_for_frames()
    frames = a.process(frames)
    depth_frame = frames.get_depth_frame()
    color_frame = frames.get_color_frame()
    points = pc.calculate(depth_frame)
    verts = np.asanyarray(points.get_vertices()).view(np.float32).reshape(-1, W, 3)  # xyz

    # Convert images to numpy arrays
    depth_image = np.asanyarray(depth_frame.get_data())
    color_image = np.asanyarray(color_frame.get_data())


    color_image = np.asanyarray(color_frame.get_data())
    scaled_size = (int(W), int(H))
    #color_image_small = cv2.resize(color_image, scaled_size) # needed to improve performance
    net.setInput(cv2.dnn.blobFromImage(color_image, size=scaled_size, swapRB=True, crop=False))
    detections = net.forward()

    colors_list = []
    for i in range(61):
        colors_list.append(tuple(np.random.choice(range(256), size=3)))

    print("[INFO] drawing bounding box on detected objects...")
    #print("[INFO] each detected object has a unique color") for ex1
    for detection in detections[0,0]:
        score = float(detection[2])
        idx = int(detection[1])

        if score > 0.4 and idx == 0:# and (idx == 0 or idx == 61): for ex2/3 idx=0, for ex1 idx = 0-61
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
            #r, g, b = colors_list[idx%61] for ex1 we need different color for each object
            #cv2.rectangle(color_image, p1, p2, (int(r), int(g), int(b)), 2, 1) # for ex1
            cv2.rectangle(color_image, p1, p2, (255, 0, 0), 2, 1) # red.. pick different color for each idx (define array of colors.. use modolu)

            # x,y,z of bounding box
            obj_points = verts[int(bbox[1]):int(bbox[1] + bbox[3]), int(bbox[0]):int(bbox[0] + bbox[2])].reshape(-1, 3)
            zs = obj_points[:,2]

            z = np.median(zs)

            # x is not relevant when calc height
            ys = obj_points[:,1]

            #xs = np.delete(xs, np.where((zs < z - 1) | (zs > z + 1)))
            ys = np.delete(ys, np.where((zs < z - 1) | (zs > z + 1))) # take only y for close z to prevent including background

            my = np.amin(ys, initial=1)
            My = np.amax(ys, initial=-1)

            print("[INFO] calculating height of object ", idx,"...")
            height = My - my # add next to rectangle print of height using cv library

    # Show images
    cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
    cv2.imshow('RealSense', color_image)
    cv2.waitKey(1)

    #cv2.namedWindow("RealSense", cv2.WND_PROP_FULLSCREEN)
    #cv2.setWindowProperty("RealSense", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)
    #cv2.imshow('RealSense', color_image)
    #cv2.waitKey(1)


# Stop streaming
pipeline.stop()