import pyrealsense2 as rs
from rspy import devices, log, test, file, repo


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
    return raw_result[4:]


test.start("Init")

try:
    ctx = rs.context()
    dev = ctx.query_devices()[0]

    print("======================= old scenario ==========================\n")
    gvd_command = "14 00 ab cd 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
    converted_cmd = convert_bytes_to_decimal(gvd_command)
    old_result = send_hardware_monitor_command(dev, converted_cmd)
    print("old_result", old_result)

    print("\n")

    print("======================= NEW USAGE ==========================")
    gvd_opcode = 0x10
    new_command = rs.debug_protocol(dev).build_raw_data(gvd_opcode)
    new_result = send_hardware_monitor_command(dev, new_command)
    print("new_result", new_result)

    print("\n")

    equal = test.check_equal_lists(old_result, new_result)
    if not equal:
        log.w("expected results are not equal.")
except:
    test.unexpected_exception()

test.finish()
test.print_results_and_exit()
