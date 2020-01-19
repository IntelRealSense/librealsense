// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once
#ifndef LIBREALSENSE_PROJECTTESTS_COMMON_H
#define LIBREALSENSE_PROJECTTESTS_COMMON_H

#include "catch/catch.hpp"

#include <fstream>

typedef uint16_t PID_t;

const std::string L500_TAG = "L500";
const std::string L500_COLOR_SENSOR_NAME = "RGB Camera";
const std::string L500_DEPTH_SENSOR_NAME = "L500 Depth Sensor";
const std::string L500_MOTION_SENSOR_NAME = "Motion Module";

inline PID_t string_to_hex(std::string pid)
{
    std::stringstream str(pid);
    PID_t value = 0;
    str >> std::hex >> value;
    return value;
}

class command_line_params
{
public:
    command_line_params(int argc, char* const argv[])
    {
        for (auto i = 0; i < argc; i++)
        {
            _params.push_back(argv[i]);
        }
    }

    char* const* get_argv() const { return _params.data(); }
    char* get_argv(int i) const { return _params[i]; }
    size_t get_argc() const { return _params.size(); }

    static command_line_params& instance(int argc = 0, char* const argv[] = 0)
    {
        static command_line_params params(argc, argv);
        return params;
    }

    bool _found_any_section = false;
private:

    std::vector<char*> _params;
};

inline bool file_exists(const std::string& filename)
{
    std::ifstream f(filename);
    return f.good();
}

inline std::string space_to_underscore(const std::string& text) {
    const std::string from = " ";
    const std::string to = "__";
    auto temp = text;
    size_t start_pos = 0;
    while ((start_pos = temp.find(from, start_pos)) != std::string::npos) {
        temp.replace(start_pos, from.size(), to);
        start_pos += to.size();
    }
    return temp;
}

#define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()

template<class T>
void require_first_contains_second(std::vector<T> first, std::vector<T> second, std::string error_msg)
{
    for (auto&& f : first)
    {
        second.erase(std::remove(second.begin(), second.end(), f), second.end());
    }
    CAPTURE(error_msg);
    std::stringstream not_contained;
    for (auto&& s : second)
        not_contained << s << std::endl;
    CAPTURE(not_contained.str());
    REQUIRE(second.empty());
}
#endif // LIBREALSENSE_PROJECTTESTS_COMMON_H