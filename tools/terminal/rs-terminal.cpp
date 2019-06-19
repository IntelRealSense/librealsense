// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <iostream>
#include <fstream>

#include "tclap/CmdLine.h"
#include "parser.hpp"
#include "auto-complete.h"

#include <string>


using namespace std;
using namespace TCLAP;


vector<uint8_t> build_raw_command_data(const command& command, const vector<string>& params)
{
    if (params.size() > command.parameters.size() && !command.is_cmd_write_data)
        throw runtime_error("Input string was not in a correct format!");

    vector<parameter> vec_parameters;
    for (auto param_index = 0; param_index < params.size(); ++param_index)
    {
        auto is_there_write_data = param_index >= int(command.parameters.size());
        auto name = (is_there_write_data) ? "" : command.parameters[param_index].name;
        auto is_reverse_bytes = (is_there_write_data) ? false : command.parameters[param_index].is_reverse_bytes;
        auto is_decimal = (is_there_write_data) ? false : command.parameters[param_index].is_decimal;
        auto format_length = (is_there_write_data) ? -1 : command.parameters[param_index].format_length;
        vec_parameters.push_back(parameter(name, params[param_index], is_decimal, is_reverse_bytes, format_length));
    }

    vector<uint8_t> raw_data;
    encode_raw_data_command(command, vec_parameters, raw_data);
    return raw_data;
}

void xml_mode(const string& line, const commands_xml& cmd_xml, rs2::device& dev, map<string, xml_parser_function>& format_type_to_lambda)
{
    vector<string> tokens;
    stringstream ss(line);
    string word;
    while (ss >> word)
    {
        stringstream converter;
        converter << hex << word;
        tokens.push_back(word);
    }

    if (tokens.empty())
        throw runtime_error("Wrong input!");

    auto command_str = tokens.front();
    auto it = cmd_xml.commands.find(command_str);
    if (it == cmd_xml.commands.end())
        throw runtime_error("Command not found!");

    auto command = it->second;
    vector<string> params;
    for (auto i = 1; i < tokens.size(); ++i)
        params.push_back(tokens[i]);

    auto raw_data = build_raw_command_data(command, params);

    for (auto b : raw_data)
    {
        cout << hex << fixed << setfill('0') << setw(2) << (int)b << " ";
    }
    cout << endl;

    auto result = dev.as<rs2::debug_protocol>().send_and_receive_raw_data(raw_data);

    unsigned returned_opcode = *result.data();
    // check returned opcode
    if (command.op_code != returned_opcode)
    {
        stringstream msg;
        msg << "OpCodes do not match! Sent 0x" << hex << command.op_code << " but received 0x" << hex << (returned_opcode) << "!";
        throw runtime_error(msg.str());
    }

    if (command.is_read_command)
    {
        string data;
        decode_string_from_raw_data(command, cmd_xml.custom_formatters, result.data(), result.size(), data, format_type_to_lambda);
        cout << endl << data << endl;
    }
    else
    {
        cout << endl << "Done!" << endl;
    }
}

void hex_mode(const string& line, rs2::device& dev)
{
    vector<uint8_t> raw_data;
    stringstream ss(line);
    string word;
    while (ss >> word)
    {
        stringstream converter;
        int temp;
        converter << hex << word;
        converter >> temp;
        raw_data.push_back(temp);
    }
    if (raw_data.empty())
        throw runtime_error("Wrong input!");

    auto result = dev.as<rs2::debug_protocol>().send_and_receive_raw_data(raw_data);

    cout << endl;
    for (auto& elem : result)
        cout << setfill('0') << setw(2) << hex << static_cast<int>(elem) << " ";
}

auto_complete get_auto_complete_obj(bool is_application_in_hex_mode, const map<string, command>& commands_map)
{
    set<string> commands;
    if (!is_application_in_hex_mode)
    {
        for (auto& elem : commands_map)
            commands.insert(elem.first);
    }
    return auto_complete(commands, is_application_in_hex_mode);
}

