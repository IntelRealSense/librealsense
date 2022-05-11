// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "dds-device-broadcaster.h"
#include "dds-participant.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.h>
#include <librealsense2/dds/topics/dds-messages.h>

#define RS_ROOT "realsense/"
#define DEVICE_NAME_PREFIX "Intel RealSense "

using namespace eprosima::fastdds::dds;
using namespace tools;

dds_device_broadcaster::dds_device_broadcaster(tools::dds_participant &participant)
    : _trigger_msg_send ( false )
    , _participant( participant.get() )
    , _publisher( nullptr )
    , _topic( nullptr )
    , _topic_type( new librealsense::dds::topics::device_info::type )
    , _dds_device_dispatcher( 10 )
    , _new_client_handler( [this]( dispatcher::cancellable_timer timer ) {

        // We wait until the new reader callback indicate a new reader has joined or until the
        // active object is stopped
        std::unique_lock< std::mutex > lock( _new_client_mutex );
        _new_client_cv.wait( lock, [this, timer]() {
            return _trigger_msg_send.load() || timer.was_stopped();
        } );
        if( _new_client_handler.is_active() && _trigger_msg_send.load() )
        {
            _trigger_msg_send = false;
            _dds_device_dispatcher.invoke( [this]( dispatcher::cancellable_timer ) {
                for( auto sn_and_handle : _device_handle_by_sn )
                {
                    if( sn_and_handle.second.listener->_new_reader_joined )
                    {
                        auto dev_info = query_device_info( sn_and_handle.second.device );
                        if( send_device_info_msg( dev_info ) )
                        {
                            sn_and_handle.second.listener->_new_reader_joined = false;
                        }
                    }
                }
            } );
        }
    } )

    , _ctx( "{"
            "\"dds-discovery\" : false"
            "}" )

{
}

bool dds_device_broadcaster::run()
{
    if( ! _participant )
    {
        std::cout << "Error - Participant is not valid" << std::endl;
        return false;
    }

    if( ! create_dds_publisher() )
    {
        std::cout << "Error - Failed creating publisher" << std::endl;
        return false;
    }

    _dds_device_dispatcher.start();
    _new_client_handler.start();
    return true;
}

void tools::dds_device_broadcaster::add_device( rs2::device dev )
{
    auto device_serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    std::vector< std::pair< std::string, rs2::device > > devices_to_add;
    devices_to_add.push_back( { device_serial, dev } );

    // Post the connected device
    handle_device_changes( {}, devices_to_add );
}

void tools::dds_device_broadcaster::remove_device( rs2::device dev )
{
    auto device_serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    std::vector< std::string > devices_to_remove;
    devices_to_remove.push_back( device_serial );

    // Post the disconnected device
    handle_device_changes( devices_to_remove, {} );
}

void dds_device_broadcaster::handle_device_changes(
    const std::vector< std::string > & devices_to_remove,
    const std::vector< std::pair< std::string, rs2::device > > & devices_to_add )
{
    _dds_device_dispatcher.invoke(
        [this, devices_to_add, devices_to_remove]( dispatcher::cancellable_timer ) {
            try
            {
                for( auto dev_to_remove : devices_to_remove )
                {
                    remove_dds_device( dev_to_remove );
                }

                for( auto dev_to_add : devices_to_add )
                {
                    if( ! add_dds_device( dev_to_add.first, dev_to_add.second ) )
                    {
                        std::cout << "Error creating a DDS writer" << std::endl;
                    }
                }
            }

            catch( ... )
            {
                std::cout << "Unknown error when trying to remove/add a DDS device" << std::endl;
            }
        } );
}

void dds_device_broadcaster::remove_dds_device( const std::string & device_key )
{
    // deleting a device also notify the clients internally
    auto ret = _publisher->delete_datawriter( _device_handle_by_sn[device_key].data_writer );
    if( ret != ReturnCode_t::RETCODE_OK)
    {
        std::cout << "Error code: " << ret() << " while trying to delete data writer ("
                  << _device_handle_by_sn[device_key].data_writer->guid() << ")" << std::endl;
        return;
    }
    _device_handle_by_sn.erase( device_key );
    std::cout << "Device '" << device_key << "' - removed" << std::endl;
}

bool dds_device_broadcaster::add_dds_device( const std::string & device_key,
                                 const rs2::device & rs2_dev )
{
    if( _device_handle_by_sn.find( device_key ) == _device_handle_by_sn.end() )
    {
        std::cout << "\nDevice '" << device_key << "' - detected" << std::endl;
        if( ! create_device_writer( device_key, rs2_dev ) )
        {
            return false;
        }
    }

    return true;
}

