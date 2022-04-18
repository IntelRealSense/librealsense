// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <types.h>
#include "dds-device-watcher.h"
#include <librealsense2/dds/topics/dds-messages.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

using namespace eprosima::fastdds::dds;
using namespace librealsense;

dds_device_watcher::dds_device_watcher( int domain_id )
    : _participant( nullptr )
    , _subscriber( nullptr )
    , _topic( nullptr )
    , _reader( nullptr )
    , _type_ptr( new dds::topics::devicesPubSubType() )
    , _init_done( false )
    , _domain_id( domain_id )
    , _active_object( [this]( dispatcher::cancellable_timer timer ) {

        if( _reader->wait_for_unread_message( { 1, 0 } ) )
        {
            dds::topics::devices data;
            SampleInfo info;
            bool device_update_detected = false;
            // Process all the samples until no one is returned,
            // We will distinguish info change vs new data by validating using `valid_data` field
            while( ReturnCode_t::RETCODE_OK == _reader->take_next_sample( &data, &info ) )
            {
                // Only samples for which valid_data is true should be accessed
                // valid_data indicates that the instance is still ALIVE and the `take` return an
                // updated sample
                if( info.valid_data )
                {
                    device_update_detected = true;
                    LOG_DEBUG( "DDS device '"
                               << std::string( data.name().begin(), data.name().end() )
                               << "' detected!" );
                }
            }

            if( device_update_detected )
            {
                std::lock_guard< std::mutex > lock( _devices_mutex );

                const eprosima::fastrtps::rtps::GUID_t & guid(
                    info.sample_identity.writer_guid() );  // Get the publisher GUID
                // Add a new device record into our dds devices map
                _dds_devices[guid.entityId.to_uint32()]
                    = std::string( data.name().begin(), data.name().end() );

                LOG_DEBUG( "DDS device writer GUID: " << std::hex << guid.entityId.to_uint32() << std::dec << " added on domain " << _domain_id );

                // TODO - Call LRS callback to create the RS devices
                // if( callback )
                // {
                //    callback_invocation_holder callback = { _callback_inflight.allocate(),
                //    &_callback_inflight }; _callback( _devices_data, curr );
                // }
            }
        }
    } )
{
    _domain_listener
        = std::make_shared< DiscoveryDomainParticipantListener >( [this]( uint32_t entity_id ) {
              std::lock_guard< std::mutex > lock( _devices_mutex );
              if( _dds_devices.find( entity_id ) != _dds_devices.end() )
              {
                  LOG_DEBUG( "DDS device writer GUID: " << std::hex << entity_id << std::dec << " removed" );
                  _dds_devices.erase( entity_id );
              }
          } );
}

void dds_device_watcher::start( platform::device_changed_callback callback )
{
    stop();
    _callback = std::move( callback );
    if( ! _init_done )
    {
        init( _domain_id );
        _init_done = true;
    }
    _active_object.start();
    LOG_DEBUG( "DDS device watcher started" );
}

void dds_device_watcher::stop()
{
    if( ! is_stopped() )
    {
        _active_object.stop();
        //_callback_inflight.wait_until_empty();
        LOG_DEBUG( "DDS device watcher stopped" );
    }
}


dds_device_watcher::~dds_device_watcher()
{
    stop();

    if( _subscriber != nullptr && _reader != nullptr )
    {
        _subscriber->delete_datareader( _reader );
    }
    if( _participant != nullptr )
    {
        if( _subscriber != nullptr )
        {
            _participant->delete_subscriber( _subscriber );
        }
        if( _topic != nullptr )
        {
            _participant->delete_topic( _topic );
        }
        DomainParticipantFactory::get_instance()->delete_participant( _participant );
    }
}

void dds_device_watcher::init( int domain_id )
{
    DomainParticipantQos pqos;
    pqos.name( "LRS_DEVICES_CLIENT" );

    // Indicates for how much time should a remote DomainParticipant consider the local
    // DomainParticipant to be alive.
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = { 10, 0 };  //[sec]

    _participant
        = DomainParticipantFactory::get_instance()->create_participant( domain_id,
                                                                        pqos,
                                                                        _domain_listener.get() );

    if( _participant == nullptr )
    {
        throw librealsense::backend_exception( "Error creating a DDS participant",
                                               RS2_EXCEPTION_TYPE_IO );
    }

    // REGISTER THE TYPE
    _type_ptr.register_type( _participant );

    // CREATE THE SUBSCRIBER
    _subscriber = _participant->create_subscriber( SUBSCRIBER_QOS_DEFAULT, nullptr );

    if( _subscriber == nullptr )
    {
        throw librealsense::backend_exception( "Error creating a DDS subscriber",
                                               RS2_EXCEPTION_TYPE_IO );
    }

    // CREATE THE TOPIC
    _topic = _participant->create_topic(  librealsense::dds::topics::DEVICES_TOPIC_NAME,
                                         _type_ptr->getName(),
                                         TOPIC_QOS_DEFAULT );

    if( _topic == nullptr )
    {
        throw librealsense::backend_exception( "Error creating a DDS Devices topic",
                                               RS2_EXCEPTION_TYPE_IO );
    }

    // CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;

    // The 'depth' parameter of the History defines how many samples are stored before starting to
    // overwrite them with newer ones.
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 10;

    rqos.reliability().kind
        = RELIABLE_RELIABILITY_QOS;  // We don't want to miss connection/disconnection events
    rqos.durability().kind = VOLATILE_DURABILITY_QOS;  // The Subscriber receives samples from the
                                                       // moment it comes online, not before
    rqos.data_sharing().automatic();                   // If possible, use shared memory
    rqos.ownership().kind = EXCLUSIVE_OWNERSHIP_QOS;
    _reader = _subscriber->create_datareader( _topic, rqos, nullptr );

    if( _reader == nullptr )
    {
        throw librealsense::backend_exception( "Error creating a DDS reader",
                                               RS2_EXCEPTION_TYPE_IO );
    }

    LOG_DEBUG( "DDS device watcher initialized successfully" );
}


dds_device_watcher::DiscoveryDomainParticipantListener::DiscoveryDomainParticipantListener(
    std::function< void( uint32_t ) > callback )
    : DomainParticipantListener()
    , _datawriter_removed_callback( std::move( callback ) )
{
}


void dds_device_watcher::DiscoveryDomainParticipantListener::on_publisher_discovery(
    DomainParticipant * participant, eprosima::fastrtps::rtps::WriterDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
        /* Process the case when a new publisher was found in the domain */
        LOG_DEBUG( "New DataWriter (" << info.info.guid() << ") publishing under topic '"
                                      << info.info.topicName() << "' of type '"
                                      << info.info.typeName() << "' discovered" );
        break;
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
        /* Process the case when a publisher was removed from the domain */
        LOG_DEBUG( "DataWriter (" << info.info.guid() << ") publishing under topic '"
                                  << info.info.topicName() << "' of type '" << info.info.typeName()
                                  << "' left the domain." );
        if( _datawriter_removed_callback )
            _datawriter_removed_callback( info.info.guid().entityId.to_uint32() );
        break;
    }
}
