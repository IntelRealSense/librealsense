// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <iostream>

#include "tclap/CmdLine.h"
#include "../src/hw-monitor.h"
#include "example.hpp"

#define MAX_PARAMS 4

using namespace std;

struct parameter_info
{
    parameter_info()
    : parameter(""),
    is_decimal(false),
    format_length(-1)
    {}
    parameter_info(string param, bool is_dec, int format_len)
    : parameter(param),
    is_decimal(is_dec),
    format_length(format_len)
    {}
    string parameter;
    bool is_decimal;
    int format_length;
};

// units of nibbles
enum FormatSize {
    Byte = 2,
    Word = 4,
    Double = 8
};

enum application_mode
{
    xml,
    hexa
};

void update_sections_data(uint8_t* raw_data, vector<section>& sections, const map<string, custom_formatter>& custom_formatters)
{
    auto data = raw_data + 4;
    string exception_str = "Size of section %s is bigger than %s struct.";

    for (auto& sec : sections)
    {
        stringstream tempStr;
        auto format_type =  sec.format_type;
        if (format_type == "ChangeSetVersion")
        {
            ChangeSetVersion changeSetVersion;
            if (sec.size > sizeof(ChangeSetVersion))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "ChangeSetVersion");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "ChangeSetVersion");
                throw exception(buffer.data());
            }

            memcpy_s(&changeSetVersion, sec.size, data + sec.offset, sec.size);
            tempStr << static_cast<int>(changeSetVersion.Version_Major) <<
                ((sec.size >= 2) ? ("." + to_string(static_cast<int>(changeSetVersion.Version_Minor))) : "") <<
                ((sec.size >= 3) ? ("." + to_string(static_cast<int>(changeSetVersion.Version_Number))) : "") <<
                ((sec.size >= 4) ? ("." + to_string(static_cast<int>(changeSetVersion.Version_Revision))) : "") <<
                ((sec.size >= 5) ? (" (" + to_string(changeSetVersion.Version_spare)) + ")" : "");
        }
        else if (format_type == "MajorMinorVersion")
        {
            MajorMinorVersion majorMinorVersion;
            if (sec.size > sizeof(MajorMinorVersion))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "MajorMinorVersion");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "MajorMinorVersion");
                throw exception(buffer.data());
            }

            memcpy_s(&majorMinorVersion, sec.size, data + sec.offset, sec.size);
            tempStr << static_cast<int>(majorMinorVersion.Version_Major) <<
                ((sec.size >= 2) ? ("." + to_string(static_cast<int>(majorMinorVersion.Version_Minor))) : "");
        }
        else if (format_type == "HexByte")
        {
            HexByte hexByte;
            if (sec.size > sizeof(HexByte))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "HexByte");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "HexByte");
                throw exception(buffer.data());
            }

            memcpy_s(&hexByte, sec.size, data + sec.offset, sec.size);
            tempStr << hexify(hexByte.Version4);
        }
        else if (format_type == "LiguriaVersion")
        {
            LiguriaVersion liguriaVersion;
            if (sec.size > sizeof(LiguriaVersion))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "LiguriaVersion");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "LiguriaVersion");
                throw exception(buffer.data());
            }

            memcpy_s(&liguriaVersion, sec.size, data + sec.offset, sec.size);
            tempStr << static_cast<int>(liguriaVersion.Version1) <<
                ((sec.size >= 2) ? ("." + to_string(static_cast<int>(liguriaVersion.Version2))) : "");
        }
        else if (format_type == "Bool")
        {
            Bool BooL;
            if (sec.size > sizeof(Bool))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "Bool");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "Bool");
                throw exception(buffer.data());
            }

            memcpy_s(&BooL, sec.size, data + sec.offset, sec.size);
            tempStr << ((static_cast<int>(BooL.sts)) ? "TRUE" : "FALSE");
        }
        else if (format_type == "HwTypeNumber")
        {
            HwTypeNumber hwTypeNumber;
            if (sec.size > sizeof(HwTypeNumber))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "HwTypeNumber");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "HwTypeNumber");
                throw exception(buffer.data());
            }

            memcpy_s(&hwTypeNumber, sec.size, data + sec.offset, sec.size);
            tempStr << static_cast<int>(hwTypeNumber.num);
        }
        else if (format_type == "Ascii")
        {
            Ascii ascii;
            if (sec.size > sizeof(Ascii))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "Ascii");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "Ascii");
                throw exception(buffer.data());
            }

            memcpy_s(&ascii, sec.size, data + sec.offset, sec.size);
            char* temp = new char[sec.size + 1];
            memcpy_s(temp, sec.size, ascii.value, sec.size);
            temp[sec.size] = '\0';
            tempStr << temp;
            delete[] temp;
        }
        else if (format_type == "DecByte")
        {
            DecByte decByte;
            if (sec.size > sizeof(DecByte))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "DecByte");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "DecByte");
                throw exception(buffer.data());
            }

            memcpy_s(&decByte, sec.size, data + sec.offset, sec.size);
            tempStr << static_cast<int>(decByte.version);
        }
        else if (format_type == "HexNumber")
        {
            HexNumber hexNumber;
            if (sec.size > sizeof(HexNumber))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "HexNumber");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "HexNumber");
                throw exception(buffer.data());
            }

            memcpy_s(&hexNumber, sec.size, data + sec.offset, sec.size);
            tempStr << hexify(hexNumber.number1) <<
                ((sec.size >= 2) ? hexify(hexNumber.number2) : "") <<
                ((sec.size >= 3) ? hexify(hexNumber.number3) : "") <<
                ((sec.size >= 4) ? hexify(hexNumber.number4) : "");
        }
        else if (format_type == "HexNumberTwoBytes")
        {
            HexNumber hexNumber;
            if (sec.size > sizeof(HexNumber))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "HexNumber");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "HexNumber");
                throw exception(buffer.data());
            }

            memcpy_s(&hexNumber, sec.size, data + sec.offset, sec.size);
            tempStr << hexify(hexNumber.number2) <<
                ((sec.size >= 2) ? hexify(hexNumber.number1) : "");
        }
        else if (format_type == "HexNumberReversed")
        {
            HexNumberReversed hexNumberReversed;
            if (sec.size > sizeof(HexNumberReversed))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "HexNumberReversed");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "HexNumberReversed");
                throw exception(buffer.data());
            }

            memcpy_s(&hexNumberReversed, sec.size, data + sec.offset, sec.size);
            tempStr << hexify(hexNumberReversed.number4) <<
                ((sec.size >= 2) ? hexify(hexNumberReversed.number3) : "") <<
                ((sec.size >= 3) ? hexify(hexNumberReversed.number2) : "") <<
                ((sec.size >= 4) ? hexify(hexNumberReversed.number1) : "");
        }
        else if (format_type == "BarCodeSerial12Char")
        {
            BarCodeSerial12Char barCodeSerial12Char;
            if (sec.size > sizeof(BarCodeSerial12Char))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "BarCodeSerial12Char");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "BarCodeSerial12Char");
                throw exception(buffer.data());
            }

            memcpy_s(&barCodeSerial12Char, sec.size, data + sec.offset, sec.size);
            tempStr << hexify(barCodeSerial12Char.number1) <<
                ((sec.size >= 2) ? hexify(barCodeSerial12Char.number2) : "") <<
                ((sec.size >= 3) ? hexify(barCodeSerial12Char.number3) : "") <<
                ((sec.size >= 4) ? hexify(barCodeSerial12Char.number4) : "") <<
                ((sec.size >= 5) ? hexify(barCodeSerial12Char.number5) : "") <<
                ((sec.size >= 6) ? hexify(barCodeSerial12Char.number6) : "") <<
                ((sec.size >= 7) ? hexify(barCodeSerial12Char.number7) : "") <<
                ((sec.size >= 8) ? hexify(barCodeSerial12Char.number8) : "");
        }
        else if (format_type == "WideMajorMinorVersion")
        {
            WideMajorMinorVersion wideMajorMinorVersion;
            if (sec.size > sizeof(WideMajorMinorVersion))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "WideMajorMinorVersion");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "WideMajorMinorVersion");
                throw exception(buffer.data());
            }

            memcpy_s(&wideMajorMinorVersion, sec.size, data + sec.offset, sec.size);
            tempStr << static_cast<int>(wideMajorMinorVersion.Version_Minor) <<
                ((sec.size >= 2) ? ("." + to_string(static_cast<int>(wideMajorMinorVersion.Version_Major))) : "");
        }
        else if (format_type == "Double")
        {
            DoubleNumber number;
            if (sec.size > sizeof(DoubleNumber))
            {
                auto size = snprintf(nullptr, 0, exception_str.c_str(), sec.name.c_str(), "DoubleNumber");
                std::vector<char> buffer(size + 1);
                snprintf(&buffer[0], buffer.size(), exception_str.c_str(), sec.name.c_str(), "DoubleNumber");
                throw exception(buffer.data());
            }

            memcpy_s(&number, sec.size, data + sec.offset, sec.size);
            tempStr << std::setprecision(10) << number.number;
        }


        if (tempStr.str().empty()) // FormatType not found. searching in the custom format list
        {
            auto it = custom_formatters.find(to_lower(sec.format_type));
            if (it != custom_formatters.end())
            {
                if (!strcmp(it->second.key_size.c_str(), "Byte"))
                {
                    auto key = uint8_t(*(data + sec.offset));
                    for (auto& elem : it->second.kv)
                    {
                        auto keyStr = to_string(int(key));
                        if (!strcmp(elem.key.c_str(), keyStr.c_str()))
                        {
                            tempStr << elem.value;
                            break;
                        }
                    }
                }
            }
        }


        vector<uint8_t> rawData;
        if (sec.size == 0)
            throw exception(string("Size of section " + sec.name + " is 0.").c_str());

        rawData.resize(sec.size);
        memcpy_s(rawData.data(), sec.size, data + sec.offset, sec.size);
        sec.data = tempStr.str();
    }
}

