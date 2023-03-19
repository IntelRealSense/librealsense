# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

# test:donotrun
# test:device D500*

import pyrealsense2 as rs
from rspy import test

ctx = rs.context()
dev = test.find_first_device_or_exit()
deb = dev.as_debug_protocol()


#############################################################################################

test.start("Get ds5 standard buffer")

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

test.start("Get buffer more than 1 KB - getting the whole table at once")

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

test.start("Get buffer more than 1 KB - getting the table chunk by chunk")

# getting rgb_lens_shading table - size is 1088
rgb_lens_shading_table_id = 0xb2
rgb_lens_shading_table_size = 1088
get_hkr_config_table_opcode = 0xa7
first_chunk_from_two_param = 0x10000
cmd1 = deb.build_command(opcode=get_hkr_config_table_opcode, param1=0, param2=rgb_lens_shading_table_id, param3=0, param4=first_chunk_from_two_param)
ans1 = deb.send_and_receive_raw_data(cmd1)
test.check_equal(ans1[0], get_hkr_config_table_opcode)

second_chunk_from_two_param = 0x10001
cmd2 = deb.build_command(opcode=get_hkr_config_table_opcode, param1=0, param2=rgb_lens_shading_table_id, param3=0, param4=second_chunk_from_two_param)
ans2 = deb.send_and_receive_raw_data(cmd2)
test.check_equal(ans2[0], get_hkr_config_table_opcode)

ans = ans1 + ans2
twice_opcode_length = 8
test.check_equal(len(ans), rgb_lens_shading_table_size + twice_opcode_length)

test.finish()
#############################################################################################

test.print_results_and_exit()
