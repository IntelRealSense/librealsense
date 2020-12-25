// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"


TEST_CASE("RESET C++ LOGGER", "[log]") {
    size_t n_callbacks = 0;
    auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
    {
        ++n_callbacks;
        TRACE(severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw());
    };

    rs2::log_to_callback(RS2_LOG_SEVERITY_INFO, callback);
    REQUIRE(!n_callbacks);
    rs2::reset_logger();
    log_all();
    REQUIRE(n_callbacks == 0);

    rs2::log_to_callback(RS2_LOG_SEVERITY_INFO, callback);
    REQUIRE(!n_callbacks);
    log_all();
    REQUIRE(n_callbacks == 3);

    rs2::reset_logger();
    log_all();
    REQUIRE(n_callbacks == 3);

    n_callbacks = 0;
    rs2::log_to_callback(RS2_LOG_SEVERITY_DEBUG, callback);
    REQUIRE(!n_callbacks);
    log_all();
    REQUIRE(n_callbacks == 4);

    n_callbacks = 0;
    rs2::reset_logger();
    log_all();
    REQUIRE(n_callbacks == 0);

}
