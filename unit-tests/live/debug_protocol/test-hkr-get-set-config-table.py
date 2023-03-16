# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:donotrun
# test:device D500*

import pyrealsense2 as rs
from rspy import test

#############################################################################################

test.start("Get ds5 standard buffer")

ctx = rs.context()
dev = ctx.query_devices()[0]
deb = dev.as_debug_protocol()

# getting gvd
gvd_opcode = 0x10
gvd_size = 272
cmd = deb.build_command(opcode=gvd_opcode)
ans = deb.send_and_receive_raw_data(cmd)

# returns 4 bytes with opcode, and then the requested buffer
test.check_equal(ans[0], gvd_opcode)
test.check_equal(len(ans), gvd_size + 4)

test.finish()
#############################################################################################

test.start("Get buffer less than 1 KB")

ctx = rs.context()
dev = ctx.query_devices()[0]
deb = dev.as_debug_protocol()

# getting depth_calibration_table - size is 512
depth_calib_table_id = 0xb4
depth_calib_table_size = 512
get_hkr_config_table_opcode = 0xa7
cmd = deb.build_command(opcode=get_hkr_config_table_opcode, param1=0, param2=depth_calib_table_id, param3=0)
ans = deb.send_and_receive_raw_data(cmd)

test.check_equal(ans[0], get_hkr_config_table_opcode)
test.check_equal(len(ans), depth_calib_table_size + 4)

test.finish()

#############################################################################################

test.start("Get buffer more than 1 KB")

ctx = rs.context()
dev = ctx.query_devices()[0]
deb = dev.as_debug_protocol()

# getting rgb_lens_shading table - size is 1088
rgb_lens_shading_table_id = 0xb2
rgb_lens_shading_table_size = 1088
get_hkr_config_table_opcode = 0xa7
cmd = deb.build_command(opcode=get_hkr_config_table_opcode, param1=0, param2=rgb_lens_shading_table_id, param3=0)
ans = deb.send_and_receive_raw_data(cmd)
test.check_equal(ans[0], get_hkr_config_table_opcode)
test.check_equal(len(ans), rgb_lens_shading_table_size + 4)

test.finish()

#############################################################################################

test.print_results_and_exit()
