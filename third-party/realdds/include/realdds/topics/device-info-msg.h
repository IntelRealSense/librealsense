// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string>

namespace realdds {
namespace topics {

class device_info
{
public:
    std::string name;
    std::string serial;
    std::string product_line;
    std::string topic_root;
    bool locked = false;

    nlohmann::json to_json() const;
    static device_info from_json( nlohmann::json const & j );

    // Substring of information already stored in the device-info that can be used to print the devide 'name'.
    // (mostly for use with debug messages)
    char const * debug_name() const;
};


}  // namespace topics
}  // namespace realdds
