#!/usr/bin/python
# from __future__ import print_function
import numpy as np
import sys
import json
import ctypes
import os
import binascii
import struct
import pyrealsense2 as rs
import ctypes

max_float = struct.unpack('f','\xff\xff\xff\xff')[0]
max_int = struct.unpack('i','\xff\xff\xff\xff')[0]
max_uint8 = struct.unpack('B', '\xff')[0]


class CHeader:
    def __init__(self, version, table_type):
        self.buffer = np.ones(16, dtype=np.uint8) * 255
        self.buffer[0] = int(version[0], 16)
        self.buffer[1] = int(version[1], 16)
        self.buffer.dtype=np.uint16
        self.buffer[1] = int(table_type, 16)

    def size(self):
        return 16

    def set_data_size(self, size):
        self.buffer.dtype=np.uint32
        self.buffer[1] = size

    def set_crc32(self, crc32):
        self.buffer.dtype=np.uint32
        self.buffer[3] = crc32 % (1<<32)    # convert from signed to unsigned 32 bit
    
    def get_buffer(self):
        self.buffer.dtype=np.uint8
        return self.buffer


def bitwise_int_to_float(ival):
    return struct.unpack('f', struct.pack('i', ival))[0]
    
def bitwise_float_to_int(fval):
    return struct.unpack('i', struct.pack('f', fval))[0]

def parse_buffer(buffer):
    cmd_size = 24
    header_size = 16

    buffer.dtype=np.uint32    
    tab1_size = buffer[3]
    buffer.dtype=np.uint8
    print 'tab1_size (all_data): ', tab1_size

    tab1 = buffer[cmd_size:cmd_size+tab1_size]  # 520 == epprom++
    tab1.dtype=np.uint32
    tab2_size = tab1[1]
    tab1.dtype=np.uint8
    print 'tab2_size (calibration_table): ', tab2_size

    tab2 = tab1[header_size:header_size+tab2_size] # calibration table
    tab2.dtype=np.uint32
    tab3_size = tab2[1]
    tab2.dtype=np.uint8
    print 'tab3_size (calibration_table): ', tab3_size

    tab3 = tab2[header_size:header_size+tab3_size]  # D435 IMU Calib Table
    tab3.dtype=np.uint32
    tab4_size = tab3[1]
    tab3.dtype=np.uint8
    print 'tab4_size (D435_IMU_Calib_Table): ', tab4_size

    tab4 = tab3[header_size:header_size+tab4_size]  # calibration data
    return tab1, tab2, tab3, tab4


def get_D435_IMU_Calib_Table(X):
    version = ['0x02', '0x01']
    table_type = '0x20'
    header = CHeader(version, table_type)
    
    header_size = header.size()
    data_size = 37*4 + 96
    size_of_buffer = header_size + data_size    # according to table "D435 IMU Calib Table" in "https://sharepoint.ger.ith.intel.com/sites/3D_project/Shared%20Documents/Arch/D400/FW/D435i_IMU_Calibration_eeprom_0_52.xlsx"
    assert(size_of_buffer % 4 == 0)
    buffer = np.ones(size_of_buffer, dtype=np.uint8) * 255

    use_extrinsics = False
    use_intrinsics = True

    data_buffer = np.ones(data_size, dtype=np.uint8) * 255
    data_buffer.dtype = np.float32

    data_buffer[0] = bitwise_int_to_float(np.int32(int(use_intrinsics)) << 8 | 
                                          np.int32(int(use_extrinsics)))

    intrinsic_vector = np.zeros(24, dtype=np.float32)
    intrinsic_vector[:9] = X[:3,:3].flatten()
    intrinsic_vector[9:12] = X[:3,3]
    intrinsic_vector[12:21] = X[3:,:3].flatten()
    intrinsic_vector[21:24] = X[3:,3]    

    data_buffer[13:13+X.size] = intrinsic_vector
    data_buffer.dtype = np.uint8

    header.set_data_size(data_size)

    header.set_crc32(binascii.crc32(data_buffer))
    buffer[:header_size] = header.get_buffer()
    buffer[header_size:] = data_buffer
    return buffer


