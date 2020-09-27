import pyrealsense2 as rs
import numpy as np
import cv2
from tensorflow import keras
import tensorflow as tf
W = 848
H = 480

# Configure depth and color streams
pipeline = rs.pipeline()
config = rs.config()
config.enable_stream(rs.stream.depth, W, H, rs.format.z16, 30)
config.enable_stream(rs.stream.color, W, H, rs.format.bgr8, 30)


print("[INFO] start streaming...")
pipeline.start(config)

aligned_stream = rs.align(rs.stream.color) # alignment between color and depth
point_cloud = rs.pointcloud()

print("[INFO] loading model...")
#net = cv2.dnn.readNetFromTensorflow(r"C:\work\git\tensorflow\model\frozen_inference_graph.pb", r"C:\work\git\tensorflow\model\faster_rcnn_inception_v2_coco_2018_01_28.pbtxt.txt")
test_model_name = r"C:\work\git\tensorflow\model\faster_rcnn_inception_v2_coco_2018_01_28\saved_model"
#net = keras.models.load_model(test_model_name)
#net=tf.saved_model.load(export_dir=test_model_name)
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

    flag, bts = cv2.imencode('.jpg', color_image)
    inp = [bts[:, 0].tobytes()]

    with tf.compat.v1.Session(graph=tf.Graph()) as sess:
        tf.compat.v1.saved_model.loader.load(sess, ['serve'], test_model_name)
        graph = tf.compat.v1.get_default_graph()
        out = sess.run([sess.graph.get_tensor_by_name('num_detections:0'),
                        sess.graph.get_tensor_by_name('detection_scores:0'),
                        sess.graph.get_tensor_by_name('detection_boxes:0'),
                        sess.graph.get_tensor_by_name('detection_classes:0')],
                       #feed_dict={'encoded_image_string_tensor:0': inp})
                       feed_dict={'map/TensorArrayStack/TensorArrayGatherV3:0': inp})

        #sample = cv2.dnn.blobFromImage(color_image, size=scaled_size, swapRB=True, crop=False)
    #detections = net.predict(sample)

    colors_list = []
    for i in range(61):
        colors_list.append(tuple(np.random.choice(range(256), size=3)))

    print("[INFO] drawing bounding box on detected objects...")
    for detection in detections[0,0]:
        score = float(detection[2])
        idx = int(detection[1])

        if score > 0.4 and idx == 0:
            left = detection[3] * W
            top = detection[4] * H
            right = detection[5] * W
            bottom = detection[6] * H
            width = right - left
            height = bottom - top

            bbox = (int(left), int(top), int(width), int(height))

            p1 = (int(bbox[0]), int(bbox[1]))
            p2 = (int(bbox[0] + bbox[2]), int(bbox[1] + bbox[3]))
            cv2.rectangle(color_image, p1, p2, (255, 0, 0), 2, 1)

            # x,y,z of bounding box
            obj_points = verts[int(bbox[1]):int(bbox[1] + bbox[3]), int(bbox[0]):int(bbox[0] + bbox[2])].reshape(-1, 3)
            zs = obj_points[:,2]

            z = np.median(zs)

            ys = obj_points[:,1]
            ys = np.delete(ys, np.where((zs < z - 1) | (zs > z + 1))) # take only y for close z to prevent including background

            my = np.amin(ys, initial=1)
            My = np.amax(ys, initial=-1)

            height = My - my # add next to rectangle print of height using cv library
            print("[INFO] object height is: ", height)

            # Write some Text
            font = cv2.FONT_HERSHEY_SIMPLEX
            bottomLeftCornerOfText = (p1[0], p1[1]-10)
            fontScale = 1
            fontColor = (255, 255, 255)
            lineType = 2
            cv2.putText(color_image, str(height),
                        bottomLeftCornerOfText,
                        font,
                        fontScale,
                        fontColor,
                        lineType)

    # Show images
    cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
    cv2.imshow('RealSense', color_image)
    cv2.waitKey(1)

# Stop streaming
print("[INFO] stop streaming ...")
pipeline.stop()