void decode_raw_data(const command& command, const map<string, custom_formatter>& custom_formatters, uint8_t* data, int dataSize, stringstream& _stringOutput)
{
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
                auto tempCurrAddress = curr_address;
                auto tempEndAddress = end_address;
                auto tempBytesPerLine = bytes_per_line;

                for (auto offset = 0; offset < dataSize; offset++)
                {
                    tempBytesPerLine++;
                    if (tempBytesPerLine == num_of_bytes_for_new_line)
                    {

                        int nextAdd = stoi(tempCurrAddress) + num_of_bytes_for_new_line;

                        if (nextAdd >= tempEndAddress)
                            break;

                        dec_to_hex.str("");
                        dec_to_hex << std::hex << nextAdd;
                        tempCurrAddress = dec_to_hex.str();

                        if (tempCurrAddress.size() == 1)
                            tempCurrAddress.insert(0, "0");

                        tempCurrAddress = to_string(nextAdd);
                        tempBytesPerLine = 0;
                    }
                }


                num_of_zeros = int(dec_to_hex.str().length());
                _stringOutput << "Address: 0x";
                dec_to_hex.str("");

                for (auto i = 1; i < num_of_zeros; i++)
                {
                    _stringOutput << "0";
                }
            }
            else
                _stringOutput << "Address: 0x";

            _stringOutput << std::hex << stoi(curr_address) << " => ";
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
                    _stringOutput << str;

                buffer.clear();

                if (bytes_per_line == num_of_bytes_for_new_line)
                {
                    auto nextAdd = stoi(curr_address) + num_of_bytes_for_new_line;

                    if (nextAdd >= end_address)
                        break;

                    dec_to_hex.str("");
                    dec_to_hex << std::hex << nextAdd;
                    curr_address = dec_to_hex.str();

                    auto putZeros = num_of_zeros - int(curr_address.size());
                    _stringOutput << "\nAddress: 0x";

                    for (auto i = 0; i < putZeros; i++)
                    {
                        _stringOutput << "0";
                    }

                    _stringOutput << curr_address << " => ";


                    curr_address = to_string(nextAdd);
                    bytes_per_line = 0;
                }
                else
                {
                    if ((offset + 1) < dataSize)
                        _stringOutput << " ";
                }
            }
        }
    }
    else
    {
        auto sections = command.read_data.sections;
        update_sections_data(data, sections, custom_formatters);
        unsigned maxLineLen = 0;
        for (auto& elem : sections)
            maxLineLen = ((elem.name.size() > maxLineLen) ? unsigned(elem.name.size()) : maxLineLen);

        for (auto& elem : sections)
            _stringOutput << elem.name << ":" << setw((maxLineLen + 3) - elem.name.size() + elem.data.length()) << right << elem.data << endl;
    }
}

