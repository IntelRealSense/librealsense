# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:timeout 20

import os
import pyrealsense2 as rs2
from rspy import test
import tempfile

# This test checks that stop of pipeline with playback file
# and non realtime mode is not stuck due to deadlock of
# pipeline stop thread and syncer blocking enqueue thread (DSO-15157)
#############################################################################################
test.start("Playback with non realtime isn't stuck at stop")

filename = tempfile.gettempdir() + os.sep + 'recording_deadlock.bag'

pipeline = rs2.pipeline()
config = rs2.config()
config.enable_all_streams()
config.enable_device_from_file(filename, repeat_playback=False)
profile = pipeline.start(config)
device = profile.get_device().as_playback().set_real_time(False)
success = True
while success:
    success, _ = pipeline.try_wait_for_frames(1000)
print("stopping...")
pipeline.stop()
print("stopped")

test.finish()
#############################################################################################

test.print_results_and_exit()
