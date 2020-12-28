// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"
#include <time.h>

std::atomic_int atomic_integer = 0;
std::chrono::milliseconds global_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
time_t my_time = time(NULL);

TEST_CASE("async logger", "[log][remi]")
{
    size_t n_callbacks = 0;
    auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
    {
        ++n_callbacks;
        //std::cout << msg.raw();
    };

    auto func = [](int required_value) {
        int iterations = 0;
        while (iterations < 10)
        {
            std::stringstream ss;
            int value_to_check = (required_value) + 10 * iterations;
            ss << "atomic integer = " << ++atomic_integer << ", and required value = " << value_to_check;
            rs2::log(RS2_LOG_SEVERITY_DEBUG, ss.str().c_str());
            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            /*bool value_result = (atomic_integer <= value_to_check + 3) && (atomic_integer >= value_to_check - 3);
            if (!value_result)
                int a = 1;*/
            std::chrono::milliseconds delta_ms = ms - global_ms;
            bool performance_result = (delta_ms < (std::chrono::milliseconds)20);
            if (!performance_result)
                int a = 1;
            std::cout << " , delta_ms = " << delta_ms.count() << std::endl;
            global_ms = ms;
            ++iterations;
        }
    };
    rs2::log_to_callback(RS2_LOG_SEVERITY_DEBUG, callback);

    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i)
    {
        threads.push_back(std::thread(func, i + 1));
    }
    
    for (auto&&t : threads)
    {
        if (t.joinable()) t.join();
    }
}
