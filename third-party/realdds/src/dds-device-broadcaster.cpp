// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-broadcaster.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-guid.h>
#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/device-info/deviceInfoPubSubTypes.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.h>

#include <iostream>

using namespace eprosima::fastdds::dds;
using namespace realdds;

// We want to know when readers join our topic
class dds_device_broadcaster::dds_client_listener : public eprosima::fastdds::dds::DataWriterListener
{
public:
    dds_client_listener( dds_device_broadcaster * owner )
        : eprosima::fastdds::dds::DataWriterListener()
        , _owner( owner )
    {
    }

    void on_publication_matched( eprosima::fastdds::dds::DataWriter * writer,
                                 const eprosima::fastdds::dds::PublicationMatchedStatus & info ) override
    {
        if( info.current_count_change == 1 )
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
        else if( info.current_count_change == -1 )
        {
        }
        else
        {
            LOG_ERROR( std::to_string( info.current_count_change )
                       << " is not a valid value for on_publication_matched" );
        }
    }

    std::atomic_bool _new_reader_joined = { false };  // Used to indicate that a new reader has joined for this writer
private:
    dds_device_broadcaster * _owner;
};


dds_device_broadcaster::dds_device_broadcaster( std::shared_ptr< dds_participant > const & participant )
    : _trigger_msg_send( false )
    , _participant( participant )
    , _publisher( nullptr )
    , _topic( nullptr )
    , _dds_device_dispatcher( 10 )
    , _new_client_handler( [this]( dispatcher::cancellable_timer timer ) {
        // We wait until the new reader callback indicate a new reader has joined or until the
        // active object is stopped
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
                        if( sn_and_handle.second.listener->_new_reader_joined )
                        {
                            if( send_device_info_msg( sn_and_handle.second ) )
                            {
                                sn_and_handle.second.listener->_new_reader_joined = false;
                            }
                        }
                    }
                } );
            }
        }
    } )
{
    if( ! _participant )
        throw std::runtime_error( "dds_device_broadcaster called with a null participant" );
}

bool dds_device_broadcaster::run()
{
    if( _active )
        throw std::runtime_error( "dds_device_broadcaster::run() called when already-active" );

    create_broadcast_topic();

    _dds_device_dispatcher.start();
    _new_client_handler.start();
    _active = true;
    return true;
}

void dds_device_broadcaster::add_device( device_info const & dev_info )
{
    if( ! _active )
        throw std::runtime_error( "dds_device_broadcaster::add_device called before run()" );
    // Post the connected device
    handle_device_changes( {}, { dev_info } );
}

void dds_device_broadcaster::remove_device( device_info const & dev_info )
{
    if( ! _active )
        throw std::runtime_error( "dds_device_broadcaster::remove_device called before run()" );
    // Post the disconnected device
    handle_device_changes( { dev_info.serial }, {} );
}

void dds_device_broadcaster::handle_device_changes(
    const std::vector< std::string > & devices_to_remove,
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
    auto const & handle = it->second;

    // deleting a device also notify the clients internally
    auto ret = _publisher->delete_datawriter( handle.data_writer );
    if( ret != ReturnCode_t::RETCODE_OK )
    {
        LOG_ERROR( "Error code: " << ret() << " while trying to delete data writer ("
                                  << _participant->print( handle.data_writer->guid() ) << ")" );
        return;
    }
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

    // wqos.data_sharing().automatic();
    wqos.data_sharing().off();
    //---------------------------------------------------------------------------------------

    std::shared_ptr< dds_client_listener > writer_listener = std::make_shared< dds_client_listener >( this );

    auto it_inserted = _device_handle_by_sn.emplace(
        dev_info.serial,
        dds_device_handle{ dev_info,
                           _publisher->create_datawriter( _topic, wqos, writer_listener.get() ),
                           writer_listener } );
    assert( it_inserted.second );

    return it_inserted.first->second.data_writer != nullptr;  // TODO: if it can be null, should we insert??
}

void dds_device_broadcaster::create_broadcast_topic()
{
    eprosima::fastdds::dds::TypeSupport topic_type( new topics::device_info::type );
    // Auto fill DDS X-Types TypeObject so other applications (e.g sniffer) can dynamically match a reader for this topic
    topic_type.get()->auto_fill_type_object( true );
    // Don't fill DDS X-Types TypeInformation, it is wasteful if you send TypeObject anyway
    topic_type.get()->auto_fill_type_information( false );
    // Registering the topic type with the participant enables topic instance creation by factory
    DDS_API_CALL( _participant->get()->register_type( topic_type ) );
    _publisher = DDS_API_CALL( _participant->get()->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr ) );
    _topic = DDS_API_CALL( _participant->get()->create_topic( topics::device_info::TOPIC_NAME,
                                                              topic_type->getName(),
                                                              TOPIC_QOS_DEFAULT ) );

    // Topic constructor creates TypeObject that will be sent as part of the discovery phase
    // If this line is removed TypeObject will be sent only after constructing the topic in the first time
    // send_device_info_msg is called (after having a matching reader)
    topics::raw::device_info raw_msg;
}

bool dds_device_broadcaster::send_device_info_msg( const dds_device_handle & handle )
{
    // Publish the device info, but only after a matching reader is found.
    topics::raw::device_info raw_msg;
    fill_device_msg( handle.info, raw_msg );

    // Post a DDS message with the new added device
    if( handle.data_writer->write( &raw_msg ) )
    {
        LOG_DEBUG( "DDS device message sent!" );
        return true;
    }
    else
    {
        LOG_ERROR( "Error writing new device message for device : " << handle.info.serial );
    }
    return false;
}

template< class Array >
void copy_to_array( std::string const & source, Array & dest )
{
    // Workaround for "warning C4996: 'strcpy': This function or variable may be unsafe"
    // Same with strncpy.
    //
    assert( dest.size() > 1 );
    size_t const cch_copied = source.copy( dest.data(), dest.size() - 1 );
    dest.at( cch_copied ) = 0;  // copy() does not append a null character
    // Note that this does not pad the rest of the array with nulls is source was shorter!
    //strncpy( dest.data(), source.c_str(), dest.size() );
    //dest.back() = 0;  // make sure it's 0-terminated
}

void dds_device_broadcaster::fill_device_msg( const device_info & dev_info,
                                              topics::raw::device_info & msg ) const
{
    copy_to_array( dev_info.name, msg.name() );
    copy_to_array( dev_info.serial, msg.serial_number() );
    copy_to_array( dev_info.product_line, msg.product_line() );
    copy_to_array( dev_info.topic_root, msg.topic_root() );
    msg.locked() = dev_info.locked;
}

dds_device_broadcaster::~dds_device_broadcaster()
{
    // Mark this class as inactive and wake up the active object do we can properly stop it.
    _active = false; 
    _new_client_cv.notify_all(); 
    
    _dds_device_dispatcher.stop();
    _new_client_handler.stop();

    for( auto const & sn_and_handle : _device_handle_by_sn )
    {
        auto dev_writer = sn_and_handle.second.data_writer;
        if( dev_writer )
        {
            // TODO why not put this in dtor for handle?
            DDS_API_CALL_NO_THROW( _publisher->delete_datawriter( dev_writer ) );
        }
    }
    _device_handle_by_sn.clear();

    if( _topic != nullptr )
    {
        DDS_API_CALL_NO_THROW( _participant->get()->delete_topic( _topic ) );
    }
    if( _publisher != nullptr )
    {
        DDS_API_CALL_NO_THROW( _participant->get()->delete_publisher( _publisher ) );
    }
}
