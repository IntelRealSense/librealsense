# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:device D585S

import pyrealsense2 as rs
from rspy import test, log
import time

# Constants
RUN_MODE     = 0 # RS2_SAFETY_MODE_RUN (RESUME)
STANDBY_MODE = 1 # RS2_SAFETY_MODE_STANDBY (PAUSE)
SERVICE_MODE = 2 # RS2_SAFETY_MODE_SERVICE (MAINTENANCE)

device = test.find_first_device_or_exit();

def verify_frames_received(pipe, pipe_profile, count):
    prev_fs = None
    for i in range(count):
        # no check is needed, assume wait_for_frames will raise exception if not frames arrive
        fs = pipe.wait_for_frames()
        if (len(fs) > 1):
            for f in fs:
                log.d(f)
        else:
            log.d(fs)

########################### SRS - 3.3.1.14.b ##############################################

test.start("Pause / Resume - no impact on streaming")

cfg = rs.config()
cfg.enable_stream(rs.stream.safety, rs.format.raw8, 30)
cfg.enable_stream(rs.stream.depth, rs.format.z16, 30)
cfg.enable_stream(rs.stream.color, rs.format.rgb8, 30)

pipe = rs.pipeline()
profile = pipe.start(cfg)

f = pipe.wait_for_frames()

pipeline_device = profile.get_device()
safety_sensor = pipeline_device.first_safety_sensor()
log.d( "Verify default is run mode" )
test.check_equal( int(safety_sensor.get_option(rs.option.safety_mode)), RUN_MODE) # verify default

log.d( "Command standby mode" )
safety_sensor.set_option(rs.option.safety_mode, STANDBY_MODE)
time.sleep(0.1)  # sleep 100 milliseconds, see SRS ID 3.3.1.13
test.check_equal( int(safety_sensor.get_option(rs.option.safety_mode)), STANDBY_MODE)
verify_frames_received(pipe, profile, count = 10)

pipe.stop()
pipe.start(cfg)
verify_frames_received(pipe, profile, count = 10)

log.d( "Command run mode" )
safety_sensor.set_option(rs.option.safety_mode, RUN_MODE) 
time.sleep(0.1)  # sleep 100 milliseconds, see SRS ID 3.3.1.13
test.check_equal( int(safety_sensor.get_option(rs.option.safety_mode)), RUN_MODE)
verify_frames_received(pipe, profile, count = 10)

pipe.stop()
test.finish()

########################### SRS - 3.3.1.14.c ##############################################

test.start("Resume --> Maintenance deactivate streaming")

cfg = rs.config()
cfg.enable_stream(rs.stream.safety, rs.format.raw8, 30)

pipe = rs.pipeline()
profile = pipe.start(cfg)

f = pipe.wait_for_frames()

pipeline_device = profile.get_device()
safety_sensor = pipeline_device.first_safety_sensor()

log.d( "Command run mode" )
safety_sensor.set_option(rs.option.safety_mode, RUN_MODE)
time.sleep(0.1)  # sleep 100 milliseconds, see SRS ID 3.3.1.13
test.check_equal( int(safety_sensor.get_option(rs.option.safety_mode)), RUN_MODE)
# Verify that on RUN mode we get frames
verify_frames_received(pipe, profile, count = 10)

log.d( "Command service mode" )
safety_sensor.set_option(rs.option.safety_mode, SERVICE_MODE)
time.sleep(0.1)  # sleep 100 milliseconds, see SRS ID 3.3.1.13
test.check_equal( int(safety_sensor.get_option(rs.option.safety_mode)), SERVICE_MODE)
# Verify that on SERVICE mode we get no frames
test.check_throws( lambda: verify_frames_received(pipe, profile, 1) , RuntimeError )

# Restore Run mode
log.d( "Command run mode" )
safety_sensor.set_option(rs.option.safety_mode, RUN_MODE)
time.sleep(0.1)  # sleep 100 milliseconds, see SRS ID 3.3.1.13
test.check_equal( int(safety_sensor.get_option(rs.option.safety_mode)), RUN_MODE)

pipe.stop()
test.finish()

################################################################################################
test.print_results_and_exit()
