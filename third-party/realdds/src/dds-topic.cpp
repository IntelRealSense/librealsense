// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {


dds_topic::dds_topic( std::shared_ptr< dds_participant > const & participant,
                      eprosima::fastdds::dds::TypeSupport const & topic_type,
                      char const * topic_name )
    : _participant( participant )
    , _topic( nullptr )
{
    // REGISTER THE TYPE
    DDS_API_CALL( topic_type.register_type( _participant->get() ) );

    // CREATE THE TOPIC
    _topic = DDS_API_CALL( _participant->get()->create_topic( topic_name,
                                                              topic_type->getName(),
                                                              eprosima::fastdds::dds::TOPIC_QOS_DEFAULT ) );
}


dds_topic::~dds_topic()
{
    if( _topic && _participant->is_valid() )
        DDS_API_CALL_NO_THROW( _participant->get()->delete_topic( _topic ) );
}


}  // namespace realdds
