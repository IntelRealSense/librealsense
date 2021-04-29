import os
import re
import signal
import subprocess
import threading
import time
from multiprocessing import Queue
from datetime import datetime
from multiprocessing import Process

import pyrealsense2 as rs


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
                              or (p.as_video_stream_profile().width() == 480 and p.as_video_stream_profile().height() == 270)
                              or (p.as_video_stream_profile().width() == 640 and p.as_video_stream_profile().height() == 360) )
                      )

    for ii in range(10):
        print("================ Iteration {} ================".format(ii))
        _stop = False
        frames = []
        count_drops = 0
        frame_drops_info = {}
        prev_hw_timestamp = 0.0
        prev_fnum = 0
        first_frame = True

        lrs_queue = rs.frame_queue(capacity=100000, keep_frames=True)
        post_process_queue = Queue(maxsize=1000000)
        rgb_sensor.set_option(rs.option.global_time_enabled,0)

        rgb_sensor.open([rgb_profile])

        def produce_frames(timeout=1):
            while not _stop:
                try:
                    lrs_frame = lrs_queue.wait_for_frame(timeout_ms=timeout * 1000)
                except Exception as e:
                    print
                    str(e)
                    continue
                post_process_queue.put(lrs_frame, block=True, timeout=timeout)
        def consume_frames():
            while not _stop:
                element = post_process_queue.get(block=True)
                lrs_frame = element
                my_process(lrs_frame)
                del lrs_frame
                post_process_queue.task_done()

        """User callback to modify"""
        def my_process(f):
            delta_tolerance_percent = 95.
            ideal_delta = round(1000000.0 / 90, 2)
            delta_tolerance_in_us = ideal_delta * delta_tolerance_percent / 100.0
            if first_frame :
                prev_hw_timestamp = f.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                prev_fnum = f.get_frame_number()
                first_frame = False
                return
            curr_hw_timestamp = f.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
            delta = curr_hw_timestamp - prev_hw_timestamp
            fnum = f.get_frame_number()
            if delta > ideal_delta + delta_tolerance_in_us:
                count_drops += 1
                frame_drops_info[fnum] = fnum - prev_fnum
            prev_hw_timestamp = curr_hw_timestamp
            prev_fnum = fnum

            print("* frame drops = ", count_drops)
            for k, v in frame_drops_info:
                print("Number of dropped frame before frame ", k, ", is :", v)

        producer_thread = threading.Thread(target=produce_frames, name="producer_thread")
        producer_thread.start()
        consumer_thread = threading.Thread(target=consume_frames, name="consumer_thread")
        consumer_thread.start()

        rgb_sensor.start(lrs_queue)

        time.sleep(10)
        _stop = True  # notify to stop producing-consuming frames

        producer_thread.join(timeout=60)
        consumer_thread.join(timeout=60)

        rgb_sensor.stop()
        rgb_sensor.close()

