// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_internal.hpp>
#include <utilities/string/string-utilities.h>
#include <fstream>
#include <thread>
#include "tclap/CmdLine.h"


using namespace std;
using namespace TCLAP;
using namespace rs2;


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
    int default_polling_interval_ms = 100;
    CmdLine cmd("librealsense rs-fw-logger example tool", ' ', RS2_API_VERSION_STR);
    ValueArg<string> xml_arg("l", "load", "Full file path of HW Logger Events XML file", false, "", "Load HW Logger Events XML file");
    ValueArg<string> out_arg("o", "out", "Full file path of output file", false, "", "Print Fw logs to output file");
    ValueArg<int> polling_interval_arg("p", "polling_interval", "Time Interval between each log messages polling (in milliseconds)", false, default_polling_interval_ms, "");
    SwitchArg flash_logs_arg("f", "flash", "Flash Logs Request", false);
    cmd.add(xml_arg);
    cmd.add(out_arg);
    cmd.add(polling_interval_arg);
    cmd.add(flash_logs_arg);
    cmd.parse(argc, argv);

    log_to_file(RS2_LOG_SEVERITY_WARN, "librealsense.log");

    auto use_xml_file = false;
    auto output_file_path = out_arg.getValue();
    std::ofstream output_file(output_file_path);
    // write to file if it is open, else write to console
    std::ostream& out = (!output_file.is_open() ? std::cout : output_file);

    auto xml_full_file_path = xml_arg.getValue();
    auto polling_interval_ms = polling_interval_arg.getValue();
    if (polling_interval_ms < 25 || polling_interval_ms > 300)
    {
        out << "Polling interval time provided: " << polling_interval_ms << "ms, is not in the valid range [25,300]. Default value " << default_polling_interval_ms << "ms is used." << std::endl;
        polling_interval_ms = default_polling_interval_ms;
    }

    bool are_flash_logs_requested = flash_logs_arg.isSet();

    context ctx;
    device_hub hub(ctx);

    bool should_loop_end = false;

    while (!should_loop_end)
    {
        try
        {
            out << "\nWaiting for RealSense device to connect...\n";
            auto dev = hub.wait_for_device();
            out << "RealSense device was connected...\n";

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
            auto time_of_previous_polling_ms = std::chrono::high_resolution_clock::now();

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
                        sstr << datetime_string() << " " << parsed_log.timestamp() << " " << parsed_log.sequence_id()
                            << " " << parsed_log.severity() << " " << parsed_log.thread_name()
                            << " " << parsed_log.file_name() << " " << parsed_log.line()
                            << " " << parsed_log.message();
                        
                        fw_log_lines.push_back(sstr.str());
                    }
                    else
                    {
                        stringstream sstr;
                        sstr << datetime_string() << "  FW_Log_Data:";
                        std::vector<uint8_t> msg_data = log_message.data();
                        for (int i = 0; i < msg_data.size(); ++i)
                        {
                        sstr << utilities::string::hexify(msg_data[i]) << " ";
                        }
                        fw_log_lines.push_back(sstr.str());
                    }
                    for (auto& line : fw_log_lines)
                        out << line << endl;
                }
                else
                {
                    if (are_flash_logs_requested)
                    {
                        are_there_remaining_flash_logs_to_pull = false;
                    }
                }
                auto num_of_messages = fw_log_device.get_number_of_fw_logs();
                if (num_of_messages == 0)
                {
                    auto current_time = std::chrono::high_resolution_clock::now();
                    auto time_since_previous_polling_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - time_of_previous_polling_ms).count();
                    if (time_since_previous_polling_ms < polling_interval_ms)
                    {
                        std::this_thread::sleep_for(chrono::milliseconds(polling_interval_ms - time_since_previous_polling_ms));
                    }
                    time_of_previous_polling_ms = std::chrono::high_resolution_clock::now();
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
