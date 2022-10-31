// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-subscriber.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>


namespace realdds {


dds_subscriber::dds_subscriber( std::shared_ptr< dds_participant > const & participant )
    : _participant( participant )
{
    _subscriber = DDS_API_CALL(
        _participant->get()->create_subscriber( eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr ) );
}


dds_subscriber::~dds_subscriber()
{
    if( _subscriber &&  _participant->is_valid() )
        DDS_API_CALL_NO_THROW( _participant->get()->delete_subscriber( _subscriber ) );
}


}  // namespace realdds