def get_calibration_table(d435_imu_calib_table):
    version = ['0x02', '0x00']
    table_type = '0x20'

    header = CHeader(version, table_type)

    d435_imu_calib_table_size = d435_imu_calib_table.size
    sn_table_size = 32
    data_size = d435_imu_calib_table_size + sn_table_size

    header_size = header.size()
    size_of_buffer = header_size + data_size    # according to table "D435 IMU Calib Table" in "https://sharepoint.ger.ith.intel.com/sites/3D_project/Shared%20Documents/Arch/D400/FW/D435i_IMU_Calibration_eeprom_0_52.xlsx"
    assert(size_of_buffer % 4 == 0)
    buffer = np.ones(size_of_buffer, dtype=np.uint8) * 255

    data_buffer = np.ones(data_size, dtype=np.uint8) * 255
    data_buffer[:d435_imu_calib_table_size] = d435_imu_calib_table

    header.set_data_size(data_size)
    header.set_crc32(binascii.crc32(data_buffer))

    buffer[:header_size] = header.get_buffer()
    buffer[header_size:header_size+data_size] = data_buffer
    return buffer

def get_eeprom(calibration_table):
    version = ['0x01', '0x01']
    table_type = '0x09'

    header = CHeader(version, table_type)

    DC_MM_EEPROM_SIZE = 520
    # data_size = calibration_table.size

    header_size = header.size()
    size_of_buffer = DC_MM_EEPROM_SIZE
    data_size = size_of_buffer - header_size 
    # size_of_buffer = header_size + data_size
    
    assert(size_of_buffer % 4 == 0)
    buffer = np.ones(size_of_buffer, dtype=np.uint8) * 255

    header.set_data_size(data_size)
    buffer[header_size:header_size+calibration_table.size] = calibration_table
    header.set_crc32(binascii.crc32(buffer[header_size:]))

    buffer[:header_size] = header.get_buffer()

    return buffer

def write_eeprom_to_camera(eeprom, serial_no=''):
    # DC_MM_EEPROM_SIZE = 520
    DC_MM_EEPROM_SIZE = eeprom.size
    DS5_CMD_LENGTH = 24

    MMEW_Cmd_bytes = '\x14\x00\xab\xcd\x50\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'


    buffer = np.ones([DC_MM_EEPROM_SIZE + DS5_CMD_LENGTH, ], dtype = np.uint8) * 255
    cmd = np.array(struct.unpack('I'*6, MMEW_Cmd_bytes), dtype=np.uint32)
    cmd.dtype = np.uint16
    cmd[0] += DC_MM_EEPROM_SIZE
    cmd.dtype = np.uint32
    cmd[3] = DC_MM_EEPROM_SIZE  # command 1 = 0x50
                                # command 2 = 0
                                # command 3 = size
    cmd.dtype = np.uint8
    buffer[:len(cmd)] = cmd
    buffer[len(cmd):len(cmd)+eeprom.size] = eeprom

    debug = get_debug_device(serial_no)
    if not debug:
        print 'Error getting RealSense Device.'
        return
    # tab1, tab2, tab3, tab4 = parse_buffer(buffer)

    rcvBuf = debug.send_and_receive_raw_data(bytearray(buffer))
    if rcvBuf[0] == buffer[4]:
        print 'SUCCESS: saved calibration to camera.'
    else:
        print 'FAILED: failed to save calibration to camera.'
        print rcvBuf


def get_debug_device(serial_no):
    ctx = rs.context()
    devices = ctx.query_devices()
    found_dev = False
    for dev in devices:
        if len(serial_no) == 0 or serial_no == dev.get_info(rs.camera_info.serial_number):
            found_dev = True
            break
    if not found_dev:
        print 'No RealSense device found' + str('.' if len(serial_no) == 0 else ' with serial number: '+serial_no)
        return 0

    # set to advance mode:
    advanced = rs.rs400_advanced_mode(dev)
    if not advanced.is_enabled():
        advanced.toggle_advanced_mode(True)

    # print a few basic information about the device
    print '  Device PID: ',  dev.get_info(rs.camera_info.product_id)
    print '  Device name: ',  dev.get_info(rs.camera_info.name)
    print '  Serial number: ',  dev.get_info(rs.camera_info.serial_number)
    print '  Firmware version: ',  dev.get_info(rs.camera_info.firmware_version)
    debug = rs.debug_protocol(dev)
    return debug



