// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"
#include<fstream>

TEST_CASE("ENABLE ROLLING C++ LOGGER", "[log]") {

    std::string log_filename = "rolling-log.log";
    rs2::log_to_file(RS2_LOG_SEVERITY_INFO, log_filename.c_str());

    int max_size = 1024;
    rs2::enable_rolling_files(max_size);

    for (int i = 0; i < 100; ++i)
        log_all();

    std::ifstream log_file(log_filename.c_str(), std::ios::binary);
    log_file.seekg(0, std::ios::end);
    int log_size = log_file.tellg();

    std::string old_filename = log_filename + ".old";
    std::ifstream old_file(old_filename.c_str(), std::ios::binary);
    old_file.seekg(0, std::ios::end);
    int old_size = old_file.tellg();

    int size = log_size + old_size;
    REQUIRE(size <= max_size);

}
