// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#test:donotrun      # temporary: currently it fails LibCI on Linux (see LRS-180)!

#include "log-common.h"
#include <atomic>

// This is usually enabled by CMake only in Linux
#ifdef EASYLOGGINGPP_ASYNC

std::atomic<int> atomic_integer(0);
std::chrono::milliseconds max_time = (std::chrono::milliseconds)0;
std::chrono::milliseconds min_time = (std::chrono::milliseconds)50;
std::chrono::milliseconds avg_time = (std::chrono::milliseconds)0;
int max_time_iteration = -1;
const int number_of_iterations = 10000;
const int number_of_threads = 10;
std::mutex stats_mutex;
//thresholds
const int checked_log_write_time = 10; //ms
const int required_log_write_time = 50; //ms
const int checked_lag_time = 15; //ms
const int required_lag_time = 50; //ms
//----------------------- HELPER METHODS START -------------------------
// input line example:
//  14/01 16:53:19,866 DEBUG [884] (rs.cpp:2579) atomic integer = 7
int get_time_from_line(std::string line)
{
    auto time_word = line.substr(7, 12);
    auto hours = stoi(time_word.substr(0, 2));
    auto minutes = stoi(time_word.substr(3, 5));
    auto seconds = stoi(time_word.substr(6, 8));
    auto ms = stoi(time_word.substr(9, 12));
    //std::cout << "h:m:s:ms = " << hours << ":" << minutes << ":" << seconds << ":" << ms << std::endl;
    return (ms + seconds * 1000 + minutes * 1000 * 60 + hours * 1000 * 60 * 60);
}


std::string get_last_line(std::ifstream& in)
{
    std::string line;
    for (int i = 0; i < (number_of_iterations * number_of_threads) - 1; ++i)
    {
        getline(in, line);
    }
    return line;
}

// inputs are use as 2 points that define a line:
// first_time_ms, first_value : x1, y1
// second_time_ms, second_value : x2, y2
// value_in_log : y3
// return value will be x3
int get_log_ideal_time(int first_time_ms, int first_value, int second_time_ms, int second_value, int value_in_log)
{
    int ideal_time = value_in_log * (second_time_ms - first_time_ms) / (second_value - first_value) +
        (first_time_ms * (second_value / (second_value - first_value))) - second_time_ms * (first_value / (second_value - first_value));
    
    return ideal_time;
}

std::pair<unsigned int, int> get_next_50_lines_time_val(std::ifstream& ifs)
{
    unsigned int avg_time_ms = -1;
    int avg_value = -1;
    for (int i = 0; i < 50; ++i) {
        std::string line;
        getline(ifs, line);
        avg_time_ms += get_time_from_line(line);
        avg_value += stoi(line.substr(line.find_last_of(" ") + 1));
    }
    avg_time_ms /= 50;
    avg_value /= 50;
    return std::make_pair(avg_time_ms, avg_value);
}

//----------------------- HELPER METHODS END ---------------------------

