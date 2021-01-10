// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"
#include <atomic>

std::atomic<int> atomic_integer(0);
std::chrono::milliseconds global_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
std::chrono::milliseconds max_time = (std::chrono::milliseconds)0;
std::chrono::milliseconds min_time = (std::chrono::milliseconds)50;
std::chrono::milliseconds avg_time = (std::chrono::milliseconds)0;
int max_time_iteration = -1;
int number_of_iterations = 10000;

#ifdef ELPP_EXPERIMENTAL_ASYNC
TEST_CASE("async logger", "[log][async_log]")
{
    size_t n_callbacks = 0;
    auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
    {
        ++n_callbacks;
        std::cout << msg.raw();
    };
    
    auto func = [](int required_value) {
        int iterations = 0;
        auto start = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(20))
        {
            std::stringstream ss;
            int value_to_check = (required_value) + 10 * iterations;
            ss << "atomic integer = " << ++atomic_integer << " and required value = " << value_to_check;
            auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            rs2::log(RS2_LOG_SEVERITY_DEBUG, ss.str().c_str());
            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            std::chrono::milliseconds delta_ms = ms - start_ms;
            if (delta_ms > max_time) {
                max_time = delta_ms; max_time_iteration = value_to_check;
            }
            if (delta_ms < min_time) min_time = delta_ms;
            avg_time += delta_ms;
            start_ms = ms;
            ++iterations;
            /*bool value_result = (atomic_integer <= value_to_check + 3) && (atomic_integer >= value_to_check - 3);
            if (!value_result)
                int a = 1;*/
            /*bool performance_result = (delta_ms < (std::chrono::milliseconds)20);
            if (!performance_result)
                int a = 1;*/
            //std::cout << " , delta_ms = " << delta_ms.count() << std::endl;
        }
    };
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG);
    //rs2::log_to_callback(RS2_LOG_SEVERITY_DEBUG, callback);
    global_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i)
    {
        threads.push_back(std::thread(func, i + 1));
    }
    
    for (auto&&t : threads)
    {
        if (t.joinable()) t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    std::cout << "max time = " << max_time.count() << " ms, on iteration: " << max_time_iteration << std::endl;
    std::cout << "min time = " << min_time.count() << " ms" << std::endl;
    std::cout << "avg time = " << (float) (avg_time.count()) / (number_of_iterations * 10.f) << " ms" << std::endl;
    REQUIRE(max_time.count() < 50);
}
#endif


