#!/usr/bin/python
# -*- coding: utf-8 -*-
## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.
from __future__ import print_function

import pyrealsense2 as rs
import numpy as np
np.set_printoptions(suppress=True,precision=5)
import cv2
assert cv2.__version__[0] >= '3', 'The fisheye module requires opencv version >= 3.0.0'
import os
import json
import argparse
import glob
from collections import OrderedDict


parser = argparse.ArgumentParser()
parser.add_argument('--SN_T265', help='serial number of T265')
parser.add_argument('--SN_D4xx', help='serial number of D4xx')
parser.add_argument('--path', default="images", help='image path')
parser.add_argument('--grid_H', default=9, help='grid height (inner corners)')
parser.add_argument('--grid_W', default=6, help='grid width (inner corners)')
parser.add_argument('--size', default=0.020, help='grid side length')
parser.add_argument('--calibrate', default=False, help='run calibration (only)', action='store_true')
parser.add_argument('--visualize', default=True, help='with GUI', action='store_true')
args = parser.parse_args()
CHECKERBOARD = (args.grid_H, args.grid_W)
SIDE_LENGTH = args.size

def add_camera_calibration(intrinsics, streams = None):
    cam = {}
    cam['center_px'] = [intrinsics.ppx, intrinsics.ppy]
    cam['focal_length_px'] = [intrinsics.fx, intrinsics.fy]
    cam['distortion'] = {}
    cam['distortion']['type'] = 'kannalabrandt4'
    cam['distortion']['k'] = intrinsics.coeffs[:4]
    if streams:
        ext = streams["cam1"].get_extrinsics_to(streams["pose"])  # w.r.t.
        #print(ext)
        cam["extrinsics"] = {}
        cam["extrinsics"]["T"] = ext.translation
        #print(ext.rotation)
        cam["extrinsics"]["R"] = ext.rotation
    return cam

def save_calibration(directory, intrinsics, streams):
    D = OrderedDict()  # in order (cam1,cam2)
    D['cameras'] = []
    D['cameras'].append(add_camera_calibration(intrinsics["cam1"], streams))
    D['cameras'].append(add_camera_calibration(intrinsics["cam2"]))
    
    if not os.path.exists(directory):
        os.mkdir(directory)
    with open(directory + '/calibration.json', 'w') as f:
        json.dump(D, f, indent=4)


def read_calibration(cam, extrinsics = False):
    #print("read_calibration")
    # intrinsics
    K = np.array([[cam['focal_length_px'][0],                         0, cam['center_px'][0]],
                  [                        0, cam['focal_length_px'][1], cam['center_px'][1]],
                  [                        0,                         0,                   1]])
    D = np.array(cam['distortion']['k'])

    if extrinsics:
        H = np.eye(4)
        H[:3,:3] = np.reshape(cam["extrinsics"]["R"],(3,3))
        H[:3,3] = cam["extrinsics"]["T"]
        #print(H)
        return (K, D, H)
    return (K, D)

def load_calibration(directory):
    with open(directory + '/calibration.json', 'r') as f:
        D = json.load(f)
    
    (K1, D1, H1) = read_calibration(D['cameras'][0], True)
    (K2, D2) = read_calibration(D['cameras'][1])
    return (K1, D1, K2, D2, H1)
    

if not args.calibrate:
    if (not args.SN_T265 or not args.SN_D4xx):
        print("Specify serial numbers --SN_T265 and --SN_D4xx (for online calibration, or --calibrate for prerecorded images with --path path to folder)")
        exit()
    print("Trying to connect devices...")
    # cam 1
    pipe1 = rs.pipeline()
    cfg1 = rs.config()
    cfg1.enable_device(args.SN_T265)
    pipe1.start(cfg1)
    
    # cam 2
    pipe2 = rs.pipeline()
    cfg2 = rs.config()
    cfg2.enable_device(args.SN_D4xx)
    cfg2.enable_all_streams()
    pipe2_profile = pipe2.start(cfg2)
    sensor_depth = pipe2_profile.get_device().first_depth_sensor()
    sensor_depth.set_option(rs.option.emitter_enabled, 0)  # turn OFF projector
    
    try:
        # Retreive the stream and intrinsic properties for both cameras
        profile1 = pipe1.get_active_profile()
        profile2 = pipe2.get_active_profile()
        # future improvements: make both stream configureable
        streams = {"cam1"  : profile1.get_stream(rs.stream.fisheye, 1).as_video_stream_profile(),
                   "pose"  : profile1.get_stream(rs.stream.pose),
                   "cam2" : profile2.get_stream(rs.stream.infrared, 1).as_video_stream_profile()}  # IR1
                   #"cam2" : profile1.get_stream(rs.stream.fisheye, 2).as_video_stream_profile()}  # test
        intrinsics = {"cam1"  : streams["cam1"].get_intrinsics(),
                      "cam2" : streams["cam2"].get_intrinsics()}
        #print("cam1:",  intrinsics["cam1"])
        #print("cam2:", intrinsics["right"])
    
        save_calibration(args.path, intrinsics, streams)
    
        # capture images
        i = 0
        print("Press 's' to save image.\nPress 'q' to quit recording (and start calibration)")
        while True:
            # cam 1
            frames1 = pipe1.wait_for_frames()
            f_fe1 = frames1.get_fisheye_frame(1)  # left fisheye
            f_fe2 = frames1.get_fisheye_frame(2)  # right fisheye
            if not f_fe1 or not f_fe2:
                continue
            img_fe1 = np.asanyarray(f_fe1.get_data())
            img_fe2 = np.asanyarray(f_fe2.get_data())
    
            # cam 2
            frames2 = pipe2.wait_for_frames()
            f_ir1 = frames2.get_infrared_frame(1)  # left infrared
            f_ir2 = frames2.get_infrared_frame(2)  # right infrared
            f_color = frames2.get_color_frame()
            if not f_ir1 or not f_ir2 or not f_color:
                continue
            img_ir1 = np.asanyarray(f_ir1.get_data())
            img_ir2 = np.asanyarray(f_ir2.get_data())
            img_color = np.asanyarray(f_color.get_data())

            # TODO: configure streams
            img1 = img_fe1
            img2 = img_ir1

            # display
            cv2.imshow('cam1', img1)
            cv2.imshow('cam2', img2)
    
            # save or quit
            k = cv2.waitKey(1)
            if k == ord('s'):
                if not os.path.exists(args.path):
                    os.mkdir(args.path)
                cv2.imwrite(args.path + '/fe1_' + str(i) + '.png', img_fe1)
                cv2.imwrite(args.path + '/fe2_' + str(i) + '.png', img_fe2)
                cv2.imwrite(args.path + '/ir1_' + str(i) + '.png', img_ir1)
                cv2.imwrite(args.path + '/ir2_' + str(i) + '.png', img_ir2)
                cv2.imwrite(args.path + '/color_' + str(i) + '.png', img_color)
                print("Saved images.")
                i = i+1
    
            if k == ord('q') or k == ord('c'):
                break
    
    finally:
        pipe1.stop()
        pipe2.stop()