void encode_command(const command& xml_cmd_info, const vector<parameter_info>& params, vector<uint8_t>& raw_data)
{
    auto cmd_op_code = xml_cmd_info.op_code;
    auto num_of_required_parameters = xml_cmd_info.num_of_parameters;
    auto is_cmd_writes_data = xml_cmd_info.is_cmd_write_data;
    auto parameters = params;

    if (int(params.size()) < num_of_required_parameters)
        throw exception("number of given parameters is less than required"); // TODO: consider to print the command info

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
            *reinterpret_cast<unsigned*>(write_ptr + cur_index) = stoul(params[param_index].parameter);
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
                *reinterpret_cast<uint8_t*>(write_ptr + cur_index) = static_cast<uint8_t>(stoul(params[j].parameter));
                cur_index += sizeof(uint8_t);
                break;

            case Word:
                *reinterpret_cast<short *>(write_ptr + cur_index) = static_cast<short>(stoul(params[j].parameter));
                cur_index += sizeof(short);
                break;

            case Double:
                *reinterpret_cast<unsigned *>(write_ptr + cur_index) = static_cast<unsigned>(stoul(params[j].parameter));
                cur_index += sizeof(unsigned);
                break;

            default:
                *reinterpret_cast<uint8_t*>(write_ptr + cur_index) = static_cast<uint8_t>(stoul(params[j].parameter));
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
    vector<parameter_info> vec_parameters;
    for (auto param_index = 1; param_index < params.size() ; ++param_index)
    {
        auto isDecimal = (param_index < int(command.parameters.size())) ? command.parameters[param_index].is_decimal : false;
        auto formatLength = (param_index < int(command.parameters.size())) ? command.parameters[param_index].format_length : -1;
        vec_parameters.push_back(parameter_info(params[param_index], isDecimal, formatLength));
    }

    vector<uint8_t> raw_data;
    encode_command(command, vec_parameters, raw_data);
    return raw_data;
}

