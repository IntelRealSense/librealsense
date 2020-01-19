// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_INTERNALTESTS_COMMON_H
#define LIBREALSENSE_INTERNALTESTS_COMMON_H

#include "catch/catch.hpp"

#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

#include "../project-common.h"

#include "../src/context.h"
#include "../src/sensor.h"
#include "../src/l500/l500-factory.h"
#include "../src/ds5/ds5-factory.h"
#include "../src/ivcam/sr300.h"

using namespace librealsense;
using namespace librealsense::platform;

inline std::string get_device_pid(const librealsense::device& device)
{
    return device.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
}

inline std::string get_sensor_name(const librealsense::sensor_interface& sensor)
{
    return sensor.get_info(RS2_CAMERA_INFO_NAME);
}

inline void resolve_records_arguments(librealsense::backend_type* backend_type, std::string* filename)
{
    auto argc = command_line_params::instance().get_argc();
    auto argv = command_line_params::instance().get_argv();

    std::string base_filename;
    for (auto i = 0u; i < argc; i++)
    {
        std::string param(argv[i]);
        if (param == "into")
        {
            i++;
            if (i < argc)
            {
                base_filename = argv[i];
                *backend_type = backend_type::record;
                return;
            }
        }
        else if (param == "from")
        {
            i++;
            if (i < argc)
            {
                base_filename = argv[i];
                *backend_type = backend_type::playback;
                return;
            }
        }
    }
    *backend_type = backend_type::standard;
}

inline void create_context(std::shared_ptr<librealsense::context>& ctx, librealsense::backend_type backend_type, std::string base_filename, std::string section, std::string min_api_version)
{
    switch (backend_type)
    {
    case backend_type::record:
        ctx = std::make_shared<librealsense::context>(librealsense::backend_type::record, base_filename.c_str(), section.c_str(), RS2_RECORDING_MODE_BLANK_FRAMES);
        break;
    case backend_type::playback:
        ctx = std::make_shared<librealsense::context>(librealsense::backend_type::playback, base_filename.c_str(), section.c_str(), RS2_RECORDING_MODE_COUNT, min_api_version);
        break;
    case backend_type::standard:
        ctx = std::make_shared<librealsense::context>(librealsense::backend_type::standard);
        break;
    }
}

inline std::shared_ptr<librealsense::context> make_lrs_context(const char* id, std::string min_api_version = "0.0.0")
{
    static std::map<std::string, int> _counters;

    _counters[id]++;

    librealsense::backend_type backendtype;
    std::string base_filename;
    resolve_records_arguments(&backendtype, &base_filename);

    std::stringstream ss;
    ss << id << "." << _counters[id] << ".test";
    auto section = ss.str();

    std::shared_ptr<librealsense::context> ctx;
    try
    {
        create_context(ctx, backendtype, base_filename, section, min_api_version);
        command_line_params::instance()._found_any_section = true;
    }
    catch (...)
    {
        return nullptr;
    }

    return ctx;
}

inline std::vector<std::shared_ptr<librealsense::device>> create_lrs_devices()
{
    auto lrsctx = make_lrs_context(SECTION_FROM_TEST_NAME);
    auto&& be = lrsctx->get_backend();

    std::shared_ptr<device_info> di;
    platform::backend_device_group devices(be.query_uvc_devices(), be.query_usb_devices(), be.query_hid_devices());

    auto l500_i = l500_info::pick_l500_devices(lrsctx, devices);
    auto ds5_i = ds5_info::pick_ds5_devices(lrsctx, devices);
    auto sr300_i = sr300_info::pick_sr300_devices(lrsctx, be.query_uvc_devices(), be.query_usb_devices());

    std::string error_msg = "Could not create any device. Check if the device is properly connected.";
    CAPTURE(error_msg);
    REQUIRE(l500_i.size() + ds5_i.size() + sr300_i.size() > 0);

    std::vector<std::shared_ptr<librealsense::device>> lrs_devices;

    if (!l500_i.empty())
    {
        std::shared_ptr<librealsense::l500_info> l500_info = std::dynamic_pointer_cast<librealsense::l500_info>(l500_i.front());
        lrs_devices.push_back(std::dynamic_pointer_cast<device>(l500_info->create(lrsctx, false)));
    }
    if (!ds5_i.empty())
    {
        std::shared_ptr<librealsense::ds5_info> ds5_info = std::dynamic_pointer_cast<librealsense::ds5_info>(ds5_i.front());
        lrs_devices.push_back(std::dynamic_pointer_cast<device>(ds5_info->create(lrsctx, false)));
    }
    if (!sr300_i.empty())
    {
        std::shared_ptr<librealsense::sr300_info> sr300_info = std::dynamic_pointer_cast<librealsense::sr300_info>(sr300_i.front());
        lrs_devices.push_back(std::dynamic_pointer_cast<device>(sr300_info->create(lrsctx, false)));
    }

    return lrs_devices;
}

inline std::vector<std::shared_ptr<stream_profile_interface>> retrieve_profiles(const librealsense::synthetic_sensor& sensor, const std::vector<librealsense::stream_profile>& profiles)
{
    std::vector<std::shared_ptr<stream_profile_interface>> res;
    auto&& supported_profiles = sensor.get_stream_profiles();
    for (auto&& p : profiles)
    {
        auto profiles_are_equal = [&p](const std::shared_ptr<stream_profile_interface>& sp) { return to_profile(sp.get()) == p; };
        auto profile_it = std::find_if(supported_profiles.begin(), supported_profiles.end(), profiles_are_equal);
        if (profile_it != supported_profiles.end())
            res.push_back(*profile_it);
    }
    return res;
}

inline void compare_profiles(synthetic_sensor& sensor, std::pair<std::vector<librealsense::stream_profile>, std::vector<librealsense::stream_profile>> expected_resolved_pair)
{
    auto&& to_resolve = retrieve_profiles(sensor, expected_resolved_pair.first);
    // Sensor's open, adds missing profile data, we must call it before checking the resolve function.
    sensor.open(to_resolve);
    sensor.close();
    auto&& resolved_profiles = sensor.resolve_requests(to_resolve);
    auto&& expected_resolved_profiles = retrieve_profiles(sensor, expected_resolved_pair.second);

    // Verify that expected resolved profiles contain the actual resolved profiles
    require_first_contains_second(expected_resolved_profiles, resolved_profiles, "There were resolved profiles which didn't find a match with expected");

    // Verify that the actual resolved profiles contain the expected resolved profiles
    require_first_contains_second(resolved_profiles, expected_resolved_profiles, "There were expected resolved profiles which didn't find a match with the actual resolved ones");
};

#endif // LIBREALSENSE_INTERNALTESTS_COMMON_H
