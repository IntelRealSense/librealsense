// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-publisher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>


namespace realdds {


dds_publisher::dds_publisher( std::shared_ptr< dds_participant > const & participant )
    : _participant( participant )
{
    _publisher = DDS_API_CALL(
        _participant->get()->create_publisher( eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr ) );
}


dds_publisher::~dds_publisher()
{
    if( _publisher  &&  _participant->is_valid() )
        DDS_API_CALL_NO_THROW( _participant->get()->delete_publisher( _publisher ) );
}


}  // namespace realdds
