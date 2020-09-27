import pyrealsense2 as rs
import numpy as np
import cv2
import tensorflow as tf
from tensorflow import keras
#from object_detection.utils import label_map_util
#from object_detection.utils import visualization_utils as vis_util

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
#model = keras.models.load_model(PB_PATH)

# Path to frozen detection graph .pb file, which contains the model that is used
# for object detection.
PATH_TO_CKPT = r"C:\work\git\tensorflow\model\frozen_inference_graph.pb"

# Path to label map file
PATH_TO_LABELS = r"C:\work\git\tensorflow\model\faster_rcnn_inception_v2_coco_2018_01_28.pbtxt"


# Number of classes the object detector can identify
NUM_CLASSES = 2

# Load the label map.
# Label maps map indices to category names, so that when our convolution
# network predicts `5`, we know that this corresponds to `king`.
# Here we use internal utility functions, but anything that returns a
# dictionary mapping integers to appropriate string labels would be fine
# = label_map_util.load_labelmap(PATH_TO_LABELS)
#categories = label_map_util.convert_label_map_to_categories(
#    label_map, max_num_classes=NUM_CLASSES, use_display_name=True)
#category_index = label_map_util.create_category_index(categories)

# Load the Tensorflow model into memory.
detection_graph = tf.Graph()
with detection_graph.as_default():
    od_graph_def = tf.compat.v1.GraphDef()
    with tf.compat.v1.gfile.GFile(PATH_TO_CKPT, 'rb') as fid:
        serialized_graph = fid.read()
        od_graph_def.ParseFromString(serialized_graph)
        tf.compat.v1.import_graph_def(od_graph_def, name='')

    sess = tf.compat.v1.Session(graph=detection_graph)

# Input tensor is the image
image_tensor = detection_graph.get_tensor_by_name('image_tensor:0')

# Output tensors are the detection boxes, scores, and classes
# Each box represents a part of the image where a particular object was detected
detection_boxes = detection_graph.get_tensor_by_name('detection_boxes:0')

# Each score represents level of confidence for each of the objects.
# The score is shown on the result image, together with the class label.
detection_scores = detection_graph.get_tensor_by_name('detection_scores:0')
detection_classes = detection_graph.get_tensor_by_name('detection_classes:0')

# Number of objects detected
num_detections = detection_graph.get_tensor_by_name('num_detections:0')

while True:
    frames = pipeline.wait_for_frames()
    frames = aligned_stream.process(frames)
    depth_frame = frames.get_depth_frame()
    color_frame = frames.get_color_frame()
    points = point_cloud.calculate(depth_frame)
    verts = np.asanyarray(points.get_vertices()).view(np.float32).reshape(-1, W, 3)  # xyz

    # Convert images to numpy arrays
    color_image = np.asanyarray(color_frame.get_data())


    color_image = np.asanyarray(color_frame.get_data())
    scaled_size = (int(W), int(H))
    #net.setInput(cv2.dnn.blobFromImage(color_image, size=scaled_size, swapRB=True, crop=False))
    #detections = net.forward()
    #detections = model.predict(color_image)

    image_tensor = detection_graph.get_tensor_by_name('image_tensor:0')
    detection_boxes = detection_graph.get_tensor_by_name('detection_boxes:0')
    detection_scores = detection_graph.get_tensor_by_name('detection_scores:0')
    detection_classes = detection_graph.get_tensor_by_name('detection_classes:0')
    num_detections = detection_graph.get_tensor_by_name('num_detections:0')
    image_expanded = np.expand_dims(color_image, axis=0)

    # Perform the actual detection by running the model with the image as input
    (boxes, scores, classes, num) = sess.run(
        [detection_boxes, detection_scores, detection_classes, num_detections],
        feed_dict={image_tensor: image_expanded})
    #index_array = np.where(scores>0)[1]

    boxes = np.squeeze(boxes)
    classes = np.squeeze(classes).astype(np.int32)
    scores = np.squeeze(scores)
    index_array = list(np.where(scores > 0))
    # Draw the results of the detection (aka 'visualize the results')
    for i in range(int(num)):
        idx = index_array[0][i]
        class_ = classes[idx]
        score = scores[idx]
        bbox = boxes[idx]
        if score > 0.4:
            # draw box
            #r, g, b = tuple(np.random.choice(range(256), size=3)) #colors_list[idx%61]
            #cv2.rectangle(color_image, p1, p2, (int(r), int(g), int(b)), 2, 1)
            #cv2.rectangle(color_image, (box[0], box[1]), (box[2], box[3]), (225,0,0), 2)
            p1 = (int(bbox[0]*10), int(bbox[1]*10))
            p2 = (int(bbox[2]*10), int(bbox[3]*10))
            # draw box
            cv2.rectangle(color_image, p1, p2, (225,0,0), 2, 1)

    #colors_list = []
    #for i in range(61):
    #    colors_list.append(tuple(np.random.choice(range(256), size=3)))

    print("[INFO] drawing bounding box on detected objects...")
    print("[INFO] each detected object has a unique color")


    #for detection in detections[0,0]:
    #    score = float(detection[2])
    #    idx = int(detection[1])

    #    if score > 0.4:
    #        left = detection[3] * W
    #        top = detection[4] * H
    #        right = detection[5] * W
    #        bottom = detection[6] * H
    #        width = right - left
    #        height = bottom - top

    #        bbox = (int(left), int(top), int(width), int(height))

    #        p1 = (int(bbox[0]), int(bbox[1]))
    #        p2 = (int(bbox[0] + bbox[2]), int(bbox[1] + bbox[3]))
    #        # draw box
    #        r, g, b = colors_list[idx%61]
    #        cv2.rectangle(color_image, p1, p2, (int(r), int(g), int(b)), 2, 1)

    print("[INFO] displaying image ...")
    cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
    cv2.imshow('RealSense', color_image)
    cv2.waitKey(1)

print("[INFO] stop streaming ...")
pipeline.stop()