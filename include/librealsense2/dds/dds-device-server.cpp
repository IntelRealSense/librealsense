// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "dds-device-server.h"
#include "dds-participant.h"
#include "dds-utilities.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <librealsense2/dds/topics/dds-topics.h>
#include <librealsense2/utilities/concurrency/concurrency.h>

using namespace eprosima::fastdds::dds;
using namespace librealsense::dds;

//-----------------------------------------------------------------------------------------------------//
// DDS stream server is in charge of distributing the relevant stream frames into it's dedicated topic
//-----------------------------------------------------------------------------------------------------//
class dds_device_server::dds_stream_server
{
public:
    dds_stream_server( eprosima::fastdds::dds::DomainParticipant * participant,
                       eprosima::fastdds::dds::Publisher * publisher,
                       const std::string & topic_root,
                       const std::string & stream_name )
        : _participant( participant )
        , _publisher( publisher )
        , _topic( nullptr )
        , _data_writer( nullptr )
    {
        _topic_name = librealsense::dds::topics::device::image::construct_stream_topic_name( topic_root,
                                                                                     stream_name );

        eprosima::fastdds::dds::TypeSupport topic_type( new librealsense::dds::topics::device::image::type );

        DDS_API_CALL( _participant->register_type( topic_type ) );
        _topic = DDS_API_CALL( _participant->create_topic( _topic_name, topic_type->getName(), TOPIC_QOS_DEFAULT ) );

        // TODO:: Maybe we want to open a writer only when the stream is requested?
        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.data_sharing().off();
        wqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        wqos.durability().kind = VOLATILE_DURABILITY_QOS;
        wqos.publish_mode().kind = SYNCHRONOUS_PUBLISH_MODE;
        // Our message has dynamic size (unbounded) so we need a memory policy that supports it
        wqos.endpoint().history_memory_policy
            = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        _data_writer = DDS_API_CALL( _publisher->create_datawriter( _topic, wqos ) );
    }
    ~dds_stream_server()
    {
        if( nullptr != _data_writer )
        {
            DDS_API_CALL_NO_THROW( _publisher->delete_datawriter( _data_writer ) );
        }

        if( nullptr != _topic )
        {
            DDS_API_CALL_NO_THROW( _participant->delete_topic( _topic ) );
        }
    }


    void publish_image( const uint8_t * data, size_t size )
    {
        LOG_DEBUG( "publishing a DDS video frame for topic: " << _topic_name );
        librealsense::dds::topics::raw::device::image raw_image;
        raw_image.size() = static_cast< uint32_t >( size );
        raw_image.format() = _image_header.format;
        raw_image.height() = _image_header.height;
        raw_image.width() = _image_header.width;
        raw_image.raw_data().assign( data, data + size );

        DDS_API_CALL( _data_writer->write( &raw_image ) );
    }
    void set_image_header( const image_header & header ) { _image_header = header; }

private:
    std::string _topic_name;
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataWriter * _data_writer;
    image_header _image_header;
};

//------------------------------------------------------------------------------------------------------------//
// DDS notifications server is in charge of publishing notifications and info like streams, profiles, events
//------------------------------------------------------------------------------------------------------------//
// We want to know when readers join our topic
class dds_device_server::dds_notifications_server
{
public:
    // We want to know when readers join our topic
    class dds_notifications_client_listener : public eprosima::fastdds::dds::DataWriterListener
    {
    public:
        dds_notifications_client_listener( dds_notifications_server * owner )
            : eprosima::fastdds::dds::DataWriterListener()
            , _owner( owner )
        {
        }

        void on_publication_matched(
            eprosima::fastdds::dds::DataWriter * writer,
            const eprosima::fastdds::dds::PublicationMatchedStatus & info ) override
        {
            if( info.current_count_change == 1 )
            {
                LOG_DEBUG( "DataReader " << writer->guid() << " discovered" );
                {
                    {
                        std::lock_guard< std::mutex > lock( _owner->_notification_send_mutex );
                        _owner->_new_reader_joined = true;
                    }
                    _owner->_send_notification_cv.notify_all();
                }
            }
            else if( info.current_count_change == -1 )
            {
                LOG_DEBUG( "DataReader " << writer->guid() << " disappeared" );
            }
            else
            {
                LOG_ERROR( std::to_string( info.current_count_change )
                           << " is not a valid value for on_publication_matched" );
            }
        }

    private:
        dds_notifications_server * _owner;
    };

