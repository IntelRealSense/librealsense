// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "types.h"
#include "aus.h"

#include <fstream>

#ifdef BUILD_AUS

namespace librealsense
{
    static aus_data aus_data_obj;
}

void librealsense::aus_set(std::string counter, int value)
{
    aus_data_obj.set(counter, value);
}

void librealsense::aus_increment(std::string counter)
{
    aus_data_obj.increment(counter);
}


std::string librealsense::aus_build_system_timer_name(std::string suffix, std::string device_name)
{
    std::string result = "RS2_AUS_";
    if (device_name != "")
    {
        result += device_name + "_";
    }
    result += suffix + "_TIMER";
    std::replace(result.begin(), result.end(), ' ', '_');
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string librealsense::aus_build_system_counter_name(std::string suffix, std::string device_name)
{
    std::string result = "RS2_AUS_";
    if (device_name != "")
    {
        result += device_name + "_";
    }
    result += suffix + "_COUNTER";
    std::replace(result.begin(), result.end(), ' ', '_');
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

void librealsense::aus_start(std::string timer)
{
    aus_data_obj.start(timer);
}

void librealsense::aus_stop(std::string timer)
{
    aus_data_obj.stop(timer);
}

long librealsense::aus_get(std::string counter)
{
    return aus_data_obj.get(counter);
}


std::vector<std::string> librealsense::aus_get_counters_list()
{
   return aus_data_obj.get_counters_list();
}


#else // BUILD_AUS


void librealsense::aus_set(std::string counter, int value)
{
    throw std::runtime_error("aus_print_stats is not supported without BUILD_AUS");
}

void librealsense::aus_increment(std::string counter)
{
    throw std::runtime_error("aus_increment is not supported without BUILD_AUS");
}

void librealsense::aus_start(std::string timer)
{
    throw std::runtime_error("aus_start is not supported without BUILD_AUS");
}

void librealsense::aus_stop(std::string timer)
{
    throw std::runtime_error("aus_stop is not supported without BUILD_AUS");
}

std::string librealsense::aus_build_system_timer_name(std::string suffix, std::string device_name)
{
    throw std::runtime_error("aus_build_system_timer_name is not supported without BUILD_AUS");
}

std::string librealsense::aus_build_system_counter_name(std::string suffix, std::string device_name)
{
    throw std::runtime_error("aus_build_system_counter_name is not supported without BUILD_AUS");
}

long librealsense::aus_get(std::string counter)
{
    throw std::runtime_error("aus_get is not supported without BUILD_AUS");
}


std::vector<std::string> librealsense::aus_get_counters_list()
{
    throw std::runtime_error("get_counters_list is not supported without BUILD_AUS");
}


#endif // BUILD_AUS