bool dds_device_broadcaster::create_device_writer( const std::string &device_key, rs2::device rs2_device )
{
    // Create a data writer for the topic
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.durability().kind = VOLATILE_DURABILITY_QOS;

    //---------------------------------------------------------------------------------------
    // The writer-reader handshake is done on UDP, even when data_sharing (shared memory) is used for
    // actual messaging. This means it is possible to send a message and receive it on the reader’s
    // side even before the UDP handshake is complete:
    //   1. The writer goes up and broadcasts its presence periodically; no readers exist
    //   2. The reader joins and broadcasts its presence, again periodically; it doesn’t know about the writer yet
    //   3. The writer sees the reader (in-between broadcasts) so sends a message
    //   4. The reader gets the message and discards it because it does not yet recognize the writer
    // This depends on timing. When shared memory is on, step 3 is so fast that this miscommunication is much more likely.
    // This is a known gap in the DDS standard.
    //
    // We can either insert a sleep between writer creation and message sending or we can disable
    // data_sharing for this topic, which we did here.
    // (See https://github.com/eProsima/Fast-DDS/issues/2641)

    //wqos.data_sharing().automatic();
    wqos.data_sharing().off();
    //---------------------------------------------------------------------------------------

    wqos.ownership().kind = EXCLUSIVE_OWNERSHIP_QOS;
    std::shared_ptr< dds_client_listener > writer_listener
        = std::make_shared< dds_client_listener >( this );

    _device_handle_by_sn[device_key]
        = { rs2_device,
            _publisher->create_datawriter( _topic, wqos, writer_listener.get() ),
            writer_listener };

    return _device_handle_by_sn[device_key].data_writer != nullptr;
}

bool dds_device_broadcaster::create_dds_publisher()
{
    // Registering the topic type enables topic instance creation by factory
    _topic_type.register_type( _participant );
    _publisher = _participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr );
    _topic = _participant->create_topic( librealsense::dds::topics::device_info::TOPIC_NAME,
                                         _topic_type->getName(),
                                         TOPIC_QOS_DEFAULT );

    return ( _topic != nullptr && _publisher != nullptr );
}

bool tools::dds_device_broadcaster::send_device_info_msg( const librealsense::dds::topics::device_info& dev_info )
{
    // Publish the device info, but only after a matching reader is found.
    librealsense::dds::topics::raw::device_info raw_msg;
    fill_device_msg( dev_info, raw_msg );

    // Post a DDS message with the new added device
    if( _device_handle_by_sn[dev_info.serial].data_writer->write( &raw_msg ) )
    {
        std::cout << "DDS device message sent!" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Error writing new device message for device : " << dev_info.serial << std::endl;
    }
    return false;
}

librealsense::dds::topics::device_info dds_device_broadcaster::query_device_info( const rs2::device &rs2_dev ) const
{
    librealsense::dds::topics::device_info dev_info;
    dev_info.name = rs2_dev.get_info( RS2_CAMERA_INFO_NAME );
    dev_info.serial = rs2_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    dev_info.product_line = rs2_dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE );
    dev_info.locked = (rs2_dev.get_info( RS2_CAMERA_INFO_CAMERA_LOCKED ) == "YES");

    // Build device topic root path
    dev_info.topic_root = get_topic_root( dev_info );
    return dev_info;
}

void dds_device_broadcaster::fill_device_msg( const librealsense::dds::topics::device_info & dev_info,
                                  librealsense::dds::topics::raw::device_info & msg ) const
{
    strcpy( msg.name().data(), dev_info.name.c_str() );
    strcpy( msg.serial_number().data(), dev_info.serial.c_str() );
    strcpy( msg.product_line().data(), dev_info.product_line.c_str() );
    strcpy( msg.topic_root().data(), dev_info.topic_root.c_str() );
    msg.locked() = dev_info.locked;
}

std::string tools::dds_device_broadcaster::get_topic_root( const librealsense::dds::topics::device_info& dev_info ) const
{
    // Build device root path (we use a device model only name like DXXX)
    // example: /realsense/D435/11223344
    std::string rs_device_name_prefix = DEVICE_NAME_PREFIX;
    std::string short_device_name = dev_info.name.substr( rs_device_name_prefix.length() );
    return RS_ROOT + short_device_name + "/" + dev_info.serial;
}

dds_device_broadcaster::~dds_device_broadcaster()
{
    std::cout << "Shutting down rs-dds-server..." << std::endl;

    _dds_device_dispatcher.stop();
    _new_client_handler.stop();

    _new_client_cv.notify_all(); // Wake up _device_info_msg_sender to finish running

    for( auto sn_and_handle : _device_handle_by_sn )
    {
        auto & dev_writer = sn_and_handle.second.data_writer;
        if( dev_writer )
            _publisher->delete_datawriter( dev_writer );
    }
    _device_handle_by_sn.clear();

    if( _topic != nullptr )
    {
        _participant->delete_topic( _topic );
    }
    if( _publisher != nullptr )
    {
        _participant->delete_publisher( _publisher );
    }
}


void dds_device_broadcaster::dds_client_listener::on_publication_matched( DataWriter * writer,
                                                             const PublicationMatchedStatus & info )
{
    if( info.current_count_change == 1 )
    {
        std::cout << "DataReader " << writer->guid() << " discovered" << std::endl;
        {
            // We send the work to the dispatcher to avoid waiting on the mutex here.
            _owner->_dds_device_dispatcher.invoke( [this]( dispatcher::cancellable_timer ) {
                {
                    std::lock_guard< std::mutex > lock( _owner->_new_client_mutex );
                    _new_reader_joined = true;
                    _owner->_trigger_msg_send = true;
                }
                _owner->_new_client_cv.notify_all();
            } );
        }
    }
    else if( info.current_count_change == -1 )
    {
        std::cout << "DataReader " << writer->guid() << " disappeared" << std::endl;
    }
    else
    {
        std::cout << std::to_string( info.current_count_change )
                  << " is not a valid value for on_publication_matched" << std::endl;
    }
}

