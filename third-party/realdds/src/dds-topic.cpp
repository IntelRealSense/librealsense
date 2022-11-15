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
    // Auto fill DDS X-Types TypeObject so other applications (e.g sniffer) can dynamically match a reader for this topic
    topic_type.get()->auto_fill_type_object( true );
    // Don't fill DDS X-Types TypeInformation, it is wasteful if you send TypeObject anyway
    topic_type.get()->auto_fill_type_information( false );
    // Registering the topic type with the participant enables topic instance creation by factory
    DDS_API_CALL( topic_type.register_type( _participant->get() ) );

    // Create the topic
    _topic = DDS_API_CALL( _participant->get()->create_topic( topic_name,
                                                              topic_type->getName(),
                                                              eprosima::fastdds::dds::TOPIC_QOS_DEFAULT ) );

    // Topic constructor creates TypeObject that will be sent as part of DDS discovery phase
    void * data = topic_type->createData();
    if ( data )
        topic_type->deleteData( data );
}


dds_topic::~dds_topic()
{
    if( _topic && _participant->is_valid() )
        DDS_API_CALL_NO_THROW( _participant->get()->delete_topic( _topic ) );
}


}  // namespace realdds
