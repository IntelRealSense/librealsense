// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/notification/notification-msg.h>
#include <realdds/topics/notification/notificationPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {
namespace device {


/*static*/ std::shared_ptr< dds_topic >
notification::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new notification::type ),
                                          topic_name );
}


}  // namespace device
}  // namespace topics
}  // namespace realdds
