// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "librealsense2/rs.hpp"
#include "librealsense2/hpp/rs_internal.hpp"
#include <fstream>
#include <thread>
#include "tclap/CmdLine.h"



using namespace std;
using namespace TCLAP;
using namespace rs2;

string char2hex(unsigned char n)
{
    string res;

    do
    {
        res += "0123456789ABCDEF"[n % 16];
        n >>= 4;
    } while (n);

    reverse(res.begin(), res.end());

    if (res.size() == 1)
    {
        res.insert(0, "0");
    }

    return res;
}

string datetime_string()
{
    auto t = time(nullptr);
    char buffer[20] = {};
    const tm* time = localtime(&t);
    if (nullptr != time)
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time);

    return string(buffer);
}

int main(int argc, char* argv[])
{
    CmdLine cmd("librealsense rs-fw-logger example tool", ' ', RS2_API_VERSION_STR);
    ValueArg<string> xml_arg("l", "load", "Full file path of HW Logger Events XML file", false, "", "Load HW Logger Events XML file");
    SwitchArg flash_logs_arg("f", "flash", "Flash Logs Request", false);
    cmd.add(xml_arg); 
    cmd.add(flash_logs_arg);
    cmd.parse(argc, argv);

    log_to_file(RS2_LOG_SEVERITY_WARN, "librealsense.log");

    auto use_xml_file = false;
    auto xml_full_file_path = xml_arg.getValue();

    bool are_flash_logs_requested = flash_logs_arg.isSet();

    context ctx;
    device_hub hub(ctx);

    bool should_loop_end = false;

    while (!should_loop_end)
    {
        try
        {
            cout << "\nWaiting for RealSense device to connect...\n";
            auto dev = hub.wait_for_device();
            cout << "RealSense device was connected...\n";

            setvbuf(stdout, NULL, _IONBF, 0); // unbuffering stdout

            auto fw_log_device = dev.as<rs2::firmware_logger>();

            bool using_parser = false;
            if (!xml_full_file_path.empty())
            {
                ifstream f(xml_full_file_path);
                if (f.good())
                {
                    std::string xml_content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    bool parser_initialized = fw_log_device.init_parser(xml_content);
                    if (parser_initialized)
                        using_parser = true;
                }
            }

            bool are_there_remaining_flash_logs_to_pull = true;

            while (hub.is_connected(dev))
            {
                if (are_flash_logs_requested && !are_there_remaining_flash_logs_to_pull)
                {
                    should_loop_end = true;
                    break;
                }
                auto log_message = fw_log_device.create_message();
                bool result = false;
                if (are_flash_logs_requested)
                {
                    result = fw_log_device.get_flash_log(log_message);
                }
                else
                {
                    result = fw_log_device.get_firmware_log(log_message);
                }
                if (result)
                {
                    std::vector<string> fw_log_lines;
                    if (using_parser)
                    {
                        auto parsed_log = fw_log_device.create_parsed_message();
                        bool parsing_result = fw_log_device.parse_log(log_message, parsed_log);
                        
                        stringstream sstr;
                        sstr << parsed_log.timestamp() << " " << parsed_log.severity() << " " << parsed_log.message()
                            << " " << parsed_log.thread_name() << " " << parsed_log.file_name()
                            << " " << parsed_log.line();

                        fw_log_lines.push_back(sstr.str());
                    }
                    else
                    {
                        stringstream sstr;
                        sstr << datetime_string() << "  FW_Log_Data:";
                        std::vector<uint8_t> msg_data = log_message.data();
                        for (int i = 0; i < msg_data.size(); ++i)
                        {
                        sstr << char2hex(msg_data[i]) << " ";
                        }
                        fw_log_lines.push_back(sstr.str());
                    }
                    for (auto& line : fw_log_lines)
                        cout << line << endl;
                }
                else
                {
                    if (are_flash_logs_requested)
                    {
                        are_there_remaining_flash_logs_to_pull = false;
                    }
                }
            }
        }
        catch (const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
    }

    return EXIT_SUCCESS;
}
