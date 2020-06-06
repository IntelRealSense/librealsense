// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "tclap/CmdLine.h"

using namespace std;
using namespace rs2;
using namespace TCLAP;

int main(int argc, char* argv[])
{
    CmdLine cmd("librealsense rs-terminal example tool", ' ', RS2_API_VERSION_STR);
    /*ValueArg<string> xml_arg("l", "load", "Full file path of commands XML file", false, "", "Load commands XML file");
    ValueArg<int> device_id_arg("d", "deviceId", "Device ID could be obtain from rs-enumerate-devices example", false, 0, "Select a device to work with");
    ValueArg<string> specific_SN_arg("n", "serialNum", "Serial Number can be obtain from rs-enumerate-devices example", false, "", "Select a device serial number to work with");
    SwitchArg all_devices_arg("a", "allDevices", "Do this command to all attached Realsense Devices", false);
    ValueArg<string> hex_cmd_arg("s", "send", "Hexadecimal raw data", false, "", "Send hexadecimal raw data to device");
    ValueArg<string> hex_script_arg("r", "raw", "Full file path of hexadecimal raw data script", false, "", "Send raw data line by line from script file");
    ValueArg<string> commands_script_arg("c", "cmd", "Full file path of commands script", false, "", "Send commands line by line from script file");
    cmd.add(xml_arg);
    cmd.add(device_id_arg);
    cmd.add(specific_SN_arg);
    cmd.add(all_devices_arg);
    cmd.add(hex_cmd_arg);
    cmd.add(hex_script_arg);
    cmd.add(commands_script_arg);
    cmd.parse(argc, argv);*/


    rs2::log_to_file(RS2_LOG_SEVERITY_WARN, "librealsense.log");
    // Obtain a list of devices currently present on the system
    rs2::context ctx = rs2::context();
    rs2::device_hub hub(ctx);
    rs2::device_list all_device_list = ctx.query_devices();
    if (all_device_list.size() == 0) {
        std::cout << "\nLibrealsense is not detecting any devices" << std::endl;
        return EXIT_FAILURE;
    };

    auto dev = all_device_list[0];

    //raw command: use debug_interface
    auto debug_device = dev.as<rs2::debug_protocol>();

    std::vector<uint8_t> raw_cmd = { 0x14, 0x00, 0xab, 0xcd, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    std::vector<uint8_t> res = debug_device.send_and_receive_raw_data(raw_cmd);

    for (auto& elem : res)
        cout << setfill('0') << setw(2) << hex << static_cast<int>(elem) << " ";
    cout << endl;


    std::string xml_full_path("C:\\Users\\rbettan\\Documents\\Dev\\RefFolder\\CommandsDS5.xml");
    //string command
    auto terminal_parser = rs2::terminal_parser(xml_full_path);

    std::string gvd_str("gvd");
    std::vector<uint8_t> xml_res = terminal_parser.parse_command(all_device_list, gvd_str);

    for (auto& elem : xml_res)
        cout << elem;
    cout << endl;


}