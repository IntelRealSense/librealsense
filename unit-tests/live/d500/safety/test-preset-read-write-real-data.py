# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:device D585S
# we initialize all safety zone , this to start all safety tests with a known table which is 0 (hard coded in FW)
#test:priority 1

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

def get_random_preset():

    # rotation matrix [-2..2] (no units)
    rx = rs.float3(random.uniform(-2, 2), random.uniform(-2, 2), random.uniform(-2, 2))
    ry = rs.float3(random.uniform(-2, 2), random.uniform(-2, 2), random.uniform(-2, 2))
    rz = rs.float3(random.uniform(-2, 2), random.uniform(-2, 2), random.uniform(-2, 2))
    rotation = rs.float3x3(rx, ry, rz)

    # translation vector [m]
    translation = rs.float3(random.uniform(0, 100), random.uniform(0, 100), random.uniform(0, 100))

    # init safety extrinsics table (transformation link) from rotation matrix and translation vector
    transformation_link = rs.safety_extrinsics_table(rotation, translation)

    # init safety platform config
    safety_platform = rs.safety_platform()
    safety_platform.transformation_link = transformation_link
    safety_platform.robot_height = random.uniform(1, 3)  # [m]
    safety_platform.robot_mass = random.uniform(20, 200)  # [kg]
    safety_platform.reserved = random.sample(range(0, 255), 16)

    # define 4 points to be used in polygon
    p1 = rs.float2(random.uniform(-10, 10), random.uniform(-10, 10))
    p2 = rs.float2(random.uniform(-10, 10), random.uniform(-10, 10))
    p3 = rs.float2(random.uniform(-10, 10), random.uniform(-10, 10))
    p4 = rs.float2(random.uniform(-10, 10), random.uniform(-10, 10))

    # MOS (minimum object size)
    mos = rs.float2(random.uniform(0, 10), random.uniform(0, 10))

    # safety zone #0 (will be used twice: as danger and as warning zones)
    safety_zone = rs.safety_zone()
    safety_zone.zone_polygon = [p1, p2, p3, p4]
    safety_zone.safety_trigger_confidence = random.randint(0, 255)  # number of consecutive frames to raise safety signal
    safety_zone.minimum_object_size = mos
    safety_zone.mos_target_type = random.randint(0, 2)  # 0=hand, 1=leg, 2=body
    safety_zone.reserved = random.sample(range(0, 255), 8)

    # masking zone #0 (will be used 8 times)
    masking_zone = rs.masking_zone()
    masking_zone.attributes = 0x01
    masking_zone.minimal_range = random.uniform(0, 50) # m
    pixel = rs.pixel2D() # will be used 4 times
    pixel.i = random.randint(0, 255)
    pixel.j = random.randint(0, 255)
    masking_zone.region_of_interests = [pixel] * 4

    # safety environment
    safety_environment = rs.safety_environment()
    safety_environment.grid_cell_size = random.uniform(0, 0.1)  # m
    safety_environment.safety_trigger_duration = random.uniform(0, 900)  # sec
    safety_environment.angular_velocity = random.uniform(0, 40)  # rad/sec
    safety_environment.linear_velocity = random.uniform(0, 40)  # m/sec
    safety_environment.payload_weight = random.uniform(0, 1000)  # kg
    safety_environment.surface_confidence = random.randint(0, 100)  # [0..100%]
    safety_environment.surface_height = random.uniform(0, 1)  # [m]
    safety_environment.surface_inclination = random.uniform(0, 360)  # angle in degrees
    safety_environment.reserved = random.sample(range(0, 255), 15)

    safety_preset = rs.safety_preset()
    safety_preset.platform_config = safety_platform
    safety_preset.safety_zones = [safety_zone] * 2
    safety_preset.masking_zones = [masking_zone] * 8
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
zone_0 = safety_sensor.get_safety_preset(0)
for x in range(63):
    safety_sensor.set_safety_preset(x + 1, zone_0)
test.finish()

#############################################################################################

test.start("Writing safety preset to random index, then reading and comparing")
index = random.randint(1, 63)
log.out( "writing to index = ", index )
safety_preset = get_random_preset()

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

# switch back to original safety mode
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.standby)
test.check_equal( safety_sensor.get_option(rs.option.safety_mode), float(rs.safety_mode.standby))
test.print_results_and_exit()
