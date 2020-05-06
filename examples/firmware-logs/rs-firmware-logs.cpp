// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <thread>
#include <string>
#include <vector>

using namespace std;


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

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[])
{
    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    // Start streaming with default recommended configuration
    // The default video configuration contains Depth and Color streams
    // If a device is capable to stream IMU data, both Gyro and Accelerometer are enabled by default
    auto res = pipe.start();

    rs2::context ctx;
    rs2::device_hub hub(ctx);
    while (true)
    {
        try
        {
            cout << "\nWaiting for RealSense device to connect...\n";
            auto dev = hub.wait_for_device();
            cout << "RealSense device was connected...\n";

            setvbuf(stdout, NULL, _IONBF, 0); // unbuffering stdout

            while (hub.is_connected(dev))
            {
                this_thread::sleep_for(chrono::milliseconds(100));

                auto fw_log = res.get_device().as<rs2::firmware_logger>();
                std::vector<uint8_t> raw_data = fw_log.get_firmware_logs();
                vector<string> fw_log_lines = { "" };
                if (raw_data.size() <= 4)
                    continue;

                /*if (use_xml_file)
                {
                    fw_logs_binary_data fw_logs_binary_data = { raw_data };
                    fw_logs_binary_data.logs_buffer.erase(fw_logs_binary_data.logs_buffer.begin(), fw_logs_binary_data.logs_buffer.begin() + 4);
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
                }*/

                stringstream sstr;
                sstr << datetime_string() << "  FW_Log_Data:";
                for (size_t i = 0; i < raw_data.size(); ++i)
                    sstr << hexify(raw_data[i]) << " ";

                fw_log_lines.push_back(sstr.str());

                for (auto& line : fw_log_lines)
                    cout << line << endl;
            }
        }
        catch (const rs2::error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
    }

    return EXIT_SUCCESS;
}