    dds_notifications_server( eprosima::fastdds::dds::DomainParticipant * participant,
                              eprosima::fastdds::dds::Publisher * publisher,
                              const std::string & topic_root )
        : _participant( participant )
        , _publisher( publisher )
        , _topic( nullptr )
        , _data_writer( nullptr )
        , _clients_listener( this )
        , _notifications_loop( [this]( dispatcher::cancellable_timer timer ) {
            if( _active )
            {
                std::unique_lock< std::mutex > lock( _notification_send_mutex );
                _send_notification_cv.wait( lock, [this]() {
                    return ! _active || _new_reader_joined || _new_notification;
                } );

                if( _new_reader_joined )
                {
                    // Send all required data
                    for( auto notification : _new_reader_notifications )
                    {
                        DDS_API_CALL( _data_writer->write( &notification ) );
                    }
                    _new_reader_joined = false;
                }

                if( _new_notification )
                {
                    topics::raw::device::notifications msg;
                    while( _notifications_msg_queue.dequeue( &msg, 1000 ) )
                    {
                        DDS_API_CALL( _data_writer->write( &msg ) );
                    }
                    _new_notification = false;
                }
            }
        } )
    {
        _topic_name
            = librealsense::dds::topics::device::notifications::construct_notifications_topic_name(
                topic_root );

        eprosima::fastdds::dds::TypeSupport topic_type(
            new librealsense::dds::topics::device::notifications::type );

        DDS_API_CALL( _participant->register_type( topic_type ) );
        _topic = DDS_API_CALL(
            _participant->create_topic( _topic_name, topic_type->getName(), TOPIC_QOS_DEFAULT ) );

        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.data_sharing().off();
        wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        wqos.publish_mode().kind = SYNCHRONOUS_PUBLISH_MODE;
        _data_writer = DDS_API_CALL( _publisher->create_datawriter( _topic, wqos, &_clients_listener ) );
        _notifications_loop.start();
        _active = true;
    }

    ~dds_notifications_server()
    {
        // Mark this class as inactive and wake up the active object do we can properly stop it.
        _active = false;
        _send_notification_cv.notify_all();
        _notifications_loop.stop();

        if( nullptr != _data_writer )
        {
            DDS_API_CALL_NO_THROW( _publisher->delete_datawriter( _data_writer ) );
        }

        if( nullptr != _topic )
        {
            DDS_API_CALL_NO_THROW( _participant->delete_topic( _topic ) );
        }
    };

    void send_notifications( const topics::raw::device::notifications& msg, bool notification_for_new_reader = false) 
    {
        std::unique_lock< std::mutex > lock( _notification_send_mutex );
        topics::raw::device::notifications msg_to_move( msg );
        if( notification_for_new_reader )
            _new_reader_notifications.push_back( std::move( msg_to_move ) );
        else
        {
            if( !_notifications_msg_queue.enqueue( std::move( msg_to_move )))
            {
                LOG_ERROR("error while trying to enqueue a message id:" << msg_to_move.id() << " to _notifications_msg_queue");
            }
        }

    };

private:
    std::string _topic_name;
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataWriter * _data_writer;
    dds_notifications_client_listener _clients_listener;
    active_object<> _notifications_loop;
    single_consumer_queue< topics::raw::device::notifications > _notifications_msg_queue;
    std::vector<topics::raw::device::notifications> _new_reader_notifications;
    std::mutex _notification_send_mutex;
    std::condition_variable _send_notification_cv;
    std::atomic_bool _new_reader_joined = { false };  // Used to indicate that a new reader has joined for the notifications writer
    std::atomic_bool _new_notification = { false };  
    std::atomic_bool _active = { false };
};

//-----------------------------------------------------------------------------------------------------//
// DDS device server is in charge of handling the device control, streams and notifications
//-----------------------------------------------------------------------------------------------------//
dds_device_server::dds_device_server( dds_participant & participant,
                                      const std::string & topic_root )
    : _participant( participant.get() )
    , _publisher( nullptr )
    , _topic_root( topic_root )
{
    LOG_DEBUG( "DDS device server for device topic root: " << _topic_root << " created" );
}

dds_device_server::~dds_device_server()
{
    // Release resources
    if( nullptr != _participant )
    {
        if( nullptr != _publisher )
        {
            _participant->delete_publisher( _publisher );
        }
    }

    _stream_name_to_server.clear();
    LOG_DEBUG( "DDS device server for device topic root: " << _topic_root << " deleted" );
}

void dds_device_server::set_image_header( const std::string & stream_name,
                                          const image_header & header )
{
    _stream_name_to_server.at( stream_name )->set_image_header( header );
}

void dds_device_server::publish_image( const std::string & stream_name, const uint8_t * data, size_t size )
{
    if( !is_valid() )
    {
        throw std::runtime_error( "Cannot publish '" + stream_name + "' frame for '" + _topic_root
                                  + "', DDS device server in uninitialized" );
    }

    _stream_name_to_server.at( stream_name )->publish_image( data, size );
}

void dds_device_server::init( const std::vector<std::string> &supported_streams_names )
{
    if( is_valid() )
    {
        throw std::runtime_error( "device server '" + _topic_root + "' is already initialized" );
    }

    _publisher = DDS_API_CALL(_participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr ));

    // Create a notifications server
    _dds_notifications_server = std::make_shared< dds_notifications_server >( _participant, _publisher, _topic_root );

    // Create streams servers per each supporting stream of the device
    for( auto stream_name : supported_streams_names )
    {
        _stream_name_to_server[stream_name] = std::make_shared< dds_stream_server >( _participant,
                                                                                     _publisher,
                                                                                     _topic_root,
                                                                                     stream_name );
    }
}

void dds_device_server::publish_notifications( const topics::raw::device::notifications& notifications_msg, bool new_reader_notification )
{
    _dds_notifications_server->send_notifications( notifications_msg, new_reader_notification );
}

