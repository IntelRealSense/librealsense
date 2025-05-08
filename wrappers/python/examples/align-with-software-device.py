## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#####################################################################################################
#                                                                                                  ##
#    Align depth to color with precaptured images in software device                               ##
#                                                                                                  ##
##  Purpose                                                                                        ##
##    This example first captures depth and color images from realsense camera and then            ##
##    demonstrate align depth to color with the precaptured images in software device              ##
##                                                                                                 ##
##  Steps:                                                                                         ##
##    1) stream realsense camera with depth 640x480@30fps and color 1280x720@30fps                 ##
##    2) capture camera depth and color intrinsics and extrinsics                                  ##
##    3) capture depth and color images and save into files in npy format                          ##
##    4) construct software device from the saved intrinsics, extrinsics, depth and color images   ##
##    5) align the precaptured depth image to to color image                                       ##
##                                                                                                 ##
#####################################################################################################

import cv2
import pyrealsense2 as rs
import numpy as np
import os
import time

fps = 30                  # frame rate
tv = 1000.0 / fps         # time interval between frames in miliseconds

max_num_frames  = 100      # max number of framesets to be captured into npy files and processed with software device

depth_file_name = "depth"  # depth_file_name + str(i) + ".npy"
color_file_name = "color"  # color_file_name + str(i) + ".npy"

# intrinsic and extrinsic from the camera
camera_depth_intrinsics          = rs.intrinsics()  # camera depth intrinsics
camera_color_intrinsics          = rs.intrinsics()  # camera color intrinsics
camera_depth_to_color_extrinsics = rs.extrinsics()  # camera depth to color extrinsics


######################## start of first part - capture images from live device #######################################
# stream depth and color on attached realsnese camera and save depth and color frames into files with npy format
try:
    # create a context object, this object owns the handles to all connected realsense devices
    ctx = rs.context()
    devs = list(ctx.query_devices())
    
    if len(devs) > 0:
        print("Devices: {}".format(devs))
    else:
        print("No camera detected. Please connect a realsense camera and try again.")
        exit(0)
    
    pipeline = rs.pipeline()

    # configure streams
    config = rs.config()
    config.enable_stream(rs.stream.depth)
    config.enable_stream(rs.stream.color)

    # start streaming
    cfg = pipeline.start(config)
    
    # get scale
    depth_sensor = cfg.get_device().first_depth_sensor()
    depth_scale = depth_sensor.get_depth_scale()

    # get intrinsics
    camera_depth_profile = cfg.get_stream(rs.stream.depth)                                      # fetch depth depth stream profile
    camera_depth_intrinsics = camera_depth_profile.as_video_stream_profile().get_intrinsics()   # downcast to video_stream_profile and fetch intrinsics
    
    camera_color_profile = cfg.get_stream(rs.stream.color)                                      # fetch color stream profile
    camera_color_intrinsics = camera_color_profile.as_video_stream_profile().get_intrinsics()   # downcast to video_stream_profile and fetch intrinsics
    
    camera_depth_to_color_extrinsics = camera_depth_profile.get_extrinsics_to(camera_color_profile)
 

    print("camera depth intrinsic:", camera_depth_intrinsics)
    print("camera color intrinsic:", camera_color_intrinsics)
    print("camera depth to color extrinsic:", camera_depth_to_color_extrinsics)

    print("streaming attached camera and save depth and color frames into files in npy format ...")

    i = 0
    while i < max_num_frames:
        # wait until a new coherent set of frames is available on the device
        frames = pipeline.wait_for_frames()
        depth = frames.get_depth_frame()
        color = frames.get_color_frame()

        # Validate that both frames are valid
        if not depth or not color:
            continue
        
        # convert images to numpy arrays
        depth_image = np.asanyarray(depth.get_data())
        color_image = np.asanyarray(color.get_data())
        # save images in npy format
        depth_file = depth_file_name + str(i) + ".npy"
        color_file = color_file_name + str(i) + ".npy"
        print("saving frame set ", i, depth_file, color_file)
        
        with open(depth_file, 'wb') as f1:
            np.save(f1,depth_image)
        
        with open(color_file, 'wb') as f2:
            np.save(f2,color_image)

        # next frameset
        i = i +1