# calibrate
print("Calibrate extrinsics...")

# arrays to store detections
P3 = []  # w.r.t. target frame
P2_1 = []  # in image #1
P2_2 = []  # in image #2

# TODO: configure streams
images1 = glob.glob(args.path + '/fe1_*')
#images2 = glob.glob(args.path + '/fe2_*')  # test
images2 = glob.glob(args.path + '/ir1_*')
images1.sort()
images2.sort()
#print(images1)
#print(images2)

if len(images1) == len(images2) == 0:
    print("No images found. Exit.")
    exit(0)

for i, fname in enumerate(images1):
    img1 = cv2.imread(images1[i])
    img2 = cv2.imread(images2[i])

    gray1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
    gray2 = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)

    # detect
    ret1, corners1 = cv2.findChessboardCorners(gray1, CHECKERBOARD, None)
    ret2, corners2 = cv2.findChessboardCorners(gray2, CHECKERBOARD, None)

    if ret1 and ret2:
        # subpixel refinement
        criteria_sub = (cv2.TermCriteria_COUNT + cv2.TERM_CRITERIA_EPS, 10, 1e-1)
        rt = cv2.cornerSubPix(gray1, corners1, (7, 7), (-1, -1), criteria_sub)
        P2_1.append(corners1)
        if args.visualize:
            ret1 = cv2.drawChessboardCorners(img1, CHECKERBOARD, corners1, ret1)
            cv2.imshow("img1", img1)
            cv2.waitKey(200)

        rt = cv2.cornerSubPix(gray2, corners2, (7, 7), (-1, -1), criteria_sub)
        P2_2.append(corners2)
        if args.visualize:
            ret2 = cv2.drawChessboardCorners(img2, CHECKERBOARD, corners2, ret2)
            cv2.imshow("img2", img2)
            cv2.waitKey(200)


# calibration (stereo extrinsics)
R = np.zeros((1, 1, 3), dtype=np.float64)
T = np.zeros((1, 1, 3), dtype=np.float64)

N = len(P2_1)  # number of successful detections

p3d = np.zeros( (CHECKERBOARD[0]*CHECKERBOARD[1], 1, 3) , np.float64)
p3d[:,0, :2] = np.mgrid[0:CHECKERBOARD[0], 0:CHECKERBOARD[1]].T.reshape(-1, 2)

# fisheye.stereoCalibrate needs different data structures/dimensions than cv2.stereoCalibrate, i.e. (N, 1, CHECKERBOARD[0]*CHECKERBOARD[1], 2/3)!
P3 = np.array([p3d]*N, dtype=np.float64)
P2_1 = np.asarray(P2_1, dtype=np.float64)
P2_2 = np.asarray(P2_2, dtype=np.float64)

P3 = np.reshape(P3, (N, 1, CHECKERBOARD[0]*CHECKERBOARD[1], 3))*SIDE_LENGTH
P2_1 = np.reshape(P2_1, (N, 1, CHECKERBOARD[0]*CHECKERBOARD[1], 2))
P2_2 = np.reshape(P2_2, (N, 1, CHECKERBOARD[0]*CHECKERBOARD[1], 2))

(K1, D1, K2, D2, H1) = load_calibration(args.path)

(rms, _, _, _, _, R, T) = \
cv2.fisheye.stereoCalibrate(
    P3,
    P2_1,
    P2_2,
    K1,
    D1,
    K2,
    D2,
    (0,0), # only used to initialize intrinsics when no intrinsics provided
    R,
    T,
    cv2.fisheye.CALIB_FIX_INTRINSIC  # extrinsics only
)

print("RMS:", rms)

H_cam2_cam1 = np.eye(4)
H_cam2_cam1[:3,:3] = R
H_cam2_cam1[:3,3] = T.flatten()

# w.r.t. pose
H_ir1_fe1 = H_cam2_cam1  # TODO: configure
H_pose_fe1 = H1

H_pose_ir1 = H_pose_fe1.dot( np.linalg.inv(H_ir1_fe1) )
print("H (ir1 wrt pose) =", H_pose_ir1)

fn = "H.txt"
np.savetxt(fn, H_pose_ir1, fmt='%.9f')
print("Output written to", fn)
