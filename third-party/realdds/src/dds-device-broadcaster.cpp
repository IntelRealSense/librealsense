// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-broadcaster.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-guid.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/topics/flexible/flexiblePubSubTypes.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/json.h>

#include <iostream>

using namespace eprosima::fastdds::dds;
using namespace realdds;

// We want to know when readers join our topic
class dds_device_broadcaster::dds_client_listener : public realdds::dds_topic_writer //dds_topic_writer is a listener
{
public:
    dds_client_listener( std::shared_ptr< dds_topic > const & topic,
                         std::shared_ptr< dds_publisher > const & publisher,
                         dds_device_broadcaster * owner )
        : dds_topic_writer( topic, publisher )
        , _owner( owner )
    {
    }

    std::atomic_bool _new_reader_joined = { false };  // Used to indicate that a new reader has joined for this writer

protected:
    void on_publication_matched( eprosima::fastdds::dds::DataWriter * writer,
                                 eprosima::fastdds::dds::PublicationMatchedStatus const & info ) override
    {
        if( info.current_count_change == 1 )
        {
            // We send the work to the dispatcher to avoid waiting on the mutex here.
            _owner->_dds_device_dispatcher.invoke( [this]( dispatcher::cancellable_timer ) {
                {
                    std::lock_guard< std::mutex > lock( _owner->_new_client_mutex ); //TODO - mutxe locking needed here? Dispatcher needed here?
                    _new_reader_joined = true;
                    _owner->_trigger_msg_send = true;
                }
                _owner->_new_client_cv.notify_all();
            } );
        }
        else if( info.current_count_change == -1 )
        {
        }
        else
        {
            LOG_ERROR( std::to_string( info.current_count_change )
                       << " is not a valid value for on_publication_matched" );
        }
    }

private:
    dds_device_broadcaster * _owner;
};


dds_device_broadcaster::dds_device_broadcaster( std::shared_ptr< dds_participant > const & participant )
    : _trigger_msg_send( false )
    , _participant( participant )
    , _publisher( std::make_shared< dds_publisher >( participant ) )
    , _dds_device_dispatcher( 10 )
    , _new_client_handler( [this]( dispatcher::cancellable_timer ) {
        // We wait until the new reader callback indicate a new reader has joined or until the active object is stopped
        if( _active )
        {
            std::unique_lock< std::mutex > lock( _new_client_mutex );
            _new_client_cv.wait( lock, [this]() { return ! _active || _trigger_msg_send.load(); } );
            if( _active && _trigger_msg_send.load() )
            {
                _trigger_msg_send = false;
                _dds_device_dispatcher.invoke( [this]( dispatcher::cancellable_timer ) {
                    for( auto const & sn_and_handle : _device_handle_by_sn )
                    {
                        if( sn_and_handle.second.client_listener->_new_reader_joined )
                        {
                            if( send_device_info_msg( sn_and_handle.second ) )
                            {
                                sn_and_handle.second.client_listener->_new_reader_joined = false;
                            }
                        }
                    }
                } );
            }
        }
    } )
{
    if( ! _participant )
        DDS_THROW( runtime_error, "dds_device_broadcaster called with a null participant" );
}

bool dds_device_broadcaster::run()
{
    if( _active )
        DDS_THROW( runtime_error, "dds_device_broadcaster::run() called when already-active" );

    _dds_device_dispatcher.start();
    _new_client_handler.start();
    _active = true;
    return true;
}

void dds_device_broadcaster::add_device( device_info const & dev_info )
{
    if( ! _active )
        DDS_THROW( runtime_error, "dds_device_broadcaster::add_device called before run()" );
    // Post the connected device
    handle_device_changes( {}, { dev_info } );
}

void dds_device_broadcaster::remove_device( device_info const & dev_info )
{
    if( ! _active )
        DDS_THROW( runtime_error, "dds_device_broadcaster::remove_device called before run()" );
    // Post the disconnected device
    handle_device_changes( { dev_info.serial }, {} );
}

void dds_device_broadcaster::handle_device_changes( const std::vector< std::string > & devices_to_remove,
                                                    const std::vector< device_info > & devices_to_add )
{
    // We want to be as quick as possible and not wait for DDS in any way -- so we handle
    // device changes in the background, using invoke:
    _dds_device_dispatcher.invoke( [this, devices_to_add, devices_to_remove]( dispatcher::cancellable_timer ) {
        try
        {
            for( auto const & dev_to_remove : devices_to_remove )
            {
                remove_dds_device( dev_to_remove );
            }

            for( auto const & dev_to_add : devices_to_add )
            {
                if( ! add_dds_device( dev_to_add ) )
                {
                    LOG_ERROR( "Error creating a DDS writer" );
                }
            }
        }
        catch( ... )
        {
            LOG_ERROR( "Unknown error when trying to remove/add a DDS device" );
        }
    } );
}

void dds_device_broadcaster::remove_dds_device( const std::string & serial_number )
{
    auto it = _device_handle_by_sn.find( serial_number );
    if( it == _device_handle_by_sn.end() )
    {
        LOG_ERROR( "failed to remove non-existing device by serial number " << serial_number );
        return;
    }
    auto & handle = it->second;

    handle.client_listener.reset(); // deleting the writer notifies the clients (through DDS middleware)
    _device_handle_by_sn.erase( it );
}

bool dds_device_broadcaster::add_dds_device( const device_info & dev_info )
{
    if( _device_handle_by_sn.find( dev_info.serial ) == _device_handle_by_sn.end() )
    {
        if( ! create_device_writer( dev_info ) )
        {
            return false;
        }
    }

    return true;
}

bool dds_device_broadcaster::create_device_writer( const device_info & dev_info )
{
    if( ! _topic )
        _topic = topics::flexible_msg::create_topic( _publisher->get_participant(), "realsense/device-info" );

    // Create a data writer for the topic
    auto writer = std::make_shared< dds_client_listener >( _topic, _publisher, this );
    if( ! writer )
        return false;

    dds_topic_writer::qos wqos( RELIABLE_RELIABILITY_QOS ); //RELIABLE_RELIABILITY_QOS default but worth mentioning
    writer->run( wqos );

    auto it_inserted = _device_handle_by_sn.emplace( dev_info.serial,
                                                     dds_device_handle{ dev_info, writer } );
    assert( it_inserted.second );

    return true;
}

bool dds_device_broadcaster::send_device_info_msg( const dds_device_handle & handle )
{
    // Publish the device info, but only after a matching reader is found.
    topics::flexible_msg msg( handle.info.to_json() );

    // Post a DDS message with the new added device
    try
    {
        msg.write_to( *handle.client_listener );
        LOG_DEBUG( "DDS device message sent!" );
        return true;
    }
    catch( ... )
    {
        LOG_ERROR( "Error writing new device message for device : " << handle.info.serial );
    }
    return false;
}

dds_device_broadcaster::~dds_device_broadcaster()
{
    // Mark this class as inactive and wake up the active object do we can properly stop it.
    _active = false; 
    _new_client_cv.notify_all(); 
    
    _dds_device_dispatcher.stop();
    _new_client_handler.stop();

    // TODO - Will be destructed anyway by shared_ptr, worth mentioning explicitly?
    for( auto & sn_and_handle : _device_handle_by_sn )
    {
        sn_and_handle.second.client_listener.reset(); // deleting the writer notifies the clients (through DDS middleware)
    }
    _device_handle_by_sn.clear();
}
