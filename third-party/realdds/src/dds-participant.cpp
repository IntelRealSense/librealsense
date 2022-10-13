// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-guid.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>

#include <map>
#include <mutex>

using namespace eprosima::fastdds::dds;
using namespace realdds;


namespace {
    std::map< dds_guid_prefix, std::string > participants;
    std::mutex participants_mutex;
}


struct dds_participant::listener_impl : public eprosima::fastdds::dds::DomainParticipantListener
{
    dds_participant & _owner;

    listener_impl() = delete;
    listener_impl( dds_participant & owner )
        : _owner( owner )
    {
    }

    virtual void on_participant_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                           eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info ) override
    {
        switch( info.status )
        {
        case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
            LOG_DEBUG( "+participant '" << info.info.m_participantName << "' (" << _owner.print( info.info.m_guid )
                                        << ")" );
            {
                std::lock_guard< std::mutex > lock( participants_mutex );
                participants.emplace( info.info.m_guid.guidPrefix, info.info.m_participantName );
            }
            _owner.on_participant_added( info.info.m_guid, info.info.m_participantName.c_str() );
            break;
        case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
        case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
            LOG_DEBUG( "-participant " << _owner.print( info.info.m_guid ) );
            _owner.on_participant_removed( info.info.m_guid, info.info.m_participantName.c_str() );
            {
                std::lock_guard< std::mutex > lock( participants_mutex );
                participants.erase( info.info.m_guid.guidPrefix );
            }
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
            LOG_DEBUG( "+DataWriter (" << _owner.print( info.info.guid() ) << ") publishing '"
                                       << info.info.topicName() << "' of type '" << info.info.typeName() << "'" );
            _owner.on_writer_added( info.info.guid(), info.info.topicName().c_str() );
            break;

        case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
            /* Process the case when a publisher was removed from the domain */
            LOG_DEBUG( "-DataWriter (" << _owner.print( info.info.guid() ) << ") publishing '"
                                       << info.info.topicName() << "'" );
            _owner.on_writer_removed( info.info.guid(), info.info.topicName().c_str() );
            break;
        }
    }

    virtual void on_subscriber_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                          eprosima::fastrtps::rtps::ReaderDiscoveryInfo && info ) override
    {
        switch( info.status )
        {
        case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER:
            LOG_DEBUG( "+DataReader (" << _owner.print( info.info.guid() ) << ") reading topic '"
                                       << info.info.topicName() << "' of type '" << info.info.typeName() << "'" );
            _owner.on_reader_added( info.info.guid(), info.info.topicName().c_str() );
            break;

        case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::REMOVED_READER:
            LOG_DEBUG( "-DataReader (" << _owner.print( info.info.guid() ) << ") reading topic '"
                                      << info.info.topicName() << "'" );
            _owner.on_reader_removed( info.info.guid(), info.info.topicName().c_str() );
            break;
        }
    }

    virtual void on_type_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                    const eprosima::fastrtps::rtps::SampleIdentity & request_sample_id,
                                    const eprosima::fastrtps::string_255 & topic_name,
                                    const eprosima::fastrtps::types::TypeIdentifier * identifier,
                                    const eprosima::fastrtps::types::TypeObject * object,
                                    eprosima::fastrtps::types::DynamicType_ptr dyn_type ) override
    {
        TypeSupport type_support( new eprosima::fastrtps::types::DynamicPubSubType( dyn_type ) );
        LOG_DEBUG( "Discovered topic " << topic_name << " of type: " << type_support->getName() );
        _owner.on_type_discovery( topic_name.c_str(), dyn_type );
    }
};


void dds_participant::init( dds_domain_id domain_id, std::string const & participant_name )
{
    if( is_valid() )
    {
        throw std::runtime_error( "participant is already initialized; cannot init '" + participant_name
                                  + "' on domain id " + std::to_string( domain_id ) );
    }

    _domain_listener = std::make_shared< listener_impl >( *this );

    DomainParticipantQos pqos;
    pqos.name( participant_name );

    // Indicates for how much time should a remote DomainParticipant consider the local DomainParticipant to be alive.
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = { 10, 0 };  // [sec,nsec]

    // Listener will call DataReaderListener::on_data_available for a specific reader,
    // not SubscriberListener::on_data_on_readers for any reader
    // ( See note on https://fast-dds.docs.eprosima.com/en/v2.7.0/fastdds/dds_layer/core/entity/entity.html )
    StatusMask par_mask = StatusMask::all() >> StatusMask::data_on_readers();
    _participant = DDS_API_CALL( DomainParticipantFactory::get_instance()->create_participant( domain_id,
                                                                                               pqos,
                                                                                               _domain_listener.get(),
                                                                                               par_mask ) );

    if( ! _participant )
    {
        throw std::runtime_error( "failed creating participant " + participant_name + " on domain id "
                                  + std::to_string( domain_id ) );
    }

    LOG_DEBUG( "participant '" << participant_name << "' (" << realdds::print( guid() ) << ") is up on domain "
                               << domain_id );
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


dds_guid const & dds_participant::guid() const
{
    return get()->guid();
}


std::string dds_participant::print( dds_guid const& guid_to_print ) const
{
    return realdds::print( guid_to_print, guid() );
}


/*static*/ std::string dds_participant::name_from_guid( dds_guid_prefix const & pref )
{
    std::string participant;
    std::lock_guard< std::mutex > lock( participants_mutex );
    auto it = participants.find( pref );
    if( it != participants.end() )
        participant = it->second;
    return participant;
}


/*static*/ std::string dds_participant::name_from_guid( dds_guid const & guid )
{
    return name_from_guid( guid.guidPrefix );
}


void dds_participant::on_writer_added( dds_guid guid, char const * topic_name )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_writer_added )
                l->_on_writer_added( guid, topic_name );
        }
    }
}


void dds_participant::on_writer_removed( dds_guid guid, char const * topic_name )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_writer_removed )
                l->_on_writer_removed( guid, topic_name );
        }
    }
}


void dds_participant::on_reader_added( dds_guid guid, char const * topic_name )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_reader_added )
                l->_on_reader_added( guid, topic_name );
        }
    }
}


void dds_participant::on_reader_removed( dds_guid guid, char const * topic_name )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_reader_removed )
                l->_on_reader_removed( guid, topic_name );
        }
    }
}


void dds_participant::on_participant_added( dds_guid guid, char const * participant_name )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_participant_added )
                l->_on_participant_added( guid, participant_name );
        }
    }
}


void dds_participant::on_participant_removed( dds_guid guid, char const * participant_name )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_participant_removed )
                l->_on_participant_removed( guid, participant_name );
        }
    }
}


void dds_participant::on_type_discovery( char const * topic_name, eprosima::fastrtps::types::DynamicType_ptr dyn_type )
{
    for( auto wl : _listeners )
    {
        if( auto l = wl.lock() )
        {
            if( l->_on_type_discovery )
                l->_on_type_discovery( topic_name, dyn_type );
        }
    }
}
