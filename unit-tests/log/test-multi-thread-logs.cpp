// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"

std::atomic_int atomic_integer = 0;

TEST_CASE("async logger", "[log][remi]")
{
    size_t n_callbacks = 0;
    auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
    {
        ++n_callbacks;
        TRACE(severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw());
    };

    auto func = []() {
        int iterations = 2;
        while (--iterations)
        {
            ++atomic_integer;
            std::stringstream ss;
            ss << "atomic integer = " << atomic_integer;
            rs2::log(RS2_LOG_SEVERITY_DEBUG, ss.str().c_str());
        }
    };
    rs2::log_to_callback(RS2_LOG_SEVERITY_DEBUG, callback);

    std::thread thread1(func);
    std::thread thread2(func);
    std::thread thread3(func);
    std::thread thread4(func);
    std::thread thread5(func);

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
    thread5.join();    
}