except Exception as e:
    logging.error("An error occurred: %s", e, exc_info=True)
    exit(1)

######################## end of first part - capture images from live device #######################################



######################## start of second part - align depth to color in software device #############################
# align depth to color with the above precaptured images in software device

# software device
sdev = rs.software_device()

# software depth sensor
depth_sensor: rs.software_sensor = sdev.add_sensor("Depth")

# depth instrincis
depth_intrinsics = rs.intrinsics()

depth_intrinsics.width  = camera_depth_intrinsics.width
depth_intrinsics.height = camera_depth_intrinsics.height

depth_intrinsics.ppx = camera_depth_intrinsics.ppx
depth_intrinsics.ppy = camera_depth_intrinsics.ppy

depth_intrinsics.fx = camera_depth_intrinsics.fx
depth_intrinsics.fy = camera_depth_intrinsics.fy

depth_intrinsics.coeffs = camera_depth_intrinsics.coeffs       ## [0.0, 0.0, 0.0, 0.0, 0.0]
depth_intrinsics.model = camera_depth_intrinsics.model         ## rs.pyrealsense2.distortion.brown_conrady

#depth stream
depth_stream = rs.video_stream()
depth_stream.type = rs.stream.depth
depth_stream.width = depth_intrinsics.width
depth_stream.height = depth_intrinsics.height
depth_stream.fps = fps
depth_stream.bpp = 2                              # depth z16 2 bytes per pixel
depth_stream.fmt = rs.format.z16
depth_stream.intrinsics = depth_intrinsics
depth_stream.index = 0
depth_stream.uid = 1

depth_profile = depth_sensor.add_video_stream(depth_stream)

# software color sensor
color_sensor: rs.software_sensor = sdev.add_sensor("Color")

# color intrinsic:
color_intrinsics = rs.intrinsics()
color_intrinsics.width = camera_color_intrinsics.width
color_intrinsics.height = camera_color_intrinsics.height

color_intrinsics.ppx = camera_color_intrinsics.ppx
color_intrinsics.ppy = camera_color_intrinsics.ppy

color_intrinsics.fx = camera_color_intrinsics.fx
color_intrinsics.fy = camera_color_intrinsics.fy

color_intrinsics.coeffs = camera_color_intrinsics.coeffs
color_intrinsics.model = camera_color_intrinsics.model

color_stream = rs.video_stream()
color_stream.type = rs.stream.color
color_stream.width = color_intrinsics.width
color_stream.height = color_intrinsics.height
color_stream.fps = fps
color_stream.bpp = 3                                # color stream rgb8 3 bytes per pixel in this example
color_stream.fmt = rs.format.rgb8
color_stream.intrinsics = color_intrinsics
color_stream.index = 0
color_stream.uid = 2

color_profile = color_sensor.add_video_stream(color_stream)

# depth to color extrinsics     auto z_to_other = depth_profile.get_extrinsics_to(other_profile);
depth_to_color_extrinsics = rs.extrinsics()
depth_to_color_extrinsics.rotation = camera_depth_to_color_extrinsics.rotation
depth_to_color_extrinsics.translation = camera_depth_to_color_extrinsics.translation
depth_profile.register_extrinsics_to(color_profile, depth_to_color_extrinsics)

# start software sensors
depth_sensor.open(depth_profile)
color_sensor.open(color_profile)

# syncronize frames from depth and color streams
camera_syncer = rs.syncer()
depth_sensor.start(camera_syncer)
color_sensor.start(camera_syncer)

