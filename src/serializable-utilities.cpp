// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <types.h>
#include <serializable-utilities.h>
#include <device.h>
#include "../third-party/json.hpp"

using json = nlohmann::json;

namespace librealsense {
namespace serializable_utilities {

std::string get_connected_device_info( librealsense::device_interface & device )
{
    json j;
    // Currently save only camera name
    auto camera_name = device.get_info( RS2_CAMERA_INFO_NAME );
    j["Camera Name"] = camera_name;
    return j.dump( 4 );
}

serialized_device_info get_pre_configured_device_info( const librealsense::device_interface& device, const std::string& json_content )
{
    serialized_device_info dev_info;
    dev_info.device_name = "Unknown";
    json j = json::parse(json_content);

    auto camera_name = device.get_info(RS2_CAMERA_INFO_NAME);

    auto it = j.find("Camera Name");
    if (it != j.end())
    {
        // Do not allow loading a file saved with a different device type
        if (*it != camera_name)
            throw librealsense::invalid_value_exception("pre-configured device("+ it.value().get<std::string>() + ") does not match connected device(" + camera_name+ ")");

        dev_info.device_name = it.value().get<std::string>();
    }

    return dev_info;
}

}  // namespace serializable_utilities
}  // namespace librealsense