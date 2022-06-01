// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-participant.h"
#include "dds-utilities.h"

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>

#include <iostream>
#include <memory>

using namespace eprosima::fastdds::dds;
using namespace librealsense::dds;

struct dds_participant::dds_participant_listener
    : public eprosima::fastdds::dds::DomainParticipantListener
{
    dds_participant & _owner;

    dds_participant_listener() = delete;
    dds_participant_listener( dds_participant & owner )
        : _owner( owner )
    {
    }

    virtual void on_participant_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                              eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info ) override
    {
        switch( info.status )
        {
        case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
            LOG_DEBUG( "Participant '" << info.info.m_participantName << "' discovered" );
            break;
        case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
        case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
            LOG_DEBUG( "Participant '" << info.info.m_participantName << "' disappeared" );
            break;
        default:
            break;
        }
    }

    virtual void on_publisher_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                         eprosima::fastrtps::rtps::WriterDiscoveryInfo && info ) override
    {
        switch( info.status )
        {
        case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
            /* Process the case when a new publisher was found in the domain */
            LOG_DEBUG( "New DataWriter (" << info.info.guid() << ") publishing under topic '" << info.info.topicName()
                                          << "' of type '" << info.info.typeName() << "' discovered" );
            if( _owner._on_writer_added )
                _owner._on_writer_added( info.info.guid(), info.info.topicName().c_str() );
            break;

        case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
            /* Process the case when a publisher was removed from the domain */
            LOG_DEBUG( "DataWriter (" << info.info.guid() << ") publishing under topic '" << info.info.topicName()
                                      << "' of type '" << info.info.typeName() << "' left the domain." );
            if( _owner._on_writer_removed )
                _owner._on_writer_removed( info.info.guid() );
            break;
        }
    }

};


void dds_participant::init( dds_domain_id domain_id, std::string const & participant_name )
{
    if( is_valid() )
    {
        throw std::runtime_error( "participant is already initialized; cannot init '" + participant_name
                                  + "' on domain id " + std::to_string( domain_id ) );
    }

    _domain_listener = std::make_shared< dds_participant_listener >( *this );

    DomainParticipantQos pqos;
    pqos.name( participant_name );

    // Indicates for how much time should a remote DomainParticipant consider the local
    // DomainParticipant to be alive.
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = { 10, 0 };  // [sec,nsec]

    _participant = DDS_API_CALL(DomainParticipantFactory::get_instance()->create_participant( domain_id, pqos, _domain_listener.get() ));
    
    if( ! _participant )
    {
        throw std::runtime_error( "failed creating participant " + participant_name + " on domain id "
                                  + std::to_string( domain_id ) );
    }

    LOG_DEBUG( "participant '" << participant_name << "' is up on domain " << domain_id );
}


dds_participant::~dds_participant()
{
    if( is_valid() )
    {
        if( _participant->has_active_entities() )
        {
            LOG_DEBUG( "participant has active entities!, deleting them all.." );
            DDS_API_CALL_NO_THROW( _participant->delete_contained_entities() );
        }

        DDS_API_CALL_NO_THROW( DomainParticipantFactory::get_instance()->delete_participant( _participant ) );
    }
}

