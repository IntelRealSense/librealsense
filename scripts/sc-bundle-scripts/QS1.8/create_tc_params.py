import numpy as np
from math import floor
import struct
import pyrealsense2 as rs
from dataclasses import dataclass
import zlib

######### These are the input params #########################################
#constants
IMAGE_WIDTH_OFFSET_IN_DEPTH_CALIB=0x148
IMAGE_HEIGHT_OFFSET_IN_DEPTH_CALIB=0x14a
PRINCIPAL_POINT_X_OFFSET_IN_DEPTH_CALIB=0x14c
PRINCIPAL_POINT_Y_OFFSET_IN_DEPTH_CALIB=0x150
FOCAL_LENGTH_X_OFFSET_IN_DEPTH_CALIB=0x154
FOCAL_LENGTH_Y_OFFSET_IN_DEPTH_CALIB=0x158

PRINCIPAL_POINT_X_OFFSET_IN_TC_PARAMS=0x4
PRINCIPAL_POINT_Y_OFFSET_IN_TC_PARAMS=0x8
FOCAL_LENGTH_X_OFFSET_IN_TC_PARAMS=0xc
FOCAL_LENGTH_Y_OFFSET_IN_TC_PARAMS=0x10

#This is the resolution for TC
target_resolution = np.array([1280, 720])
#This is the entire tc_params table with the calibration dependant part set to 0xFF
tc_param_table=np.asarray([0x00, 0x05, 0xD0, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x06, 0x08, 0x01, 0x04, 0xD0, 0x07, 0x01, 0x00, 0x20, 0x20, 0x20, 0x03, 0xF4, 0x01, 0x01, 0xCD, 0xCC, 0x4C, 0x3E, 0x32, 0x00, 0x00, 0x80, 0x3F, 0x9A, 0x99, 0x99, 0x3E, 0x9A, 0x99, 0x99, 0x3E, 0xCD, 0xCC, 0x4C, 0x3D, 0x00, 0x10, 0x10, 0x9A, 0x99, 0x19, 0x3E, 0x06, 0x33, 0x33, 0x33, 0x3F, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x51, 0x78, 0x3F, 0x64, 0x62, 0xBE, 0xBC, 0x38, 0x62, 0xBE, 0xBC, 0x38, 0x62, 0xBE, 0xBC, 0x38, 0x62, 0xBE, 0xBC, 0x38, 0xAC, 0xC5, 0x27, 0x38, 0xAC, 0xC5, 0x27, 0x38, 0xAC, 0xC5, 0x27, 0x38, 0xAC, 0xC5, 0x27, 0x38, 0x82, 0xA8, 0x7B, 0x38, 0x82, 0xA8, 0x7B, 0x38, 0x82, 0xA8, 0x7B, 0x38, 0x82, 0xA8, 0x7B, 0x38, 0x01, 0x02, 0xCD, 0xCC, 0x4C, 0x3E, 0x33, 0x33, 0x33, 0x3F, 0x9A, 0x99, 0x99, 0x3E, 0x00, 0x00, 0x00, 0x41, 0xCD, 0xCC, 0xCC, 0x3E, 0x00, 0x00, 0xA0, 0x40, 0x00, 0x00, 0xA0, 0x41, 0x00, 0x00, 0x20, 0xC1, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0xCD, 0xCC, 0xCC, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0x33, 0x33, 0x33, 0x3F, 0x9A, 0x99, 0x99, 0x3E, 0x00, 0x00, 0x00, 0x41, 0xCD, 0xCC, 0xCC, 0x3E, 0x00, 0x00, 0xA0, 0x40, 0x00, 0x00, 0xA0, 0x41, 0x00, 0x00, 0x20, 0xC1, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x33, 0x33, 0xB3, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0x33, 0x33, 0x33, 0x3F, 0x9A, 0x99, 0x99, 0x3E, 0x00, 0x00, 0x00, 0x41, 0xCD, 0xCC, 0xCC, 0x3E, 0x00, 0x00, 0xA0, 0x40, 0x00, 0x00, 0xA0, 0x41, 0x00, 0x00, 0x20, 0xC1, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x9A, 0x99, 0x99, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0x00, 0x01, 0x90, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x14, 0x05, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0x8A, 0x02, 0x00, 0x00, 0x00, 0x3F, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x68, 0x01, 0x99, 0x00, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x20, 0x05, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x02, 0x20, 0x01, 0x00, 0x01, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], dtype=np.uint8)

##############################################################################
def float_to_hex(f):
    value=struct.unpack('>I', struct.pack('<f', f))[0]
    return '{:08x}'.format(value)

def hex_to_float(h):
    return struct.unpack('<f', bytes.fromhex(h))[0]

