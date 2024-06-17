# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device each(D400*)

import pyrealsense2 as rs
from rspy import devices, log, test, file, repo


#############################################################################################
# Help Functions
#############################################################################################

def convert_bytes_string_to_decimal_list(command):
    command_input = []  # array of uint_8t

    # Parsing the command to array of unsigned integers(size should be < 8bits)
    # threw out spaces
    command = command.lower()
    command = command.split()

    for byte in command:
        command_input.append(int('0x' + byte, 0))

    return command_input


def send_hardware_monitor_command(device, command):
    raw_result = rs.debug_protocol(device).send_and_receive_raw_data(command)
    status = raw_result[:4]
    result = raw_result[4:]
    return status, result


#############################################################################################
# Tests
#############################################################################################

test.start("Init")
try:
    ctx = rs.context()
    dev = ctx.query_devices()[0]
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("WRONG COMMAND")
try:
    hwmc_opcode_as_int = 0xee
    hwmc_opcode_as_string = "ff ff ff ff"  # little endian

    raw_command = rs.debug_protocol(dev).build_command(hwmc_opcode_as_int)
    status, result = send_hardware_monitor_command(dev, raw_command)

    # expected status in case of success of "send_hardware_monitor_command" is the same as opcode
    # we expect error code instead of opcode
    expected_status = convert_bytes_string_to_decimal_list(hwmc_opcode_as_string)
    test.check_equal_lists(status, expected_status)

except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("GETSUBPRESETID - No Data to Return")
try:
    hwmc_opcode_as_int = 0x7d
    # ERR_NoDataToReturn = -21 = 0xeb
    hwmc_opcode_as_string = "eb ff ff ff"  # little endian

    raw_command = rs.debug_protocol(dev).build_command(hwmc_opcode_as_int)
    status, result = send_hardware_monitor_command(dev, raw_command)

    # expected status in case of success of "send_hardware_monitor_command" is the same as opcode
    # we expect error code instead of opcode
    expected_status = convert_bytes_string_to_decimal_list(hwmc_opcode_as_string)
    test.check_equal_lists(status, expected_status)

except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Set advanced mode - Wrong Parameter")
try:
    hwmc_opcode_as_int = 0x2b
    # ERR_WrongParameter = -6 = 0xfa
    hwmc_opcode_as_string = "fa ff ff ff"  # little endian

    raw_command = rs.debug_protocol(dev).build_command(hwmc_opcode_as_int)
    raw_command[9] = 2
    status, result = send_hardware_monitor_command(dev, raw_command)

    # expected status in case of success of "send_hardware_monitor_command" is the same as opcode
    # we expect error code instead of opcode
    expected_status = convert_bytes_string_to_decimal_list(hwmc_opcode_as_string)
    test.check_equal_lists(status, expected_status)

except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.print_results_and_exit()
