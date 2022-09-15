// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "device-info-msg.h"
#include "deviceInfoPubSubTypes.h"


namespace librealsense {
namespace dds {
namespace topics {


device_info::device_info( raw::device_info const & dev )
    : name( dev.name().data() )
    , serial( dev.serial_number().data() )
    , product_line( dev.product_line().data() )
    , topic_root( dev.topic_root().data() )
    , locked( dev.locked() )
{
}


}  // namespace topics
}  // namespace dds
}  // namespace librealsense
