import copy
import sys
import cv2
import pyrealsense2 as rs
import time
import numpy as np
frames = [] #= {rs.stream.depth:[], rs.stream.infrared:[], rs.stream.color:[]}

def callback(frame):
    global last_ts
    stream_type = frame.get_profile().stream_type()
    frame_index = frame.get_frame_number()
    if frame_index in frames:
        return
    frames.appeand(frame_index) # add only new frames


if __name__ == '__main__':
    ctx = rs.context()
    device = ctx.query_devices()[0]
    sn = device.get_info(rs.camera_info.serial_number)
    fw = device.get_info(rs.camera_info.firmware_version)
    print ('found device {}, fw {}'.format(sn, fw))
    sensors = device.query_sensors()
    depth_ir_sensor = next(s for s in sensors if s.get_info(rs.camera_info.name) == 'Stereo Module')
    rgb_sensor = next(s for s in sensors if s.get_info(rs.camera_info.name) == 'RGB Camera')

    rgb_profiles = rgb_sensor.profiles
    rgb_profile = next(p for p in
                         rgb_profiles if p.fps() == 90
                         and p.stream_type() == rs.stream.color
                         and p.format() == rs.format.yuyv
                         and ((p.as_video_stream_profile().width() == 424 and p.as_video_stream_profile().height() == 240)
                              or (p.as_video_stream_profile().width() == 480 and p.as_video_stream_profile().height() == 270) )
                       )

    for ii in range(4):
        print("================ Iteration {} ================".format(ii))
        rgb_sensor.open([rgb_profile])
        rgb_sensor.start(callback)
        time.sleep(30)
        prev_frame_num = 0
        count_drops = 0;
        for i in range(len(frames)):
            if prev_frame_num > 0 :
                if frames[i] - prev_frame_num > 2 :
                    count_drops += 1;
            prev_frame_num = frames[i]
        print(" frame drops = ", count_drops)
        rgb_sensor.stop()
        rgb_sensor.close()
        print ('RGB sensor is stopped')
