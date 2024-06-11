// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json.h>
#include <rsutils/string/slice.h>

namespace realdds {
namespace topics {

class device_info
{
    rsutils::json _json;

public:
    std::string const & name() const;
    void set_name( std::string const & );

    std::string const & topic_root() const;
    void set_topic_root( std::string const & );

    std::string const & serial_number() const;
    void set_serial_number( std::string const & );

    rsutils::json const & to_json() const;
    static device_info from_json( rsutils::json const & );

    // Substring of information already stored in the device-info that can be used to print the device 'name'.
    // (mostly for use with debug messages)
    rsutils::string::slice debug_name() const;

    // Common names (that would otherwise go into dds-topic-names.h)
public:
    struct key {
        static std::string const name;
        static std::string const topic_root;
        static std::string const serial;
        static std::string const recovery;
        static std::string const stopping;
    };
};


}  // namespace topics
}  // namespace realdds