# create a depth alignment object
# rs.align allows us to perform alignment of depth frames to others frames
# the "align_to" is the stream type to which we plan to align depth frames
# align depth frame to color frame
align_to = rs.stream.color
align = rs.align(align_to)

# colorizer for depth rendering
colorizer = rs.colorizer()

# use "Enter", "Spacebar", "p", keys to pause for 5 seconds
paused = False

# loop through pre-captured frames
for i in range(0, max_num_frames):
    print("\nframe set:", i)
    
    # precaptured depth and color image files in npy format
    df = depth_file_name + str(i) + ".npy"
    cf = color_file_name + str(i) + ".npy"

    if (not os.path.exists(cf)) or (not os.path.exists(df)): continue

    # load depth frame from precaptured npy file
    print('loading depth frame ', df)
    depth_npy = np.load(df, mmap_mode='r')

    # create software depth frame
    depth_swframe = rs.software_video_frame()
    depth_swframe.stride = depth_stream.width * depth_stream.bpp
    depth_swframe.bpp = depth_stream.bpp
    depth_swframe.timestamp = i * tv
    depth_swframe.pixels = depth_npy
    depth_swframe.domain = rs.timestamp_domain.hardware_clock
    depth_swframe.frame_number = i
    depth_swframe.profile = depth_profile.as_video_stream_profile()
    depth_swframe.depth_units = depth_scale
    depth_sensor.on_video_frame(depth_swframe)

    # load color frame from precaptured npy file
    print('loading color frame ', cf)
    color_npy = np.load(cf, mmap_mode='r')
 
    # create software color frame
    color_swframe = rs.software_video_frame()
    color_swframe.stride = color_stream.width * color_stream.bpp
    color_swframe.bpp = color_stream.bpp
    color_swframe.timestamp = i * tv
    color_swframe.pixels = color_npy
    color_swframe.domain = rs.timestamp_domain.hardware_clock
    color_swframe.frame_number = i
    color_swframe.profile = color_profile.as_video_stream_profile()
    color_sensor.on_video_frame(color_swframe)
    
    # synchronize depth and color, receive as frameset
    frames = camera_syncer.wait_for_frames()
    print("frame set:", frames.size(), " ", frames)

    # get unaligned depth frame
    unaligned_depth_frame = frames.get_depth_frame()
    if not unaligned_depth_frame: continue

    # align depth frame to color 
    aligned_frames = align.process(frames)

    aligned_depth_frame = aligned_frames.get_depth_frame()
    color_frame = aligned_frames.get_color_frame()

    if (not aligned_depth_frame) or (not color_frame): continue

    aligned_depth_frame = colorizer.colorize(aligned_depth_frame)    
    npy_aligned_depth_image = np.asanyarray(aligned_depth_frame.get_data())

    npy_color_image = np.asanyarray(color_frame.get_data())

    # render aligned images:
    # depth align to color
    # aligned depth on left
    # color on right    
    images = np.hstack((npy_aligned_depth_image, npy_color_image))
    cv2.namedWindow('Align Example', cv2.WINDOW_NORMAL)
    cv2.imshow('Align Example', images)
    key = cv2.waitKey(1)

    # render original unaligned depth as reference
    # colorized_unaligned_depth_frame = colorizer.colorize(unaligned_depth_frame)
    # npy_unaligned_depth_image = np.asanyarray(colorized_unaligned_depth_frame.get_data())
    # cv2.imshow("Unaligned Depth", npy_unaligned_depth_image)
    
    # press ENTER or SPACEBAR key to pause the image window for 5 seconds

    if key == 13 or key == 32: paused = not paused
        
    if paused:
        print("Paused for 5 seconds ...", i, ", press ENTER or SPACEBAR key anytime for additional pauses.")
        time.sleep(5)
        paused = not paused

# end of second part - align depth to color with the precaptured images in software device
######################## End of second part - align depth to color in software device #############################
    
cv2.destroyAllWindows()