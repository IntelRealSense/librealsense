// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <nlohmann/json.hpp>

#include <rsutils/string/slice.h>

namespace realdds {
namespace topics {

class device_info
{
    nlohmann::json _json;

public:
    //std::string serial;
    //std::string product_line;
    //bool locked = true;

    std::string const & name() const;
    void set_name( std::string const & );

    std::string const & topic_root() const;
    void set_topic_root( std::string const & );

    std::string const & serial_number() const;
    void set_serial_number( std::string const & );

    nlohmann::json const & to_json() const;
    static device_info from_json( nlohmann::json const & );

    // Substring of information already stored in the device-info that can be used to print the device 'name'.
    // (mostly for use with debug messages)
    rsutils::string::slice debug_name() const;
};


}  // namespace topics
}  // namespace realdds