TEST_CASE("async logger", "[log][async_log]")
{
    size_t n_callbacks = 0;
    auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
    {
        ++n_callbacks;
        std::cout << msg.raw();
    };
   
    unsigned n_iter = 0; 
    auto func = [&n_iter](int required_value) {
        int iterations = 0;
        
        while (iterations < number_of_iterations)
        {
            std::ostringstream ss;
            ss << "atomic integer = " << ++atomic_integer;
            auto start_time = std::chrono::high_resolution_clock::now();
            rs2::log(RS2_LOG_SEVERITY_DEBUG, ss.str().c_str());
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds delta_ms = std::chrono::duration_cast< std::chrono::milliseconds >( end_time - start_time );
            {
                std::lock_guard<std::mutex> lock(stats_mutex);
                if (delta_ms > max_time) max_time = delta_ms;
                if (delta_ms < min_time) min_time = delta_ms;
                avg_time += delta_ms;
                ++n_iter;
            }
            ++iterations;
        }
    };
    const char* log_file_path = ".//multi-thread-logs.log";
    try 
    {
        remove(log_file_path);
    }
    catch (...) {}
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, log_file_path);

    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_threads; ++i)
    {
        threads.push_back(std::thread(func, i + 1));
    }
    
    for (auto&&t : threads)
    {
        if (t.joinable()) t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    std::ostringstream ss;
    CAPTURE( max_time.count() );
    CAPTURE( min_time.count() );
    CAPTURE( n_iter );
    CAPTURE( avg_time.count() / float( n_iter ));
    // The threshold in the CHECK_NOFAIL statement should be filled when running in "normal PC"
    // The threshold in the REQUIRE statement should be filled when running CI tests (less cores are available there)
    CHECK_NOFAIL(max_time.count() < checked_log_write_time);
    REQUIRE(max_time.count() < required_log_write_time);
    
    // checking logs correctness - that all the integer's values have been logged
    {
        // preparing array of number of iterations * number of threads
        std::array<char, number_of_iterations * number_of_threads> check_table;
        // setting all cells to 0
        memset(&check_table[0], 0, check_table.size());
        // marking cells at the position of each log integer value to 1
        std::ifstream check_file(log_file_path, std::ios::in);
        if (check_file.good())
        {
            std::string line;
            while (std::getline(check_file, line)) 
            {
                auto value_in_log = stoi(line.substr(line.find_last_of(" ")));
                REQUIRE(value_in_log > 0);
                REQUIRE((value_in_log - 1) < (number_of_iterations* number_of_threads));
                check_table.at(value_in_log - 1) = 1;
            }
            // checking that all cells have value 1, which means that all logs have been sent and received
            std::for_each(check_table.cbegin(), check_table.cend(), [](const char c) {
                REQUIRE(c == 1);
                });
            check_file.close();
        }
    }

    // checking logs order in means of logging time
    {
        std::ifstream check_file(log_file_path, std::ios::in);
        int overall_ms = 0;
        int overall_logs = 0;
        if (check_file.good())
        {
            // get data to calculate "ideal" line
            // using average from 50 first logs as one point,
            // and average of 50 last points as second point
            auto first_50_pair = get_next_50_lines_time_val(check_file);
            unsigned int first_50_avg_time_ms = first_50_pair.first;
            int first_50_avg_value = first_50_pair.second;
            
            std::string line;
            // the aim of this loop is to get the file's cursor before the 50 last lines, 
            // considering it is already after the 50 first lines
            for (int i = 0; i < (number_of_iterations * number_of_threads) - 1 - 50 - 50; ++i)
                getline(check_file, line);
            
            auto last_50_pair = get_next_50_lines_time_val(check_file);
            unsigned int last_50_avg_time_ms = last_50_pair.first;
            int last_50_avg_value = last_50_pair.second;

            // clear and return to start of the file
            check_file.clear();
            check_file.seekg(0);

            // go over each line and check that its logging time is not "far away" in means of time than the "ideal" time
            int max_delta = 0;
            while (getline(check_file, line)) 
            {
                auto value_in_log = stoi(line.substr(line.find_last_of(" ")));
                auto log_time_ms = get_time_from_line(line);
                auto log_ideal_time_ms = get_log_ideal_time(first_50_avg_time_ms, first_50_avg_value, last_50_avg_time_ms, last_50_avg_value, value_in_log);
                auto delta = std::abs(log_ideal_time_ms - log_time_ms);
                if (delta > max_delta) max_delta = delta;
            }
            // The threshold in the CHECK_NOFAIL statement should be filled when running in "normal PC"
            // The threshold in the REQUIRE statement should be filled when running CI tests (less cores are available there)
            CHECK_NOFAIL(max_delta < checked_lag_time);
            REQUIRE(max_delta < required_lag_time);
        }
        check_file.close();
    }

    // checking number of logs throughput per ms
    {
        std::ifstream check_file(log_file_path, std::ios::in);
        int overall_ms = 0;
        int overall_logs = 0;
        if (check_file.good())
        {
            std::string line;
            while (std::getline(check_file, line)) 
            {
                auto previous_time = get_time_from_line(line);
                int logs_counter_in_one_ms = 1;
                while (std::getline(check_file, line))
                {
                    auto current_time = get_time_from_line(line);
                    if (current_time != previous_time)
                    {
                        overall_logs += logs_counter_in_one_ms;
                        ++overall_ms;
                        break;
                    }
                    else
                        ++logs_counter_in_one_ms;
                }                
            }
        }
        check_file.close();
        float average_logs_in_one_ms = (float)overall_logs / overall_ms;
        REQUIRE(average_logs_in_one_ms >= 20.0F);
    }    
}
#endif


