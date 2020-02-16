// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <string>
#include <thread>

class android_fw_logger
{
public:
    android_fw_logger(std::string xml_path = "", int sample_rate = 100);
    ~android_fw_logger();

private:
    bool _active = false;
    std::thread _thread;
    std::string _xml_path;

    void read_log_loop();
    int _sample_rate;
};
