// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/device-info-msg.h>

#include <librealsense2/utilities/json.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>

namespace realdds {
namespace topics {


/* static  */ device_info device_info::from_json( nlohmann::json const & j )
{
    device_info ret;

    ret.name         = utilities::json::get< std::string >( j, "name" );
    ret.serial       = utilities::json::get< std::string >( j, "serial" );
    ret.product_line = utilities::json::get< std::string >( j, "product-line" );
    ret.product_id   = utilities::json::get< std::string >( j, "product-id" );
    ret.topic_root   = utilities::json::get< std::string >( j, "topic-root" );
    ret.locked       = utilities::json::get< bool >( j, "locked" );

    return ret;
}

nlohmann::json device_info::to_json() const
{
    auto msg = nlohmann::json( {
        { "name", name },
        { "serial", serial },
        { "product-line", product_line },
        { "product-id", product_id },
        { "topic-root", topic_root },
        { "locked", locked },
    } );
    LOG_DEBUG( "-----> JSON = " << msg.dump() );
    return msg;
}

}  // namespace topics
}  // namespace realdds