def main():
    if len(sys.argv) < 2:
        print("Usage:", sys.argv[0], "/path/to/accel.txt [/path/to/gyro.txt] [serial_no]")
        sys.exit(1)

    if len(sys.argv) > 2:
        #compute gyro bais

        #assume the first 4 seconds the device is still
        gyro = np.loadtxt(sys.argv[2], delimiter=",")
        gyro = gyro[gyro[:, 0] < gyro[0, 0]+4000, :]

        gyro_bais = np.mean(gyro[:, 1:], axis=0)
        print(gyro_bais)

    serial_no = '' if len(sys.argv) < 4 else sys.argv[3]

    #compute accel intrinsic parameters
    g = 9.80665 # SI Gravity page 52 of https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication330e2008.pdf
    buckets = [[g, 0, 0], [-g, 0, 0],
            [0, g, 0], [0, -g, 0],
            [0, 0, g], [0, 0, -g]]

    max_norm = np.linalg.norm(np.array([0.5, 0.5, 0.5]))

    measurements = [[], [], [], [], [], []];
    import csv
    with open(sys.argv[1], 'r') as csvfile:
        reader = csv.reader(csvfile)
        rnum = 0
        for row in reader:
            M = np.array([float(row[1]), float(row[2]), float(row[3])])
            for i in range(0, len(buckets)):
                if np.linalg.norm(M - buckets[i]) < max_norm:
                    measurements[i].append(M)
            
            rnum += 1

    mlen =  np.array([len(meas) for meas in measurements])
    print(mlen)
    print rnum, "rows and", mlen.sum(), "measurements to use"

    nrows = mlen.sum()
    w = np.zeros([nrows, 4])
    Y = np.zeros([nrows, 3])
    row = 0
    for i in range(0, len(buckets)):
        for m in measurements[i]:
            w[row, 0] = m[0]
            w[row, 1] = m[1]
            w[row, 2] = m[2]
            w[row, 3] = -1
            Y[row, 0] = buckets[i][0]
            Y[row, 1] = buckets[i][1]
            Y[row, 2] = buckets[i][2]
            row += 1
    params = {}
    if sys.version_info.major > 2:
        params = {'rcond': None}
    X, residuals, rank, singular = np.linalg.lstsq(w, Y, **params)
    print(X)
    print "residuals:", residuals
    print "rank:", rank
    print "singular:", singular

    calibration = {}
    calibration["device_type"] = "D435i"
    calibration["imus"] = list()
    calibration["imus"].append({})
    calibration["imus"][0]["accelerometer"] = {}
    calibration["imus"][0]["accelerometer"]["scale_and_alignment"] = X.flatten()[:9].tolist()
    calibration["imus"][0]["accelerometer"]["bias"] = X.flatten()[9:].tolist()
    calibration["imus"][0]["gyroscope"] = {}
    calibration["imus"][0]["gyroscope"]["scale_and_alignment"] = np.eye(3).flatten().tolist()
    calibration["imus"][0]["gyroscope"]["bias"] = gyro_bais.tolist()
    json_data = json.dumps(calibration, indent=4, sort_keys=True)

    directory = os.path.dirname(sys.argv[1])

    # with open(os.path.join(directory,"calibration.json"), 'w') as outfile:
    #     outfile.write(json_data)

    # #concatinate the two 12 element arrays and save
    intrinsic_buffer = np.zeros([6,4])

    intrinsic_buffer[:3,:4] = X.T
    intrinsic_buffer[3:,:3] = np.eye(3)
    intrinsic_buffer[3:,3] = gyro_bais

    # import pdb; pdb.set_trace()
    # intrinsic_buffer = ((np.array(range(24),np.float32)+1)/10).reshape([6,4])

    d435_imu_calib_table = get_D435_IMU_Calib_Table(intrinsic_buffer)
    calibration_table = get_calibration_table(d435_imu_calib_table)
    eeprom = get_eeprom(calibration_table)

    with open(os.path.join(directory,"calibration.bin"), 'wb') as outfile:
        outfile.write(eeprom.astype('f').tostring())

    write_eeprom_to_camera(eeprom, serial_no)

    """
    wtw = dot(transpose(w),w)
    wtwi = np.linalg.inv(wtw)
    print wtwi
    X = dot(wtwi, Y)
    print X
    """
if __name__ == '__main__':
    main()