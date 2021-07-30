# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device D455
#test:donotrun

import time
import threading
from queue import Queue
from rspy import test
import pyrealsense2 as rs

# Run RGB stream in D455 with 90 fps and find frame drops by checking HW timestamp of each frame

if __name__ == '__main__':
    ctx = rs.context()
    device = ctx.query_devices()[0]
    product_line = device.get_info(rs.camera_info.product_line)
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
    class Test:
        def __init__(self, rgb_sensor):
            self._stop = False
            self.frames = []
            self.count_drops = 0
            self.frame_drops_info = {}
            self.prev_hw_timestamp = 0.0
            self.prev_fnum = 0
            self.first_frame = True
            self.lrs_queue = rs.frame_queue(capacity=100000, keep_frames=True)
            self.post_process_queue = Queue(maxsize=1000000)
            self.rgb_sensor = rgb_sensor

        def start_rgb_sensor(self):
            self.rgb_sensor.start(self.lrs_queue)
        def stop(self):
            self._stop = True

        def produce_frames(self, timeout=1):
            while not self._stop:
                try:
                    lrs_frame = self.lrs_queue.wait_for_frame(timeout_ms=timeout * 1000)
                except Exception as e:
                    print
                    str(e)
                    continue
                self.post_process_queue.put(lrs_frame, block=True, timeout=timeout)

        def consume_frames(self):
            while not self._stop:
                element = self.post_process_queue.get(block=True)
                lrs_frame = element
                self.my_process(lrs_frame)
                del lrs_frame
                self.post_process_queue.task_done()

        def my_process(self, f):
            if not f:
                return
            delta_tolerance_percent = 95.
            ideal_delta = round(1000000.0 / 90, 2)
            delta_tolerance_in_us = ideal_delta * delta_tolerance_percent / 100.0
            if self.first_frame:
                self.prev_hw_timestamp = f.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                self.prev_fnum = f.get_frame_number()
                self.first_frame = False
                return
            curr_hw_timestamp = f.get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
            delta = curr_hw_timestamp - self.prev_hw_timestamp
            fnum = f.get_frame_number()
            if delta > ideal_delta + delta_tolerance_in_us:
                self.count_drops += 1
                self.frame_drops_info[fnum] = fnum - self.prev_fnum
            self.prev_hw_timestamp = curr_hw_timestamp
            self.prev_fnum = fnum
            #print("* frame drops = ", self.count_drops)

        def analysis(self):
            print ("Number of frame drops is {}".format(self.count_drops))
            for k, v in self.frame_drops_info:
                print("Number of dropped frame before frame ", k, ", is :", v)

            test.check(self.count_drops == 0)
            test.finish()


    test.start("Testing D455 frame drops on " + product_line + " device ")
    for ii in range(60):
        print("================ Iteration {} ================".format(ii))
        test = Test(rgb_sensor)
        rgb_sensor.set_option(rs.option.global_time_enabled, 0)
        rgb_sensor.open([rgb_profile])

        producer_thread = threading.Thread(target=test.produce_frames, name="producer_thread")
        producer_thread.start()
        consumer_thread = threading.Thread(target=test.consume_frames, name="consumer_thread")
        consumer_thread.start()

        test.start_rgb_sensor()
        time.sleep(30)
        test.stop()  # notify to stop producing-consuming frames

        producer_thread.join(timeout=60)
        consumer_thread.join(timeout=60)

        test.analysis()
        rgb_sensor.stop()
        rgb_sensor.close()
