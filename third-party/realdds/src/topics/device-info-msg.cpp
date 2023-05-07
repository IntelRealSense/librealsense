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
    rsutils::json::get_ex( j, "serial", &ret.serial );
    rsutils::json::get_ex( j, "product-line", &ret.product_line );
    rsutils::json::get_ex( j, "product-id", &ret.product_id );
    ret.topic_root   = rsutils::json::get< std::string >( j, "topic-root" );
    rsutils::json::get_ex( j, "locked", &ret.locked );

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
    return msg;
}

}  // namespace topics
}  // namespace realdds
