// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "terminal-parser.h"
#include <iostream>

namespace librealsense
{

    using namespace std;

    terminal_parser::terminal_parser(const std::string& xml_content)
    {
        if (!xml_content.empty())
        {
            parse_xml_from_memory(xml_content.c_str(), _cmd_xml);
            update_format_type_to_lambda(_format_type_to_lambda);
        }
    }


    std::vector<uint8_t> terminal_parser::parse_command(const std::string& line) const
    {
        command_from_xml command;
        vector<string> params;

        get_command_and_params_from_input(line, command, params);

        // In case of receiving input from file, the data will be retrieved and converted into raw format
        file_argument_to_blob(params);

        auto raw_data = build_raw_command_data(command, params);

        for (auto b : raw_data)
        {
            cout << hex << fixed << setfill('0') << setw(2) << (int)b << " ";
        }
        cout << endl;

        return raw_data;
    }

    std::vector<uint8_t> terminal_parser::parse_response(const std::string& line,
        const std::vector<uint8_t>& response) const
    {
        command_from_xml command;
        vector<string> params;

        get_command_and_params_from_input(line, command, params);

        unsigned returned_opcode = *response.data();
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
            decode_string_from_raw_data(command, _cmd_xml.custom_formatters, response.data(), response.size(), data, _format_type_to_lambda);
            vector<uint8_t> data_vec;
            data_vec.insert(data_vec.begin(), data.begin(), data.end());
            return data_vec;
        }
        else
        {
            return response;
        }
    }

    vector<uint8_t> terminal_parser::build_raw_command_data(const command_from_xml& command, const vector<string>& params) const
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

    void terminal_parser::get_command_and_params_from_input(const std::string& line, command_from_xml& command,
        vector<string>& params) const
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
            throw runtime_error("Invalid input! - no arguments provided");

        auto command_str = utilities::string::to_lower(tokens.front());
        auto it = _cmd_xml.commands.find(command_str);
        if (it == _cmd_xml.commands.end())
            throw runtime_error(to_string() << "Command " << command_str << " was not found!");

        command = it->second;
        for (auto i = 1; i < tokens.size(); ++i)
            params.push_back(tokens[i]);
    }
}
