# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:device D585S
# we initialize all safety zones , before all other safety tests will run
#test:priority 10

import pyrealsense2 as rs
import random
from rspy import test, log
import time

#############################################################################################
# Helper Functions
#############################################################################################

# Constants
RUN_MODE     = 0 # RS2_SAFETY_MODE_RUN (RESUME)
STANDBY_MODE = 1 # RS2_SAFETY_MODE_STANDBY (PAUSE)
SERVICE_MODE = 2 # RS2_SAFETY_MODE_SERVICE (MAINTENANCE)

def generate_diagnostics_zone(rp1, rp2, rp3, rp4):
    diagnostics_zone = rs.masking_zone()

    p1 = rs.pixel2D()
    p1.i = int(rp1.x * 1000)
    p1.j = 3200 + int(rp1.y * 1000)
    p2 = rs.pixel2D()
    p2.i = int(rp2.x * 1000)
    p2.j = 3200 + int(rp2.y * 1000)
    p3 = rs.pixel2D()
    p3.i = int(rp3.x * 1000)
    p3.j = 3200 + int(rp3.y * 1000)
    p4 = rs.pixel2D()
    p4.i = int(rp4.x * 1000)
    p4.j = 3200 + int(rp3.y * 1000)

    diagnostics_zone.region_of_interests = [p1, p2, p3, p4]
    diagnostics_zone.minimal_range = 0
    diagnostics_zone.attributes = 1

    return diagnostics_zone
    
def get_valid_preset():

    # convert from RS camera cordination system to Robot cordination system
    # We use hard coded valid values as HKR compare and expect a match between safety interface extrinsic with the current safety preset extrinsic
    # rotation matrix 
    rx = rs.float3(0.0, 0.0, 1.0)
    ry = rs.float3(-1.0, 0.0, 0.0)
    rz = rs.float3(0.0, -1.0, 0.0)
    rotation = rs.float3x3(rx, ry, rz)

    # translation vector [m] 
    translation = rs.float3(0.0, 0.0, 0.27)

    # init safety extrinsics table (transformation link) from rotation matrix and translation vector
    transformation_link = rs.safety_extrinsics_table(rotation, translation)

    # init safety platform config
    safety_platform = rs.safety_platform()
    safety_platform.transformation_link = transformation_link
    safety_platform.robot_height = 1.0  # [m]
    safety_platform.reserved = [0] * 20

    # safety zone #0 (danger zone)
    # define 4 points to be used in polygon
    p1 = rs.float2(0.5, 0.1)
    p2 = rs.float2(0.8, 0.1)
    p3 = rs.float2(0.8, -0.1)
    p4 = rs.float2(0.5, -0.1)
    danger_zone = rs.safety_zone()
    danger_zone.zone_polygon = [p1, p2, p3, p4]
    danger_zone.safety_trigger_confidence = 1  # number of consecutive frames to raise safety signal
    danger_zone.reserved = [0] * 7
    
    # safety zone #1 (warning zones)
    # define 4 points to be used in polygon
    p1 = rs.float2(0.8, 0.1)
    p2 = rs.float2(1.2, 0.1)
    p3 = rs.float2(1.2, -0.1)
    p4 = rs.float2(0.8, -0.1)
    warning_zone = rs.safety_zone()
    warning_zone.zone_polygon = [p1, p2, p3, p4]
    warning_zone.safety_trigger_confidence = 1  # number of consecutive frames to raise safety signal
    warning_zone.reserved = [0] * 7

    # masking zone #0 (will be used 8 times)
    masking_zone = rs.masking_zone()
    masking_zone.attributes = 0 # disable masks
    masking_zone.minimal_range = 0.5 # m
    
    # currently will be ignored as we disabled the mask with 0 in attribute
    
    p1 = rs.pixel2D()
    p1.i = 0
    p1.j = 0
    p2 = rs.pixel2D()
    p2.i = 0
    p2.j = 320
    p3 = rs.pixel2D()
    p3.i = 200
    p3.j = 320
    p4 = rs.pixel2D()
    p4.i = 200
    p4.j = 0
    masking_zone.region_of_interests = [p1, p2, p3, p4]
    
    diagnostics_zone = generate_diagnostics_zone(
        rs.float2(0.5, 0.1), rs.float2(0.8, 0.1), rs.float2(0.8, -0.1), rs.float2(0.5, -0.1))

    # safety environment
    safety_environment = rs.safety_environment()
    safety_environment.safety_trigger_duration = 1  # sec
    safety_environment.zero_safety_monitoring = 0  # Regular
    safety_environment.hara_history_continuation = 0  # Regular
    safety_environment.angular_velocity = 0  # rad/sec
    safety_environment.payload_weight = 0  # kg
    safety_environment.surface_inclination = 15  # angle in degrees
    safety_environment.surface_height = 0.05  # [m]
    safety_environment.diagnostic_zone_fill_rate_threshold = 255  # [0..100%] , 255 is reserved for ignore
    safety_environment.floor_fill_threshold = 255  # [0..100%] , 255 is reserved for ignore
    safety_environment.depth_fill_threshold = 255  # [0..100%] , 255 is reserved for ignore
    safety_environment.diagnostic_zone_height_median_threshold = 255  # [millimeter]
    safety_environment.vision_hara_persistency = 1  # consecutive frames
    # The below param is random on purpose and the test
    # "Writing safety preset to random index, then reading and comparing" relies on that to be random
    safety_environment.crypto_signature = random.sample(range(0, 255), 32)
    safety_environment.reserved = [0] * 3

    safety_preset = rs.safety_preset()
    safety_preset.platform_config = safety_platform
    safety_preset.safety_zones = [danger_zone, warning_zone]
    safety_preset.masking_zones = [masking_zone] * 7 + [diagnostics_zone] # last mast index is diagnostic zone
    safety_preset.reserved = [0] * 16
    safety_preset.environment = safety_environment

    return safety_preset

#############################################################################################
# Tests
#############################################################################################

test.start("Verify Safety Sensor Extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
test.finish()

#############################################################################################

test.start("Switch to Service Mode")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.service))
test.finish()

#############################################################################################

test.start("Init all safety zones")
random_safety_preset = get_valid_preset()
for x in range(64):
    log.d("Init preset ID =", x)
    safety_sensor.set_safety_preset(x, random_safety_preset)
test.finish()

#############################################################################################

test.start("Writing safety preset to random index, then reading and comparing")
index = random.randint(0, 63)
log.out( "writing to index = ", index )
safety_preset = get_valid_preset()

# save original table
previous_result = safety_sensor.get_safety_preset(index)

# write a random new table to the device
safety_sensor.set_safety_preset(index, safety_preset)
# read the table from the device
read_result = safety_sensor.get_safety_preset(index)
# verify the tables are equal
test.check_equal(read_result, safety_preset)

# restore original table
safety_sensor.set_safety_preset(index, previous_result)
test.finish()

#############################################################################################

# switch back to Run safety mode
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.run)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.run))
test.print_results_and_exit()