void xml_mode(const string& line, const commands_xml& cmd_xml, rs::device& dev)
{
    vector<string> tokens;
    split(line, tokens, ' ');
    auto command_str = tokens.front();

    auto it = cmd_xml.commands.find(command_str);
    if (it == cmd_xml.commands.end())
        throw exception("Command not found!");

    auto command = it->second;

    auto raw_data = build_raw_command_data(command, line);
    auto result = dev.debug().send_and_receive_raw_data(raw_data);

    stringstream ss;
    decode_raw_data(command, cmd_xml.custom_formatters, result.data(), result.size(), ss);
    cout << ss.str() << endl;
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
    rs::log_to_console(RS_LOG_SEVERITY_WARN);
    // Obtain a list of devices currently present on the system
    rs::context ctx;
    auto devices = ctx.query_devices();
    int device_count = devices.size();
    auto app_mode = xml;
    if (!device_count)
    {
        printf("No device detected. Is it plugged in?\n");
        return EXIT_SUCCESS;
    }
    auto dev = devices.front(); // choose first device

    commands_xml cmd_xml;
    auto sts = parse_xml_from_file("C:/Program Files/Intel/IPDev/bin/Release/CommandsDS5.xml", cmd_xml);
    if (!sts)
    {
        cout << "XML file not found!\nMoving to hex console mode.\n\n";
        app_mode = hexa;
    }

    bool loop = true;
    while (loop)
    {
        fflush(nullptr);
        cout << "\n\n#>";
        string line;
        getline(cin, line);

        switch (app_mode)
        {
        case xml:
            xml_mode(line, cmd_xml, dev);
            break;
        case hexa:
            hex_mode(line, dev);
            break;
        default:
            hex_mode(line, dev);
            break;
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