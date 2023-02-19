# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun
#test:device D585S

import pyrealsense2 as rs
from rspy import test

# Constants
RUN_MODE     = 0 # RS2_SAFETY_MODE_RUN (RESUME)
STANDBY_MODE = 1 # RS2_SAFETY_MODE_STANDBY (PAUSE)
SERVICE_MODE = 2 # RS2_SAFETY_MODE_SERVICE (MAINTENANCE)

device = test.find_first_device_or_exit();

def verify_frameset_received(pipe, pipe_profiles, count):
    for i in range(count):
        fs = pipe.wait_for_frames()
        test.check_equal(fs.size() , pipe_profiles.size())

########################### SRS - 3.3.1.14.b ##############################################

test.start("Pause / Resume - no impact on streaming")

cfg = rs.config()
cfg.enable_stream(rs.stream.safety)
cfg.enable_stream(rs.stream.depth)
cfg.enable_stream(rs.stream.color)

pipe = rs.pipeline()
profiles = pipe.start(cfg)

f = pipe.wait_for_frames()

pipeline_device = profiles.get_device()
safety_sensor = pipeline_device.first_safety_sensor()
safety_sensor.get_option(rs.option.safety_mode, RUN_MODE) # verify default

safety_sensor.set_option(rs.option.safety_mode, STANDBY_MODE) 
test.check_equal(safety_sensor.get_option(rs.option.safety_mode), STANDBY_MODE)
verify_frames_received(pipe, profiles, count = 10)

pipe.stop()
pipe.start(config)
verify_frames_received(pipe, profiles, count = 10)

safety_sensor.set_option(rs.option.safety_mode, RUN_MODE) 
test.check_equal(safety_sensor.get_option(rs.option.safety_mode), RUN_MODE)
verify_frames_received(pipe, profiles, count = 10)

pipe.stop()
test.finish()

########################### SRS - 3.3.1.14.c ##############################################

test.start("Maintenance / Resume - deactivate/activate streaming")

cfg = rs.config()
cfg.enable_stream(rs.stream.safety)
cfg.enable_stream(rs.stream.depth)
cfg.enable_stream(rs.stream.color)

pipe = rs.pipeline()
profiles = pipe.start(config)

f = pipe.wait_for_frames()

pipeline_device = profiles.get_device()
safety_sensor = pipeline_device.first_safety_sensor()

safety_sensor.set_option(rs.option.safety_mode, RUN_MODE) 
test.check_equal(safety_sensor.get_option(rs.option.safety_mode), RUN_MODE)
# Verify that on RUN mode we get frames
verify_frames_received(pipe, profiles, count = 10)

safety_sensor.set_option(rs.option.safety_mode, SERVICE_MODE) 
test.check_equal(safety_sensor.get_option(rs.option.safety_mode), SERVICE_MODE)
# Verify that on SERVICE mode we get no frames
test.check_throws( lambda: verify_frames_received(pipe, profiles, count = 1) , RuntimeError )

safety_sensor.set_option(rs.option.safety_mode, RUN_MODE) 
test.check_equal(safety_sensor.get_option(rs.option.safety_mode), RUN_MODE)
# Verify that on RUN mode we get frames
verify_frames_received(pipe, profiles, count = 10)

pipe.stop()
test.finish()

################################################################################################
test.print_results_and_exit()