def build_header(table_array, table_id, version_array):
    version_major = version_array[0]
    version_minor = version_array[1]
    table_size = len(table_array)
    counter = 0
    test_cal_version = 0
    crc_computed = zlib.crc32(table_array)

    table_id_ints = np.frombuffer(table_id.to_bytes(2, 'little'), dtype=np.uint8)
    table_size_ints = np.frombuffer(table_size.to_bytes(4, 'little'), dtype=np.uint8)
    counter_ints = np.frombuffer(counter.to_bytes(2, 'little'), dtype=np.uint8)
    test_cal_version_ints = np.frombuffer(test_cal_version.to_bytes(2, 'little'), dtype=np.uint8)
    crc_ints = np.frombuffer(crc_computed.to_bytes(4, 'little'), dtype=np.uint8)

    version_ints = np.hstack((version_major, version_minor))
    with_table_id = np.hstack((version_ints, table_id_ints))
    with_table_size = np.hstack((with_table_id, table_size_ints))
    with_counter = np.hstack((with_table_size, counter_ints))
    with_test_cal_version = np.hstack((with_counter, test_cal_version_ints))
    header_ready = np.hstack((with_test_cal_version, crc_ints))

    return header_ready

def get_depth_calib_table():
    get_table_opcode = 0xa7
    flash_mem_enum = 1
    depth_calib_id = 0x00b4
    d500_dynamic = 1
    cmd = hwm_dev.build_command(opcode=get_table_opcode,
                                param1=flash_mem_enum,
                                param2=depth_calib_id,
                                param3=d500_dynamic)
    ans = hwm_dev.send_and_receive_raw_data(cmd)
    opcode = ans[0]
    if opcode != get_table_opcode:
        print("opcode not received back!")
    # slicing:
    # the 4 first bytes - opcode
    # the 16 following bytes = header
    ans_data = ans[20:]
    return ans_data

def set_tc_params_table(tc_params_table):
    set_table_opcode = 0xa6
    flash_mem_enum = 1
    tc_params_id = 0x00c0
    d500_dynamic = 1
    version_array = np.array([2, 0])
    header = build_header(tc_params_table, tc_params_id, version_array)
    table_with_header = np.hstack((header, tc_params_table))
    cmd = hwm_dev.build_command(opcode=set_table_opcode,
                                param1=flash_mem_enum,
                                param2=tc_params_id,
                                param3=d500_dynamic,
                                data=table_with_header)
    ans = hwm_dev.send_and_receive_raw_data(cmd)
    opcode = ans[0]
    if opcode != set_table_opcode:
        print("opcode not received back!")
    return ans

@dataclass
class Calib:
    principal_point: np.array([0,0])
    focal_length: np.array([0,0])

def convert_calib(input_principal_point, input_focal_length, input_resolution, output_resolution):
    temp_size_xy = input_resolution
    temp_principal_point=np.array([0,0])
    temp_principal_point[0] = floor(min(input_principal_point[0], input_resolution[0] - 1 - input_principal_point[0]))
    temp_principal_point[1] = floor(min(input_principal_point[1], input_resolution[1] - 1 - input_principal_point[1]))
    temp_size_xy = 1 + (2*temp_principal_point)

    scale_ratio = max(output_resolution/temp_size_xy)
    crop_xy = (temp_size_xy * scale_ratio - output_resolution) / 2;

    output_principal_point = (temp_principal_point + 0.5) * scale_ratio - crop_xy - 0.5;
    output_focal_length = input_focal_length * scale_ratio

    return Calib(output_principal_point, output_focal_length)



#############################################################################################
print("******************************************************************************")
print("Setting tc_param table according to spec flash 0.92 and the camera calibration")
print("******************************************************************************")
#############################################################################################

ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()
hwm_dev = dev.as_debug_protocol()

print("Switching to Service Mode")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)

depth_calib_table = get_depth_calib_table()

src_width = int(f"{depth_calib_table[IMAGE_WIDTH_OFFSET_IN_DEPTH_CALIB+1]:0{2}x}" + f"{depth_calib_table[IMAGE_WIDTH_OFFSET_IN_DEPTH_CALIB]:0{2}x}", 16)
src_height = int(f"{depth_calib_table[IMAGE_HEIGHT_OFFSET_IN_DEPTH_CALIB+1]:0{2}x}" + f"{depth_calib_table[IMAGE_HEIGHT_OFFSET_IN_DEPTH_CALIB]:0{2}x}", 16)
src_resolution = np.array([src_width, src_height])

src_principal_point = np.array([hex_to_float(f"{depth_calib_table[PRINCIPAL_POINT_X_OFFSET_IN_DEPTH_CALIB]:0{2}x}" + \
                        f"{depth_calib_table[PRINCIPAL_POINT_X_OFFSET_IN_DEPTH_CALIB + 1]:0{2}x}" + \
                        f"{depth_calib_table[PRINCIPAL_POINT_X_OFFSET_IN_DEPTH_CALIB + 2]:0{2}x}" + \
                        f"{depth_calib_table[PRINCIPAL_POINT_X_OFFSET_IN_DEPTH_CALIB + 3]:0{2}x}"),
                        hex_to_float(f"{depth_calib_table[PRINCIPAL_POINT_Y_OFFSET_IN_DEPTH_CALIB]:0{2}x}" + \
                        f"{depth_calib_table[PRINCIPAL_POINT_Y_OFFSET_IN_DEPTH_CALIB + 1]:0{2}x}" + \
                        f"{depth_calib_table[PRINCIPAL_POINT_Y_OFFSET_IN_DEPTH_CALIB + 2]:0{2}x}" + \
                        f"{depth_calib_table[PRINCIPAL_POINT_Y_OFFSET_IN_DEPTH_CALIB + 3]:0{2}x}")])

