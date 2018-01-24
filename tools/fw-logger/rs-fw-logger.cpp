// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <fstream>
#include <thread>
#include "tclap/CmdLine.h"
#include "fw-logs-parser.h"


using namespace std;
using namespace TCLAP;
using namespace fw_logger;
using namespace rs2;

string hexify(unsigned char n)
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
    cmd.add(xml_arg);
    cmd.parse(argc, argv);

    log_to_file(RS2_LOG_SEVERITY_WARN, "librealsense.log");
    // Obtain a list of devices currently present on the system

    unique_ptr<fw_logs_parser> fw_log_parser;
    auto use_xml_file = false;
    auto xml_full_file_path = xml_arg.getValue();
    if (!xml_full_file_path.empty())
    {
        ifstream f(xml_full_file_path);
        if (f.good())
        {
            fw_log_parser = unique_ptr<fw_logs_parser>(new fw_logs_parser(xml_full_file_path));
            use_xml_file = true;
        }
    }

    context ctx;
    device_hub hub(ctx);

    while (true)
    {
        try
        {
            cout << "\nWaiting for RealSense device to connect...\n";
            auto dev = hub.wait_for_device();
            cout << "RealSense device was connected...\n";

            vector<uint8_t> input;
            auto str_op_code = dev.get_info(RS2_CAMERA_INFO_DEBUG_OP_CODE);
            auto op_code = static_cast<uint8_t>(stoi(str_op_code));
            input = {0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
                     0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00};

            cout << "Device Name: " << dev.get_info(RS2_CAMERA_INFO_NAME) << endl <<
                    "Device Location: " << dev.get_info(RS2_CAMERA_INFO_PHYSICAL_PORT) << endl << endl;

            setvbuf(stdout, NULL, _IONBF, 0); // unbuffering stdout

            while (hub.is_connected(dev))
            {
                this_thread::sleep_for(chrono::milliseconds(100));

                auto raw_data = dev.as<debug_protocol>().send_and_receive_raw_data(input);
                vector<string> fw_log_lines = {""};
                if (raw_data.size() <= 4)
                    continue;

                if (use_xml_file)
                {
                    fw_logs_binary_data fw_logs_binary_data = {raw_data};
                    fw_logs_binary_data.logs_buffer.erase(fw_logs_binary_data.logs_buffer.begin(),fw_logs_binary_data.logs_buffer.begin()+4);
                    fw_log_lines = fw_log_parser->get_fw_log_lines(fw_logs_binary_data);
                    for (auto& elem : fw_log_lines)
                        elem = datetime_string() + "  " + elem;
                }
                else
                {
                    stringstream sstr;
                    sstr << datetime_string() << "  FW_Log_Data:";
                    for (size_t i = 0; i < raw_data.size(); ++i)
                        sstr << hexify(raw_data[i]) << " ";

                    fw_log_lines.push_back(sstr.str());
                }

                for (auto& line : fw_log_lines)
                    cout << line << endl;
            }
        }
        catch (const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
    }

    return EXIT_SUCCESS;
}
