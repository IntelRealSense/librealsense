// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <iostream>
#include <functional>

#include "tclap/CmdLine.h"
#include "example.hpp"

#define MAX_PARAMS 4

using namespace std;

void check_section_size(unsigned section_size, unsigned struct_size, const string& section_name, const string& struct_name)
{
    string exception_str("Size of section %s is bigger than %s struct.");
    if (section_size > struct_size)
    {
        auto msg = "Size of section " + section_name + " is bigger than " + struct_name + " struct.";
        throw runtime_error(msg);
    }
}

void update_format_type_to_lambda(map<string, function<void(uint8_t*, const section&, stringstream&)>>& format_type_to_lambda)
{
    format_type_to_lambda.insert(make_pair("ChangeSetVersion", [](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(ChangeSetVersion), sec.name.c_str(), "ChangeSetVersion");
        auto changeSetVersion = reinterpret_cast<ChangeSetVersion*>(data_offset + sec.offset);
        tempStr << static_cast<int>(changeSetVersion->Version_Major) <<
            ((sec.size >= 2) ? ("." + to_string(static_cast<int>(changeSetVersion->Version_Minor))) : "") <<
            ((sec.size >= 3) ? ("." + to_string(static_cast<int>(changeSetVersion->Version_Number))) : "") <<
            ((sec.size >= 4) ? ("." + to_string(static_cast<int>(changeSetVersion->Version_Revision))) : "") <<
            ((sec.size >= 5) ? (" (" + to_string(changeSetVersion->Version_spare)) + ")" : "");
    }));

    format_type_to_lambda.insert(make_pair("MajorMinorVersion", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(MajorMinorVersion), sec.name.c_str(), "MajorMinorVersion");
        auto majorMinorVersion = reinterpret_cast<MajorMinorVersion*>(data_offset + sec.offset);;
        tempStr << static_cast<int>(majorMinorVersion->Version_Major) <<
            ((sec.size >= 2) ? ("." + to_string(static_cast<int>(majorMinorVersion->Version_Minor))) : "");
    }));

    format_type_to_lambda.insert(make_pair("HexByte", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(HexByte), sec.name.c_str(), "HexByte");
        auto hexByte = reinterpret_cast<HexByte*>(data_offset + sec.offset);
        tempStr << hexify(hexByte->Version4);
    }));

    format_type_to_lambda.insert(make_pair("LiguriaVersion", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(LiguriaVersion), sec.name.c_str(), "LiguriaVersion");
        auto liguriaVersion = reinterpret_cast<LiguriaVersion*>(data_offset + sec.offset);
        tempStr << static_cast<int>(liguriaVersion->Version1) <<
            ((sec.size >= 2) ? ("." + to_string(static_cast<int>(liguriaVersion->Version2))) : "");
    }));

    format_type_to_lambda.insert(make_pair("Bool", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(Bool), sec.name.c_str(), "Bool");
        auto BooL = reinterpret_cast<Bool*>(data_offset + sec.offset);
        tempStr << ((static_cast<int>(BooL->sts)) ? "TRUE" : "FALSE");
    }));

    format_type_to_lambda.insert(make_pair("HwTypeNumber", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(HwTypeNumber), sec.name.c_str(), "HwTypeNumber");
        auto hwTypeNumber = reinterpret_cast<HwTypeNumber*>(data_offset + sec.offset);
        tempStr << static_cast<int>(hwTypeNumber->num);
    }));

    format_type_to_lambda.insert(make_pair("Ascii", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(Ascii), sec.name.c_str(), "Ascii");
        auto ascii = reinterpret_cast<Ascii*>(data_offset + sec.offset);
        auto temp = new char[sec.size + 1];
        memcpy(temp, ascii->value, sec.size);
        temp[sec.size] = '\0';
        tempStr << temp;
        delete[] temp;
    }));

    format_type_to_lambda.insert(make_pair("DecByte", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(DecByte), sec.name.c_str(), "DecByte");
        auto decByte = reinterpret_cast<DecByte*>(data_offset + sec.offset);
        tempStr << static_cast<int>(decByte->version);
    }));

    format_type_to_lambda.insert(make_pair("HexNumber", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(HexNumber), sec.name.c_str(), "HexNumber");
        auto hexNumber = reinterpret_cast<HexNumber*>(data_offset + sec.offset);
        tempStr << hexify(hexNumber->number1) <<
            ((sec.size >= 2) ? hexify(hexNumber->number2) : "") <<
            ((sec.size >= 3) ? hexify(hexNumber->number3) : "") <<
            ((sec.size >= 4) ? hexify(hexNumber->number4) : "");
    }));

    format_type_to_lambda.insert(make_pair("HexNumberTwoBytes", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(HexNumber), sec.name.c_str(), "HexNumber");
        auto hexNumber = reinterpret_cast<HexNumber*>(data_offset + sec.offset);
        tempStr << hexify(hexNumber->number2) <<
            ((sec.size >= 2) ? hexify(hexNumber->number1) : "");
    }));

    format_type_to_lambda.insert(make_pair("HexNumberReversed", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(HexNumberReversed), sec.name.c_str(), "HexNumberReversed");
        auto hexNumberReversed = reinterpret_cast<HexNumberReversed*>(data_offset + sec.offset);
        tempStr << hexify(hexNumberReversed->number4) <<
            ((sec.size >= 2) ? hexify(hexNumberReversed->number3) : "") <<
            ((sec.size >= 3) ? hexify(hexNumberReversed->number2) : "") <<
            ((sec.size >= 4) ? hexify(hexNumberReversed->number1) : "");
    }));

    format_type_to_lambda.insert(make_pair("BarCodeSerial12Char", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(BarCodeSerial12Char), sec.name.c_str(), "BarCodeSerial12Char");
        auto barCodeSerial12Char = reinterpret_cast<BarCodeSerial12Char*>(data_offset + sec.offset);
        tempStr << hexify(barCodeSerial12Char->number1) <<
            ((sec.size >= 2) ? hexify(barCodeSerial12Char->number2) : "") <<
            ((sec.size >= 3) ? hexify(barCodeSerial12Char->number3) : "") <<
            ((sec.size >= 4) ? hexify(barCodeSerial12Char->number4) : "") <<
            ((sec.size >= 5) ? hexify(barCodeSerial12Char->number5) : "") <<
            ((sec.size >= 6) ? hexify(barCodeSerial12Char->number6) : "") <<
            ((sec.size >= 7) ? hexify(barCodeSerial12Char->number7) : "") <<
            ((sec.size >= 8) ? hexify(barCodeSerial12Char->number8) : "");
    }));

    format_type_to_lambda.insert(make_pair("WideMajorMinorVersion", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(WideMajorMinorVersion), sec.name.c_str(), "WideMajorMinorVersion");
        auto wideMajorMinorVersion = reinterpret_cast<WideMajorMinorVersion*>(data_offset + sec.offset);
        tempStr << static_cast<int>(wideMajorMinorVersion->Version_Minor) <<
            ((sec.size >= 2) ? ("." + to_string(static_cast<int>(wideMajorMinorVersion->Version_Major))) : "");
    }));

    format_type_to_lambda.insert(make_pair("Double", [&](uint8_t* data_offset, const section& sec, stringstream& tempStr) {
        check_section_size(sec.size, sizeof(DoubleNumber), sec.name.c_str(), "DoubleNumber");
        auto number = reinterpret_cast<DoubleNumber*>(data_offset + sec.offset);
        tempStr << std::setprecision(10) << number->number;
    }));
}

