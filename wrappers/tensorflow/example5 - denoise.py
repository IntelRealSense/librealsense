import pyrealsense2 as rs
import numpy as np
import cv2
from tensorflow import keras
import time, sys

# Configure depth and color streams
pipeline = rs.pipeline()
config = rs.config()
config.enable_stream(rs.stream.depth, 848, 480, rs.format.z16, 30)
config.enable_stream(rs.stream.infrared, 1, 848, 480, rs.format.y8, 30) # 1 for left frame
# Start streaming
pipeline.start(config)

channels = 2
cropped_w, cropped_h = 480, 480

test_model_name = ""
if (len(sys.argv) > 1):
    test_model_name = str(sys.argv[1])

t1 = time.perf_counter()
model = keras.models.load_model(test_model_name)
t2 = time.perf_counter()
print('model loading : ', t2 - t1, 'seconds')

def predict(noisy_image, ir_image):
    t1 = time.perf_counter()
    ir_image = np.array(ir_image).astype("uint16")
    cropped_ir , cropped_noisy = [], []
    width, height = 848, 480
    w, h = cropped_w, cropped_h
    for col_i in range(0, width, w):
        for row_i in range(0, height, h):
            cropped_ir.append(ir_image[row_i:row_i+h, col_i:col_i+w])
            cropped_noisy.append(noisy_image[row_i:row_i+h, col_i:col_i+w])

    # fill with zero to get size 480x480 for both images
    fill = np.zeros((h, w - cropped_ir[-1].shape[1]), dtype="uint16")
    cropped_ir[-1] = np.hstack((cropped_ir[-1], fill))
    cropped_noisy[-1] = np.hstack((cropped_noisy[-1], fill))
    t2 = time.perf_counter()
    print('image cropping : ', t2 - t1, 'seconds')

    cropped_image_offsets = [(0,0), (0,480)]
    whole_image = np.zeros((height, width, channels), dtype="float32")

    for i in range(len(cropped_ir)):
        t1 = time.perf_counter()
        noisy_images_plt = cropped_noisy[i].reshape(1, cropped_w, cropped_h, 1)
        ir_images_plt = cropped_ir[i].reshape(1, cropped_w, cropped_h, 1)
        im_and_ir = np.stack((noisy_images_plt, ir_images_plt), axis=3)
        im_and_ir = im_and_ir.reshape(1, cropped_w, cropped_h, channels)
        img = np.array(im_and_ir)
        # Parse numbers as floats
        img = img.astype('float32')

        # Normalize data : remove average then devide by standard deviation
        img = img / 65535
        sample = img
        row, col = cropped_image_offsets[i]
        t2 = time.perf_counter()
        print('image channeling : ', t2 - t1, 'seconds')

        t1 = time.perf_counter()
        denoised_image = model.predict(sample)
        t2 = time.perf_counter()
        print('prediction only : ', t2 - t1, 'seconds')
        row_end = row + cropped_h
        col_end = col + cropped_w
        denoised_row = cropped_h
        denoised_col = cropped_w
        if row + cropped_h >= height:
            row_end = height - 1
            denoised_row = abs(row - row_end)
        if col + cropped_w >= width:
            col_end = width - 1
            denoised_col = abs(col - col_end)
        # combine tested images
        whole_image[row:row_end, col:col_end] = denoised_image[:, 0:denoised_row, 0:denoised_col, :]
    return whole_image[:, :, 0]

#=============================================================================================================
def convert_image(i):
    m = np.min(i)
    M = np.max(i)
    i = np.divide(i, np.array([M - m], dtype=np.float)).astype(np.float)
    i = (i - m).astype(np.float)
    i8 = (i * 255.0).astype(np.uint8)
    if i8.ndim == 3:
        i8 = cv2.cvtColor(i8, cv2.COLOR_BGRA2GRAY)
    i8 = cv2.equalizeHist(i8)
    colorized = cv2.applyColorMap(i8, cv2.COLORMAP_JET)
    colorized[i8 == int(m)] = 0
    font = cv2.FONT_HERSHEY_SIMPLEX
    m = float("{:.2f}".format(m))
    M = float("{:.2f}".format(M))
    colorized = cv2.putText(colorized, str(m) + " .. " + str(M) + "[m]", (20, 50), font, 1, (255, 255, 255), 2, cv2.LINE_AA)
    return colorized


try:
    c = rs.colorizer()
    while True:
        print("==============================================================")
        t0 = time.perf_counter()
        # Wait for a coherent pair of frames: depth and ir
        t1 = time.perf_counter()
        frames = pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        ir_frame = frames.get_infrared_frame()
        t2 = time.perf_counter()
        print('getting depth + ir frames : ', t2 - t1, 'seconds')

        if not depth_frame or not ir_frame:
            continue

        # Convert images to numpy arrays
        t1 = time.perf_counter()
        depth_image = np.asanyarray(depth_frame.get_data())
        ir_image = np.asanyarray(ir_frame.get_data())
        t2 = time.perf_counter()
        print('convert frames to numpy arrays : ', t2 - t1, 'seconds')

        t1 = time.perf_counter()
        predicted_image = predict(depth_image, ir_image)
        t2 = time.perf_counter()
        print('processing + prediction : ', t2 - t1, 'seconds')

        # Stack both images horizontally
        # depth_image = convert_image(depth_image)
        t1 = time.perf_counter()
        depth_image = np.asanyarray(c.process(depth_frame).get_data())
        predicted_image = convert_image(predicted_image)

        red = depth_image[:, :, 2].copy()
        blue = depth_image[:, :, 0].copy()
        depth_image[:, :, 0] = red
        depth_image[:, :, 2] = blue
        images = np.hstack((depth_image, predicted_image))

        # Show images
        cv2.namedWindow('RealSense', cv2.WINDOW_AUTOSIZE)
        cv2.imshow('RealSense', images)
        cv2.waitKey(1)
        t2 = time.perf_counter()
        print('show image : ', t2 - t1, 'seconds')
        print('TOTAL TIME : ', t2 - t0, 'seconds')

finally:

    # Stop streaming
    pipeline.stop()
