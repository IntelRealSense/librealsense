// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/device-info-msg.h>

#include <rsutils/json.h>
#include <rsutils/easylogging/easyloggingpp.h>

namespace realdds {
namespace topics {


/* static  */ device_info device_info::from_json( nlohmann::json const & j )
{
    device_info ret;

    ret.name         = rsutils::json::get< std::string >( j, "name" );
    ret.serial       = rsutils::json::get< std::string >( j, "serial" );
    ret.product_line = rsutils::json::get< std::string >( j, "product-line" );
    ret.product_id   = rsutils::json::get< std::string >( j, "product-id" );
    ret.topic_root   = rsutils::json::get< std::string >( j, "topic-root" );
    ret.locked       = rsutils::json::get< bool >( j, "locked" );

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
