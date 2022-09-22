// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "device-info-msg.h"
#include "deviceInfoPubSubTypes.h"

#include "../../dds-topic.h"

#include <fastdds/dds/topic/TypeSupport.hpp>


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


/*static*/ std::shared_ptr< dds_topic > device_info::create( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new device_info::type ),
                                          topic_name );
}


}  // namespace topics
}  // namespace dds
}  // namespace librealsense
