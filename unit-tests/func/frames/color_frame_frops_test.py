from __future__ import division
import logging
import time

import pyrealsense2 as rs

from lrs_frame_queue_manager import LRSFrameQueueManager

logging.basicConfig(level=logging.INFO)

# iterations, sleep = int(sys.argv[1]), int(sys.argv[2])
iterations, sleep = 4, 10
fps = 60
width, height = 640, 480
_format = rs.format.rgb8

ctx = rs.context()
dev = ctx.query_devices()[0]  # type: rs.device
color_sensor = dev.first_color_sensor()  # type: rs.sensor
color_sensor.set_option(rs.option.auto_exposure_priority, 0)

hw_ts = []


def cb(frame, ts):
    global hw_ts
    hw_ts.append(frame.get_frame_metadata(rs.frame_metadata_value.frame_timestamp))


lrs_fq = LRSFrameQueueManager()
lrs_fq.register_callback(cb)

pipe = rs.pipeline()
for i in range(iterations):
    lrs_fq.start()
    lrs_fq.unblock()
    print ('iteration #{}'.format(i))
    hw_ts = []
    print ('\tStart stream'.format(i))
    cfg = rs.config()
    cfg.enable_stream(rs.stream.color, width, height, _format, fps)
    pipe.start(cfg, lrs_fq.lrs_queue)
    time.sleep(sleep)
    print ('\tStop stream'.format(i))
    pipe.stop()

    expected_delta = 1000 / fps
    deltas_ms = [(ts1 - ts2) / 1000 for ts1, ts2 in zip(hw_ts[1:], hw_ts[:-1])]
    for idx, delta in enumerate(deltas_ms, 1):
        if delta > (expected_delta * 1.95):
            print ('\tFound drop #{} actual delta {} vs expected delta: {}'.format(idx, delta, expected_delta))
    lrs_fq.stop()
