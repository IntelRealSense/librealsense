# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun
#test:device D585S

import pyrealsense2 as rs
import random
from rspy import test

#############################################################################################
# Tests
#############################################################################################

test.start("Verify Safety Sensor Extension")
ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
test.finish()

#############################################################################################

test.start("New safety preset creation")

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

# zone boundary
p5 = rs.float2(random.uniform(0, 10), random.uniform(0, 10))

# MOS (minimum object size)
p6 = rs.float2(random.uniform(0, 10), random.uniform(0, 10))

# safety zone #1
safety_zone = rs.safety_zone()
safety_zone.flags = 3  # valid and mandatory (first two bits are 1)
safety_zone.zone_type = random.randint(0, 2)  # 0=danger, 1=warning, 2=mask_zone
safety_zone.zone_polygon = [p1, p2, p3, p4]
safety_zone.masking_zone_v_boundary = p5
safety_zone.safety_trigger_confidence = random.randint(0, 255)  # number of consecutive frames to raise safety signal
safety_zone.minimum_object_size = p6
safety_zone.mos_target_type = random.randint(0, 2)  # 0=hand, 1=leg, 2=body
safety_zone.reserved = random.sample(range(0, 255), 16)

# safety environment
safety_environment = rs.safety_environment()
safety_environment.grid_cell_size = random.uniform(0, 100)  # cm
safety_environment.safety_trigger_duration = random.uniform(0, 900)  # sec
safety_environment.max_angular_velocity = random.uniform(0, 40)  # rad/sec
safety_environment.max_linear_velocity = random.uniform(0, 40)  # m/sec
safety_environment.payload_weight = random.uniform(0, 1000)  # kg
safety_environment.surface_confidence = random.randint(0, 100)  # [0..100%]
safety_environment.surface_height = random.uniform(0, 1)  # [m]
safety_environment.surface_inclination = random.uniform(0, 360)  # angle in degrees
safety_environment.reserved = random.sample(range(0, 255), 16)

# init safety preset
safety_preset = rs.safety_preset()
safety_preset.platform_config = safety_platform
safety_preset.safety_zones = [safety_zone, safety_zone, safety_zone, safety_zone]
safety_preset.environment = safety_environment

test.finish()

#############################################################################################

test.start("Writing safety preset to random index, then reading and comparing")
index = random.randint(1, 63)
safety_sensor.set_safety_preset(index, safety_preset)
read_result = safety_sensor.get_safety_preset(index)
test.check_equal(read_result, safety_preset)
test.finish()

#############################################################################################

test.print_results_and_exit()