src_focal_length = np.array([hex_to_float(f"{depth_calib_table[FOCAL_LENGTH_X_OFFSET_IN_DEPTH_CALIB]:0{2}x}" + \
                        f"{depth_calib_table[FOCAL_LENGTH_X_OFFSET_IN_DEPTH_CALIB + 1]:0{2}x}" + \
                        f"{depth_calib_table[FOCAL_LENGTH_X_OFFSET_IN_DEPTH_CALIB + 2]:0{2}x}" + \
                        f"{depth_calib_table[FOCAL_LENGTH_X_OFFSET_IN_DEPTH_CALIB + 3]:0{2}x}"),
                        hex_to_float(f"{depth_calib_table[FOCAL_LENGTH_Y_OFFSET_IN_DEPTH_CALIB]:0{2}x}" + \
                        f"{depth_calib_table[FOCAL_LENGTH_Y_OFFSET_IN_DEPTH_CALIB + 1]:0{2}x}" + \
                        f"{depth_calib_table[FOCAL_LENGTH_Y_OFFSET_IN_DEPTH_CALIB + 2]:0{2}x}" + \
                        f"{depth_calib_table[FOCAL_LENGTH_Y_OFFSET_IN_DEPTH_CALIB + 3]:0{2}x}")])

new_calib = convert_calib(src_principal_point, src_focal_length, src_resolution, target_resolution)

print("New principal point:")
print(new_calib.principal_point)
print("New focal length:")
print(new_calib.focal_length)

tc_param_table[PRINCIPAL_POINT_X_OFFSET_IN_TC_PARAMS  ] = int(float_to_hex(new_calib.principal_point[0])[0:2],16)
tc_param_table[PRINCIPAL_POINT_X_OFFSET_IN_TC_PARAMS+1] = int(float_to_hex(new_calib.principal_point[0])[2:4],16)
tc_param_table[PRINCIPAL_POINT_X_OFFSET_IN_TC_PARAMS+2] = int(float_to_hex(new_calib.principal_point[0])[4:6],16)
tc_param_table[PRINCIPAL_POINT_X_OFFSET_IN_TC_PARAMS+3] = int(float_to_hex(new_calib.principal_point[0])[6:8],16)
tc_param_table[PRINCIPAL_POINT_Y_OFFSET_IN_TC_PARAMS  ] = int(float_to_hex(new_calib.principal_point[1])[0:2],16)
tc_param_table[PRINCIPAL_POINT_Y_OFFSET_IN_TC_PARAMS+1] = int(float_to_hex(new_calib.principal_point[1])[2:4],16)
tc_param_table[PRINCIPAL_POINT_Y_OFFSET_IN_TC_PARAMS+2] = int(float_to_hex(new_calib.principal_point[1])[4:6],16)
tc_param_table[PRINCIPAL_POINT_Y_OFFSET_IN_TC_PARAMS+3] = int(float_to_hex(new_calib.principal_point[1])[6:8],16)
tc_param_table[FOCAL_LENGTH_X_OFFSET_IN_TC_PARAMS  ] = int(float_to_hex(new_calib.focal_length[0])[0:2],16)
tc_param_table[FOCAL_LENGTH_X_OFFSET_IN_TC_PARAMS+1] = int(float_to_hex(new_calib.focal_length[0])[2:4],16)
tc_param_table[FOCAL_LENGTH_X_OFFSET_IN_TC_PARAMS+2] = int(float_to_hex(new_calib.focal_length[0])[4:6],16)
tc_param_table[FOCAL_LENGTH_X_OFFSET_IN_TC_PARAMS+3] = int(float_to_hex(new_calib.focal_length[0])[6:8],16)
tc_param_table[FOCAL_LENGTH_Y_OFFSET_IN_TC_PARAMS  ] = int(float_to_hex(new_calib.focal_length[1])[0:2],16)
tc_param_table[FOCAL_LENGTH_Y_OFFSET_IN_TC_PARAMS+1] = int(float_to_hex(new_calib.focal_length[1])[2:4],16)
tc_param_table[FOCAL_LENGTH_Y_OFFSET_IN_TC_PARAMS+2] = int(float_to_hex(new_calib.focal_length[1])[4:6],16)
tc_param_table[FOCAL_LENGTH_Y_OFFSET_IN_TC_PARAMS+3] = int(float_to_hex(new_calib.focal_length[1])[6:8],16)

set_tc_params_table(tc_param_table)

print("Returning to Safety Mode")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.run)