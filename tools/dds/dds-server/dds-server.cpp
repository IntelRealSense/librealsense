// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "dds-server.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.h>
#include <librealsense2/dds/topics/dds-messages.h>

using namespace eprosima::fastdds::dds;
using namespace tools;

dds_server::dds_server()
    : _running( false )
    , _participant( nullptr )
    , _publisher( nullptr )
    , _topic( nullptr )
    , _topic_type( new librealsense::dds::topics::device_info::type )
    , _dds_device_dispatcher( 10 )
    , _ctx( "{"
            "\"dds-discovery\" : false"
            "}" )
{
}
bool dds_server::init( DomainId_t domain_id )
{
    auto participant_created_ok = create_dds_participant( domain_id );
    auto publisher_created_ok = create_dds_publisher();
    return participant_created_ok && publisher_created_ok;
}

void dds_server::run()
{
    _dds_device_dispatcher.start();
    _running = true;

    post_connected_devices_on_wakeup();

    // Register to LRS device changes function to notify on future devices being
    // connected/disconnected
    _ctx.set_devices_changed_callback( [this]( rs2::event_information & info ) {
        if( _running )
        {
            std::vector< std::string > devices_to_remove;
            std::vector< std::pair< librealsense::dds::topics::device_info, rs2::device > > devices_to_add;

            if( prepare_devices_changed_lists( info, devices_to_remove, devices_to_add ) )
            {
                // Post the devices connected / removed
                _dds_device_dispatcher.invoke(
                    [this, devices_to_remove, devices_to_add]( dispatcher::cancellable_timer c ) {
                        post_device_changes( devices_to_remove, devices_to_add );
                    } );
            }
        }
    } );

    std::cout << "RS DDS Server is on.." << std::endl;
}

bool dds_server::prepare_devices_changed_lists(
    const rs2::event_information & info,
    std::vector< std::string > & devices_to_remove,
    std::vector< std::pair< librealsense::dds::topics::device_info, rs2::device > > & devices_to_add )
{
    // Remove disconnected devices from devices list
    for( auto dev_info : _devices_writers )
    {
        auto & dev = dev_info.second.device;
        auto device_key = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );

        if( info.was_removed( dev ) )
        {
            devices_to_remove.push_back( device_key );
        }
    }

    // Add new connected devices from devices list
    for( auto && dev : info.get_new_devices() )
    {
        auto rs_dev_info = query_device_info( dev );
        auto device_name = dev.get_info( RS2_CAMERA_INFO_NAME );
        devices_to_add.push_back( { rs_dev_info, dev } );
    }

    bool device_change_detected = ! devices_to_remove.empty() || ! devices_to_add.empty();
    return device_change_detected;
}

void dds_server::post_device_changes(
    const std::vector< std::string > & devices_to_remove,
    const std::vector< std::pair< librealsense::dds::topics::device_info, rs2::device > > & devices_to_add )
{
    try
    {
        for( auto dev_to_remove : devices_to_remove )
        {
            remove_dds_device( dev_to_remove );
        }

        for( auto dev_to_add : devices_to_add )
        {
            add_dds_device( dev_to_add.first, dev_to_add.second );
        }
    }

    catch( ... )
    {
        std::cout << "Unknown error when trying to remove/add a DDS device" << std::endl;
    }
}

void dds_server::remove_dds_device( const std::string & device_key )
{
    // deleting a device also notify the clients internally
    auto ret = _publisher->delete_datawriter( _devices_writers[device_key].data_writer );
    if( ret != ReturnCode_t::RETCODE_OK)
    {
        std::cout << "Error code: " << ret() << " while trying to delete data writer ("
                  << _devices_writers[device_key].data_writer->guid() << ")" << std::endl;
        return;
    }
    _devices_writers.erase( device_key );
    std::cout << "Device '" << device_key << "' - removed" << std::endl;
}

void dds_server::add_dds_device( const librealsense::dds::topics::device_info &dev_info, const rs2::device & rs2_dev )
{

    if( ! create_device_writer( dev_info.serial, rs2_dev ) )
    {
        std::cout << "Error creating a DDS writer" << std::endl;
        return;
    }

    // Publish the device info, but only after a matching reader is found.
    librealsense::dds::topics::raw::device_info raw_msg;
    fill_device_msg( dev_info, raw_msg );
    std::cout << "\nDevice '" << dev_info.serial << "' - detected" << std::endl;
    std::cout << "Looking for at least 1 matching reader... ";  // Status value will be appended to
                                                                // this line
    // It takes some time from the moment we create the data writer until the data reader is matched
    // If we send before the data reader is matched the message will not arrive to it.
    // Currently if we remove the sleep line the client sometimes miss the message. (See https://github.com/eProsima/Fast-DDS/issues/2641)
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    if( verify_client_exist( dev_info.serial, std::chrono::seconds( 1 ) ) )
    {
        std::cout << "found" << std::endl;
        // Post a DDS message with the new added device
        if( _devices_writers[dev_info.serial].data_writer->write( &raw_msg ) )
        {
            std::cout << "DDS device message sent!" << std::endl;
        }
        else
        {
            std::cout << "Error writing new device message for device : " << dev_info.serial << std::endl;
        }
    }
    else
    {
        std::cout << "not found" << std::endl;
        std::cout << "Timeout finding a reader for devices topic" << std::endl;
    }
}