void update_sections_data(uint8_t* raw_data_offset, vector<section>& sections, const map<string, custom_formatter>& custom_formatters, map<string, function<void(uint8_t*, const section&, stringstream&)>>& format_type_to_lambda)
{
    auto data_offset = raw_data_offset + 4;
    for (auto& sec : sections)
    {
        stringstream tempStr;
        auto pair = format_type_to_lambda.find(sec.format_type);
        if (pair != format_type_to_lambda.end())
        {
            pair->second(data_offset, sec, tempStr);
        }
        else
        {
            // FormatType not found. searching in the custom format list
            auto it = custom_formatters.find(to_lower(sec.format_type));
            if (it != custom_formatters.end())
            {
                if (it->second.key_size == "Byte")
                {
                    auto key = uint8_t(*(data_offset + sec.offset));
                    for (auto& elem : it->second.kv)
                    {
                        auto keyStr = to_string(int(key));
                        if (elem.key == keyStr)
                        {
                            tempStr << elem.value;
                            break;
                        }
                    }
                }
            }
        }

        vector<uint8_t> raw_data;
        if (sec.size == 0)
            throw runtime_error(string("Size of section " + sec.name + " is 0.").c_str());

        raw_data.resize(sec.size);
        memcpy(raw_data.data(), data_offset + sec.offset, sec.size);
        sec.data = tempStr.str();
    }
}

