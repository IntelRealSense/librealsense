// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"
#include <fstream>

TEST_CASE("ROLLING C++ LOGGER", "[log]") {

    std::string log_filename = "rolling-log.log";
    rs2::log_to_file(RS2_LOG_SEVERITY_INFO, log_filename.c_str());

    int max_size = 1;
    rs2::enable_rolling_log_file(max_size);

    for (int i = 0; i < 15000; ++i)
    {
        const std::string msg("debug message ");
        REQUIRE_NOTHROW(rs2::log(RS2_LOG_SEVERITY_INFO, (msg + std::to_string(i)).c_str()));
    }

    rs2::reset_logger();

    std::ifstream log_file(log_filename.c_str(), std::ios::binary | std::ios::in);
    REQUIRE(log_file.good());
    log_file.seekg(0, std::ios::end);
    auto log_size = log_file.tellg();

    std::string old_filename = log_filename + ".old";
    std::ifstream old_file(old_filename.c_str(), std::ios::binary | std::ios::in);
    REQUIRE(old_file.good());
    old_file.seekg(0, std::ios::end);
    auto old_size = old_file.tellg();
    auto max_size_in_bytes = max_size * 1024 * 1024;
    auto size = log_size + old_size;
    REQUIRE(size <= 2 * max_size_in_bytes);

}