void read_script_file(const string& full_file_path, vector<string>& hex_lines)
{
    ifstream myfile(full_file_path);
    if (myfile.is_open())
    {
        string line;
        while (getline(myfile, line))
            hex_lines.push_back(line);

        myfile.close();
        return;
    }
    throw runtime_error("Script file not found!");
}

rs2::device wait_for_device(const rs2::device_hub& hub, bool print_info = true)
{
    if (print_info)
        cout << "\nWaiting for RealSense device to connect...\n";

    auto dev = hub.wait_for_device();

    if (print_info)
        cout << "RealSense device has connected...\n";

    return dev;
}

int main(int argc, char** argv)
{
    CmdLine cmd("librealsense rs-terminal example tool", ' ', RS2_API_VERSION_STR);
    ValueArg<string> xml_arg("l", "load", "Full file path of commands XML file", false, "", "Load commands XML file");
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
    cmd.parse(argc, argv);

    // parse command.xml
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN, "librealsense.log");
    // Obtain a list of devices currently present on the system
    rs2::context ctx = rs2::context();
    rs2::device_hub hub(ctx);
    rs2::device_list all_device_list = ctx.query_devices();
    if (all_device_list.size() == 0) {
        std::cout << "\nLibrealsense is not detecting any devices" << std::endl;
        return EXIT_FAILURE;
    };

    std::vector<rs2::device> rs_device_list;
    //Ensure that diviceList only has realsense devices in it. tmpList contains webcams as well
    for (size_t i = 0; i < all_device_list.size(); i++) {
        try {
            all_device_list[i].get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION);
            rs_device_list.push_back(all_device_list[i]);
        }
        catch (...) {
            continue;
        }

    }
    auto num_rs_devices = rs_device_list.size();
    if (rs_device_list.size() == 0) {
        std::cout << "\nLibrealsense is not detecting any Realsense cameras" << std::endl;
        return EXIT_FAILURE;
    };

    auto xml_full_file_path = xml_arg.getValue();
    map<string, xml_parser_function> format_type_to_lambda;
    commands_xml cmd_xml;
    auto is_application_in_hex_mode = true;

    if (!xml_full_file_path.empty())
    {
        auto sts = parse_xml_from_file(xml_full_file_path, cmd_xml);
        if (!sts)
        {
            cout << "Provided XML not found!\n";
            return EXIT_FAILURE;
        }

        update_format_type_to_lambda(format_type_to_lambda);
        is_application_in_hex_mode = false;
        cout << "Commands XML file - " << xml_full_file_path << " was loaded successfully. Type commands by name (e.g.'gvd'`).\n";
    }
    else
    {
        cout << "Commands XML file not provided.\nyou still can send raw data to device in hexadecimal\nseparated by spaces.\n";
        cout << "Example GVD command for the SR300:\n14 00 ab cd 3b 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00\n";
        cout << "Example GVD command for the RS4xx:\n14 00 ab cd 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00\n";
    }
    auto auto_comp = get_auto_complete_obj(is_application_in_hex_mode, cmd_xml.commands);


    std::vector<rs2::device> selected_rs_devices;
    while (true)
    {
        if (all_devices_arg.isSet()) {
            for (size_t i = 0; i < num_rs_devices; i++) {
                std::string sn = std::string(rs_device_list[i].get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                selected_rs_devices.push_back(rs_device_list[i]);
                std::cout << "\nDevice with Serial Number:  " << rs_device_list[i].get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << " has loaded.\n";
            }
        }
        else if (specific_SN_arg.isSet()) {
            auto desired_sn = specific_SN_arg.getValue();
            bool device_not_found = true;
            for (size_t i = 0; i < num_rs_devices; i++) {
                std::string device_sn = std::string(rs_device_list[i].get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                if (device_sn.compare(desired_sn) == 0) { //std::compare returns 0 if the strings are the same
                    selected_rs_devices.push_back(rs_device_list[i]);
                    device_not_found = false;
                    std::cout << "\nDevice with SN:  " << device_sn << " has loaded.\n";
                    break;
                }
            }
            if (device_not_found) {
                std::cout << "\nGiven device serial number doesn't exist! desired serial number=" << desired_sn << std::endl;
                return EXIT_FAILURE;
            }

        }
        else if (device_id_arg.isSet())
        {
            auto dev_id = device_id_arg.getValue();
            if (num_rs_devices < (dev_id + 1))
            {
                std::cout << "\nGiven device_id doesn't exist! device_id=" <<
                    dev_id << " ; connected devices=" << num_rs_devices << std::endl;
                return EXIT_FAILURE;
            }

            for (int i = 0; i < (num_rs_devices - 1); ++i)
            {
                wait_for_device(hub, true);
            }
            selected_rs_devices.push_back(rs_device_list[dev_id]);
            std::cout << "\nDevice ID " << dev_id << " has loaded.\n";
        }
        else if (rs_device_list.size() == 1)
        {
            selected_rs_devices.push_back(rs_device_list[0]);
        }
        else
        {
            std::cout << "\nEnter a command line option:" << std::endl;
            std::cout << "-d to choose by device number" << std::endl;
            std::cout << "-n to choose by serial number" << std::endl;
            std::cout << "-a to send to all devices" << std::endl;
            return EXIT_FAILURE;
        }

        if (selected_rs_devices.empty()) {
            std::cout << "\nNo devices were selected. Recheck input arguments" << std::endl;
            return EXIT_FAILURE;
        }
        fflush(nullptr);
        if (hex_cmd_arg.isSet())
        {
            for (auto dev : selected_rs_devices) {
                auto line = hex_cmd_arg.getValue();
                try
                {
                    hex_mode(line, dev);
                }
                catch (const exception& ex)
                {
                    cout << endl << ex.what() << endl;
                    continue;
                }
            }
            return EXIT_SUCCESS;

        }

        std::string script_file("");
        vector<string> script_lines;
        if (hex_script_arg.isSet())
        {
            script_file = hex_script_arg.getValue();
        }
        else if (commands_script_arg.isSet())
        {
            script_file = commands_script_arg.getValue();
        }

        if (!script_file.empty())
            read_script_file(script_file, script_lines);

        if (hex_script_arg.isSet())
        {
            for (auto dev : selected_rs_devices) {
                try
                {
                    for (auto& elem : script_lines)
                        hex_mode(elem, dev);
                }
                catch (const exception& ex)
                {
                    cout << endl << ex.what() << endl;
                    continue;
                }
            }
            return EXIT_SUCCESS;
        }




        if (commands_script_arg.isSet())
        {
            for (auto dev : selected_rs_devices) {
                try
                {
                    for (auto& elem : script_lines)
                        xml_mode(elem, cmd_xml, dev, format_type_to_lambda);
                }
                catch (const exception& ex)
                {
                    cout << endl << ex.what() << endl;
                    continue;
                }
            }
            return EXIT_SUCCESS;
        }

        auto dev = selected_rs_devices[0];
        while (hub.is_connected(dev))
        {
            try
            {
                cout << "\n\n#>";
                fflush(nullptr);
                string line = "";
                line = auto_comp.get_line([&]() {return !hub.is_connected(dev); });
                if (!hub.is_connected(dev))
                    continue;


                if (line == "next")
                {
                    dev = wait_for_device(hub);
                    continue;
                }
                if (line == "exit")
                {
                    return EXIT_SUCCESS;
                }

                for (auto&& dev : selected_rs_devices)
                {
                    if (!hub.is_connected(dev))
                        continue;

                    if (is_application_in_hex_mode)
                    {
                        hex_mode(line, dev);
                    }
                    else
                    {
                        xml_mode(line, cmd_xml, dev, format_type_to_lambda);
                    }
                    cout << endl;
                }
            }
            catch (const rs2::error & e)
            {
                cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
            }
            catch (const exception & e)
            {
                cerr << e.what() << endl;
            }
        }

    }
}

