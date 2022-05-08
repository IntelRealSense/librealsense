// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>
#include "dds-participant.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>

using namespace eprosima::fastdds::dds;
using namespace tools;

class dds_participant_listener : public eprosima::fastdds::dds::DomainParticipantListener
{

    virtual void
    on_participant_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                              eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info ) override;
};

dds_participant::dds_participant( int domain_id, std::string name )
    : _participant( nullptr ),
    _domain_listener(std::make_shared<dds_participant_listener>())
{
    DomainParticipantQos pqos;
    pqos.name( name );
    _participant
        = DomainParticipantFactory::get_instance()->create_participant( domain_id,
                                                                        pqos,
                                                                        _domain_listener.get() );

    if( ! _participant )
    {
        throw std::runtime_error( "Failed creating participant" + name + " on domain id "
                            + std::to_string( domain_id ) );
    }
}


dds_participant::~dds_participant()
{
    if( _participant->has_active_entities() )
    {
        std::cout << "participant has active entities!, deleting them all.." << std::endl;
        _participant->delete_contained_entities();
    }


    if( ! DomainParticipantFactory::get_instance()->delete_participant( _participant ) )
        std::cout << "Failed deleting participant!" << std::endl;
}

void dds_participant_listener::on_participant_discovery(
    DomainParticipant * participant, eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
        std::cout << "Participant '" << info.info.m_participantName << "' discovered" << std::endl;
        break;
    case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
    case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
        std::cout << "Participant '" << info.info.m_participantName << "' disappeared" << std::endl;
        break;
    default:
        break;
    }
}
