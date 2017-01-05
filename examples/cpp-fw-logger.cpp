// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <fstream>
#include <thread>
#include "tclap/CmdLine.h"
#include "cpp-fw-logger-helpers/fw-logs-parser.h"


using namespace std;
using namespace TCLAP;

std::string hexify(unsigned char n)
{
    std::string res;

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
    if (nullptr != &time)
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time);

    return string(buffer);
}

int main(int argc, char* argv[]) try
{
    CmdLine cmd("librealsense cpp-fw-logger example tool", ' ', RS_API_VERSION_STR);
    ValueArg<string> xml_arg("l", "load", "Full file path of HW Logger Events XML file", false, "", "Load HW Logger Events XML file");
    cmd.add(xml_arg);
    cmd.parse(argc, argv);

    rs::log_to_file(RS_LOG_SEVERITY_WARN, "librealsense.log");
    // Obtain a list of devices currently present on the system
    rs::context ctx;
    auto devices = ctx.query_devices();
    unsigned device_count = devices.size();
    if (!device_count)
    {
        printf("No device detected. Is it plugged in?\n");
        return EXIT_FAILURE;
    }

    rs::device dev = devices.front();
    auto str_op_code = dev.get_camera_info(RS_CAMERA_INFO_DEVICE_DEBUG_OP_CODE);
    auto op_code = static_cast<uint8_t>(stoi(str_op_code));
    std::vector<uint8_t> input = {0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
                                  0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00};

    unique_ptr<FWLogsParser> fw_logs_parser;
    auto use_xml_file = false;
    auto xml_full_file_path = xml_arg.getValue();
    if (!xml_full_file_path.empty())
    {
        std::ifstream f(xml_full_file_path);
        if (f.good())
        {
            fw_logs_parser = unique_ptr<FWLogsParser>(new FWLogsParser(xml_full_file_path));
            use_xml_file = true;
        }
    }

    cout << "Device Name: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << endl <<
            "Device Location: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_LOCATION) << endl << endl;

    setvbuf(stdout, NULL, _IONBF, 0); // unbuffering stdout

    std::thread logger([&](){
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto raw_data = dev.debug().send_and_receive_raw_data(input);
            vector<string> fw_log_lines = {""};
            if (raw_data.size() <= 4)
                continue;

            if (use_xml_file)
            {
                FWlogsBinaryData fw_logs_binary_data = {raw_data};
                fw_logs_binary_data.logsBuffer.erase(fw_logs_binary_data.logsBuffer.begin(),fw_logs_binary_data.logsBuffer.begin()+4);
                fw_log_lines = fw_logs_parser->GetFWlogLines(fw_logs_binary_data);
                for (auto & elem : fw_log_lines)
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
                std::cout << line << endl;
        }
    });
    logger.detach();


    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