void decode_string_from_raw_data(const command& command, const map<string, custom_formatter>& custom_formatters, uint8_t* data, int dataSize, string& output, map<string, function<void(uint8_t*, const section&, stringstream&)>>& format_type_to_lambda)
{
    stringstream ss_output;
    ss_output.str("");
    auto num_of_bytes_for_space = 1;
    auto num_of_bytes_for_new_line = 16;
    auto read_format = command.read_format;
    if (read_format == "Bytes")
        num_of_bytes_for_space = 1;
    else if (read_format == "Words")
        num_of_bytes_for_space = 2;
    else if (read_format == "Doubles")
        num_of_bytes_for_space = 4;


    if (command.read_data.sections.empty())
    {
        vector<string> buffer;
        auto bytes = 0;
        auto bytes_per_line = 0;
        auto start_address = "0";
        string curr_address = start_address;
        stringstream dec_to_hex;
        auto end_address = stoi(start_address) + dataSize;
        auto num_of_zeros = 1;

        if (dataSize == 0)
            return; // TODO: throw an exception

        if (dataSize != 0)
        {
            if (curr_address.size() == 1)
            {
                auto temp_curr_address = curr_address;
                auto temp_end_address = end_address;
                auto temp_bytes_per_line = bytes_per_line;

                for (auto offset = 0; offset < dataSize; offset++)
                {
                    temp_bytes_per_line++;
                    if (temp_bytes_per_line == num_of_bytes_for_new_line)
                    {

                        int next_add = stoi(temp_curr_address) + num_of_bytes_for_new_line;

                        if (next_add >= temp_end_address)
                            break;

                        dec_to_hex.str("");
                        dec_to_hex << std::hex << next_add;
                        temp_curr_address = dec_to_hex.str();

                        if (temp_curr_address.size() == 1)
                            temp_curr_address.insert(0, "0");

                        temp_curr_address = to_string(next_add);
                        temp_bytes_per_line = 0;
                    }
                }


                num_of_zeros = int(dec_to_hex.str().length());
                ss_output << "Address: 0x";
                dec_to_hex.str("");

                for (auto i = 1; i < num_of_zeros; i++)
                {
                    ss_output << "0";
                }
            }
            else
                ss_output << "Address: 0x";

            ss_output << std::hex << stoi(curr_address) << " => ";
        }


        for (auto offset = 0; offset < dataSize; ++offset)
        {
            bytes_per_line++;
            bytes++;
            buffer.push_back(hexify(static_cast<unsigned char>(*(data + offset))));

            if (bytes == num_of_bytes_for_space)
            {
                bytes = 0;
                reverse(buffer.begin(), buffer.end());

                for (auto str : buffer)
                    ss_output << str;

                buffer.clear();

                if (bytes_per_line == num_of_bytes_for_new_line)
                {
                    auto next_add = stoi(curr_address) + num_of_bytes_for_new_line;

                    if (next_add >= end_address)
                        break;

                    dec_to_hex.str("");
                    dec_to_hex << std::hex << next_add;
                    curr_address = dec_to_hex.str();

                    auto putZeros = num_of_zeros - int(curr_address.size());
                    ss_output << "\nAddress: 0x";

                    for (auto i = 0; i < putZeros; i++)
                    {
                        ss_output << "0";
                    }

                    ss_output << curr_address << " => ";


                    curr_address = to_string(next_add);
                    bytes_per_line = 0;
                }
                else
                {
                    if ((offset + 1) < dataSize)
                        ss_output << " ";
                }
            }
        }
    }
    else
    {
        auto sections = command.read_data.sections;
        update_sections_data(data, sections, custom_formatters, format_type_to_lambda);
        unsigned max_line_len = 0;
        for (auto& elem : sections)
            max_line_len = ((elem.name.size() > max_line_len) ? unsigned(elem.name.size()) : max_line_len);

        const int spaces = 3; // number of spaces between section name to section data
        for (auto& elem : sections)
            ss_output << elem.name << ":" << setw((max_line_len + spaces) - elem.name.size() + elem.data.length()) << right << elem.data << endl;
    }

    output = ss_output.str();
}