bool dds_server::verify_client_exist( const std::string & device_key, std::chrono::steady_clock::duration timeout ) const
{
    bool client_found = false;
    auto & listener = _devices_writers.at( device_key ).listener;
    client_found = listener->_matched != 0;

    if( timeout > std::chrono::steady_clock::duration::zero() )
    {
        int retry_cnt = 4;
        auto interval_timeout = timeout / retry_cnt;
        while( !client_found && retry_cnt-- > 0 )
        {
            std::this_thread::sleep_for( interval_timeout );
            std::cout << "slept for " << interval_timeout.count() << " time" << std::endl;
            client_found = listener->_matched != 0;
        }
    }
    return client_found;
}

bool dds_server::create_device_writer( const std::string &device_key, rs2::device rs2_device )
{
    // Create a data writer for the topic
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.durability().kind = VOLATILE_DURABILITY_QOS;
    wqos.data_sharing().automatic();
    wqos.ownership().kind = EXCLUSIVE_OWNERSHIP_QOS;
    std::shared_ptr< dds_serverListener > writer_listener
        = std::make_shared< dds_serverListener >();

    _devices_writers[device_key]
        = { rs2_device,
            _publisher->create_datawriter( _topic, wqos, writer_listener.get() ),
            writer_listener };

    return _devices_writers[device_key].data_writer != nullptr;
}

bool dds_server::create_dds_participant( DomainId_t domain_id )
{
    DomainParticipantQos pqos;
    pqos.name( "rs-dds-server" );
    _participant
        = DomainParticipantFactory::get_instance()->create_participant( domain_id,
                                                                        pqos,
                                                                        &_domain_listener );
    return _participant != nullptr;
}

bool dds_server::create_dds_publisher()
{
    // Registering the topic type enables topic instance creation by factory
    _topic_type.register_type( _participant );
    _publisher = _participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr );
    _topic = _participant->create_topic( librealsense::dds::topics::device_info::DEVICE_INFO_TOPIC_NAME,
                                         _topic_type->getName(),
                                         TOPIC_QOS_DEFAULT );

    return ( _topic != nullptr && _publisher != nullptr );
}

void dds_server::post_connected_devices_on_wakeup()
{
    // Query the devices connected on startup
    auto connected_dev_list = _ctx.query_devices();
    std::vector< std::pair< librealsense::dds::topics::device_info , rs2::device > > devices_to_add;

    for( auto connected_dev : connected_dev_list )
    {
        auto dev_info = query_device_info( connected_dev );
        devices_to_add.push_back( { dev_info, connected_dev } );
    }

    if( ! devices_to_add.empty() )
    {
        // Post the devices connected on startup
        _dds_device_dispatcher.invoke( [this, devices_to_add]( dispatcher::cancellable_timer c ) {
            post_device_changes( {}, devices_to_add );
        } );
    }
}

librealsense::dds::topics::device_info dds_server::query_device_info( const rs2::device &rs2_dev ) const
{
    librealsense::dds::topics::device_info dev_info;
    dev_info.name = rs2_dev.get_info( RS2_CAMERA_INFO_NAME );
    dev_info.serial = rs2_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    dev_info.product_line = rs2_dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE );
    dev_info.locked = (rs2_dev.get_info( RS2_CAMERA_INFO_CAMERA_LOCKED ) == "YES");
    return dev_info;
}

void dds_server::fill_device_msg( const librealsense::dds::topics::device_info & dev_info,
                                  librealsense::dds::topics::raw::device_info & msg ) const
{
    strcpy( msg.name().data(), dev_info.name.c_str() );
    strcpy( msg.serial_number().data(), dev_info.serial.c_str() );
    strcpy( msg.product_line().data(), dev_info.product_line.c_str() );
    msg.locked() = dev_info.locked;
}

dds_server::~dds_server()
{
    std::cout << "Shutting down rs-dds-server..." << std::endl;
    _running = false;

    _dds_device_dispatcher.stop();

    for( auto device_writer : _devices_writers )
    {
        auto & dev_writer = device_writer.second.data_writer;
        if( dev_writer )
            _publisher->delete_datawriter( dev_writer );
    }
    _devices_writers.clear();

    if( _topic != nullptr )
    {
        _participant->delete_topic( _topic );
    }
    if( _publisher != nullptr )
    {
        _participant->delete_publisher( _publisher );
    }
    DomainParticipantFactory::get_instance()->delete_participant( _participant );
}


void dds_server::dds_serverListener::on_publication_matched( DataWriter * writer,
                                                             const PublicationMatchedStatus & info )
{
    if( info.current_count_change == 1 )
    {
        std::cout << "DataReader " << writer->guid() << " discovered" << std::endl;
        _matched = info.total_count;
    }
    else if( info.current_count_change == -1 )
    {
        std::cout << "DataReader " << writer->guid() << " disappeared" << std::endl;
        _matched = info.total_count;
    }
    else
    {
        std::cout << std::to_string( info.current_count_change )
                  << " is not a valid value for on_publication_matched" << std::endl;
    }
}

void dds_server::DiscoveryDomainParticipantListener::on_participant_discovery(
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
