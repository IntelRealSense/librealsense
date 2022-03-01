import pyrealsense2 as rs
from rspy import devices, log, test, file, repo


#############################################################################################
# Help Functions
#############################################################################################

def convert_bytes_to_decimal(command):
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
    expected_status = convert_bytes_to_decimal("10 00 00 00")
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("Old Scenario Test")
try:
    gvd_command = "14 00 ab cd 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
    raw_command = convert_bytes_to_decimal(gvd_command)
    status, old_scenario_result = send_hardware_monitor_command(dev, raw_command)
    test.check_equal_lists(status, expected_status)
except:
    test.unexpected_exception()
test.finish()

#############################################################################################

test.start("New Scenario Test")
try:
    gvd_opcode = 0x10
    raw_command = rs.debug_protocol(dev).build_command(gvd_opcode)
    status, new_scenario_result = send_hardware_monitor_command(dev, raw_command)
    test.check_equal_lists(status, expected_status)
    test.check_equal_lists(new_scenario_result, old_scenario_result)
except:
    test.unexpected_exception()
test.finish()


#############################################################################################

test.print_results_and_exit()