void encode_raw_data_command(const command& xml_cmd_info, const vector<parameter>& params, vector<uint8_t>& raw_data)
{
    auto cmd_op_code = xml_cmd_info.op_code;
    auto num_of_required_parameters = xml_cmd_info.num_of_parameters;
    auto is_cmd_writes_data = xml_cmd_info.is_cmd_write_data;
    auto parameters = params;

    if (int(params.size()) < num_of_required_parameters)
        throw runtime_error("number of given parameters is less than required"); // TODO: consider to print the command info

    auto format_length = Byte;
    if (is_cmd_writes_data)
        format_length = ((params[num_of_required_parameters].format_length == -1) ? format_length : static_cast<FormatSize>(params[num_of_required_parameters].format_length));

    uint16_t pre_header_data = 0xcdab;  // magic number
    raw_data.resize(1024); // TODO: resize vector to the right size
    auto write_ptr = raw_data.data();
    auto header_size = 4;


    auto cur_index = 2;
    *reinterpret_cast<uint16_t *>(write_ptr + cur_index) = pre_header_data;
    cur_index += sizeof(uint16_t);
    *reinterpret_cast<unsigned int *>(write_ptr + cur_index) = cmd_op_code;
    cur_index += sizeof(unsigned int);

    // Parameters
    for (auto param_index = 0; param_index < MAX_PARAMS; ++param_index)
    {
        if (param_index < num_of_required_parameters)
        {
            *reinterpret_cast<unsigned*>(write_ptr + cur_index) = stoul(params[param_index].data);
            cur_index += sizeof(unsigned);
        }
        else
        {
            *reinterpret_cast<unsigned *>(write_ptr + cur_index) = 0;
            cur_index += sizeof(unsigned);
        }
    }

    // Data
    if (is_cmd_writes_data)
    {
        for (auto j = num_of_required_parameters; j < int(params.size()); ++j)
        {
            switch (format_length)
            {
            case Byte:
                *reinterpret_cast<uint8_t*>(write_ptr + cur_index) = static_cast<uint8_t>(stoul(params[j].data));
                cur_index += sizeof(uint8_t);
                break;

            case Word:
                *reinterpret_cast<short *>(write_ptr + cur_index) = static_cast<short>(stoul(params[j].data));
                cur_index += sizeof(short);
                break;

            case Double:
                *reinterpret_cast<unsigned *>(write_ptr + cur_index) = static_cast<unsigned>(stoul(params[j].data));
                cur_index += sizeof(unsigned);
                break;

            default:
                *reinterpret_cast<uint8_t*>(write_ptr + cur_index) = static_cast<uint8_t>(stoul(params[j].data));
                cur_index += sizeof(uint8_t);
                break;
            }
        }
    }

    *reinterpret_cast<uint16_t*>(raw_data.data()) = static_cast<uint16_t>(cur_index - header_size);// Length doesn't include hdr.
    raw_data.resize(cur_index); // TODO:
}

vector<uint8_t> build_raw_command_data(const command& command, const string& line)
{
    vector<string> params;
    vector<parameter> vec_parameters;
    for (auto param_index = 1; param_index < params.size() ; ++param_index)
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

void xml_mode(const string& line, const commands_xml& cmd_xml, rs::device& dev, map<string, function<void(uint8_t*, const section&, stringstream&)>>& format_type_to_lambda)
{
    vector<string> tokens;
    split(line, tokens, ' ');
    auto command_str = tokens.front();

    auto it = cmd_xml.commands.find(command_str);
    if (it == cmd_xml.commands.end())
        throw runtime_error("Command not found!");

    auto command = it->second;

    auto raw_data = build_raw_command_data(command, line);
    auto result = dev.debug().send_and_receive_raw_data(raw_data);

    string data;
    decode_string_from_raw_data(command, cmd_xml.custom_formatters, result.data(), result.size(), data, format_type_to_lambda);
    cout << endl << data << endl;
}

void hex_mode(const string& line, rs::device& dev)
{
    vector<uint8_t> raw_data;
    stringstream ss(line);
    std::string word;
    while (ss >> word)
    {
        stringstream converter;
        int temp;
        converter << std::hex << word;
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
    //CmdLine cmd("librealsense cpp-enumerate-devices example tool", ' ', RS_API_VERSION_STR);

    //SwitchArg compact_view_arg("s", "short", "Provide short summary of the devices");
    //SwitchArg show_options("o", "option", "Show all supported options per subdevice");
    //SwitchArg show_modes("m", "modes", "Show all supported stream modes per subdevice");
    //cmd.add(compact_view_arg);
    //cmd.add(show_options);
    //cmd.add(show_modes);

    //cmd.parse(argc, argv);

    // parse command.xml
    rs::log_to_file(RS_LOG_SEVERITY_WARN, "librealsense.log");
    // Obtain a list of devices currently present on the system
    rs::context ctx;
    auto devices = ctx.query_devices();
    int device_count = devices.size();
    bool is_application_in_hex_mode = false;
    if (!device_count)
    {
        printf("No device detected. Is it plugged in?\n");
        return EXIT_SUCCESS;
    }

    auto dev = devices.front(); // use first device
    map<string, function<void(uint8_t*, const section&, stringstream&)>> format_type_to_lambda;
    commands_xml cmd_xml;
    auto sts = parse_xml_from_file("./CommandsDS5.xml", cmd_xml); // TODO: file name from prompt
    if (!sts)
    {
        cout << "XML file not found!\nMoving to hex console mode.\n\n";
        is_application_in_hex_mode = true;
    }
    else
    {
        update_format_type_to_lambda(format_type_to_lambda);
    }

    auto loop = true;
    while (loop)
    {
        cout << "\n\n#>";
        fflush(nullptr);

        string line;
        getline(cin, line);

        try {
            if (is_application_in_hex_mode)
            {
                hex_mode(line, dev);
            }
            else
            {
                xml_mode(line, cmd_xml, dev, format_type_to_lambda);
            }
        }
        catch(exception ex)
        {
            cout << endl << ex.what() << endl;
        }
        cout << endl;
    }

    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
