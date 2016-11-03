// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <iostream>

#include "tclap/CmdLine.h"
#include "parser.hpp"

using namespace std;
using namespace TCLAP;

vector<uint8_t> build_raw_command_data(const command& command, const vector<string>& params)
{
    if (params.size() > command.parameters.size())
        throw runtime_error("Input string was not in a correct format!");

    vector<parameter> vec_parameters;
    for (auto param_index = 0; param_index < params.size() ; ++param_index)
    {
        auto name = command.parameters[param_index].name;
        auto is_reverse_bytes = command.parameters[param_index].is_reverse_bytes;
        auto is_decimal = (param_index < int(command.parameters.size())) ? command.parameters[param_index].is_decimal : false;
        auto format_length = (param_index < int(command.parameters.size())) ? command.parameters[param_index].format_length : -1;
        vec_parameters.push_back(parameter(name, params[param_index], is_decimal, is_reverse_bytes, format_length ));
    }

    vector<uint8_t> raw_data;
    encode_raw_data_command(command, vec_parameters, raw_data);
    return raw_data;
}

void xml_mode(const string& line, const commands_xml& cmd_xml, rs::device& dev, map<string, xml_parser_function>& format_type_to_lambda)
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

    auto command_str = tokens.front();
    auto it = cmd_xml.commands.find(command_str);
    if (it == cmd_xml.commands.end())
        throw runtime_error("Command not found!");

    auto command = it->second;
    vector<string> params;
    for (auto i = 1; i < tokens.size(); ++i)
        params.push_back(tokens[i]);

    auto raw_data = build_raw_command_data(command, params);
    auto result = dev.debug().send_and_receive_raw_data(raw_data);

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

void hex_mode(const string& line, rs::device& dev)
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

    auto result = dev.debug().send_and_receive_raw_data(raw_data);

    cout << endl;
    for (auto& elem : result)
        cout << setfill('0') << setw(2) << hex << static_cast<int>(elem) << " ";
}

int main(int argc, char** argv) try
{
    CmdLine cmd("librealsense cpp-terminal example tool", ' ', RS_API_VERSION_STR);
    ValueArg<string> xml_arg("l", "load", "Full file path of commands XML file", false, "", "Load commands XML file");
    ValueArg<int> device_id_arg("d", "deviceId", "Device ID could be obtain from cpp-enumerate-devices example", false, 0, "Select a device to work with");
    cmd.add(xml_arg);
    cmd.add(device_id_arg);
    cmd.parse(argc, argv);

    // parse command.xml
    rs::log_to_file(RS_LOG_SEVERITY_WARN, "librealsense.log");
    // Obtain a list of devices currently present on the system
    rs::context ctx;
    auto devices = ctx.query_devices();
    int device_count = devices.size();
    if (!device_count)
    {
        printf("No device detected. Is it plugged in?\n");
        return EXIT_SUCCESS;
    }

    auto device_id = device_id_arg.getValue();
    if ((device_id + 1) > device_count || device_id < 0)
    {
        cout << "Given device ID is out of range!\n";
        return EXIT_FAILURE;
    }

    auto dev = devices[device_id];
    auto is_application_in_hex_mode = true;
    auto xml_full_file_path = xml_arg.getValue();
    map<string, xml_parser_function> format_type_to_lambda;
    commands_xml cmd_xml;
    if (!xml_full_file_path.empty())
    {
        auto sts = parse_xml_from_file(xml_full_file_path, cmd_xml);
        if (!sts)
        {
            cout << "Provided XML not found!\n";
            return EXIT_FAILURE;
        }
        cout << "Loaded provided commands XML file.\nNow you can type commands from the XML file.";
        update_format_type_to_lambda(format_type_to_lambda);
        is_application_in_hex_mode = false;
    }
    else
    {
        cout << "Commands XML file not provided.\nyou still can send raw data to device in hexadecimal\nseparated by spaces (e.g. 5A 4B 3C).\n";
    }

    while (true)
    {
        cout << "\n\n#>";
        fflush(nullptr);

        string line;
        getline(cin, line);

        try
        {
            if (is_application_in_hex_mode)
            {
                hex_mode(line, dev);
            }
            else
            {
                xml_mode(line, cmd_xml, dev, format_type_to_lambda);
            }
        }
        catch(const exception& ex)
        {
            cout << endl << ex.what() << endl;
        }
        cout << endl;
    }
}
catch (const rs::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
