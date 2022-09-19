// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-participant.h"
#include "dds-utilities.h"

#include "topics/device-info/device-info-msg.h"
#include "topics/notification/notification-msg.h"

#include <librealsense2/utilities/time/timer.h>

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>

namespace librealsense {
namespace dds {


namespace {


enum class state_type
{
    WAIT_FOR_DEVICE_HEADER,
    WAIT_FOR_SENSOR_HEADER,
    WAIT_FOR_PROFILES,
    DONE
};

std::ostream & operator<<( std::ostream & s, state_type st )
{
    switch( st )
    {
    case state_type::WAIT_FOR_DEVICE_HEADER:
        s << "WAIT_FOR_DEVICE_HEADER";
        break;
    case state_type::WAIT_FOR_SENSOR_HEADER:
        s << "WAIT_FOR_SENSOR_HEADER";
        break;
    case state_type::WAIT_FOR_PROFILES:
        s << "WAIT_FOR_PROFILES";
        break;
    case state_type::DONE:
        s << "DONE";
        break;
    default:
        s << "UNKNOWN";
        break;
    }
    return s;
}


}  // namespace

class dds_device::impl
{
public:
    topics::device_info const _info;
    dds_guid const _guid;
    std::shared_ptr< dds::dds_participant > const _participant;

    bool _running = false;

    size_t _num_of_sensors = 0;
    std::unordered_map< int, std::string > _sensor_index_to_name;
    std::unordered_map< int, topics::device::notification::video_stream_profiles_msg > _sensor_to_video_profiles;
    std::unordered_map< int, topics::device::notification::motion_stream_profiles_msg > _sensor_to_motion_profiles;

    eprosima::fastdds::dds::Subscriber* _subscriber;
    eprosima::fastdds::dds::Topic* _topic;
    eprosima::fastdds::dds::DataReader* _reader;

    impl( std::shared_ptr< dds::dds_participant > const& participant,
        dds::dds_guid const& guid,
        dds::topics::device_info const& info )
        : _info( info )
        , _guid( guid )
        , _participant( participant )
    {
    }

    void run()
    {
        if( _running )
            throw std::runtime_error( "trying to run() a device that's already running" );

        create_notifications_reader();
        if( ! init() )
            throw std::runtime_error( "failed getting sensors data from device: " + _info.topic_root );

        LOG_DEBUG( "Device" << _info.topic_root << " was initialized successfully" );
        _running = true;
    }

private:
    void create_notifications_reader()
    {
        using namespace eprosima::fastdds::dds;

        // CREATE THE SUBSCRIBER
        _subscriber = DDS_API_CALL( _participant->get()->create_subscriber( SUBSCRIBER_QOS_DEFAULT, nullptr ) );
        if( _subscriber == nullptr )
        {
            throw std::runtime_error( "Error creating a DDS subscriber" );
        }

        // CREATE TOPIC TYPE
        eprosima::fastdds::dds::TypeSupport topic_type( new librealsense::dds::topics::device::notification::type );

        // REGISTER THE TYPE
        DDS_API_CALL( topic_type.register_type( _participant->get() ) );


        std::string topic_name
            = librealsense::dds::topics::device::notification::construct_topic_name( _info.topic_root );

        // CREATE THE TOPIC
        _topic
            = DDS_API_CALL( _participant->get()->create_topic( topic_name, topic_type->getName(), TOPIC_QOS_DEFAULT ) );

        // CREATE THE READER
        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.data_sharing().off();
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.durability().kind = VOLATILE_DURABILITY_QOS;
        rqos.history().kind = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth = 10;
        rqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
        _reader = DDS_API_CALL( _subscriber->create_datareader( _topic, rqos, nullptr ) );
        if( _reader == nullptr )
        {
            throw std::runtime_error( "Error creating a DDS reader for " + topic_name );
        }

        LOG_DEBUG( "topic '" << topic_name << "' created" );
    }

    bool init()
    {
        // We expect to receive all of the sensors data under a timeout
        utilities::time::timer t( std::chrono::seconds( 30 ) );  // TODO: refine time out
        state_type state = state_type::WAIT_FOR_DEVICE_HEADER;

        while( ! t.has_expired() && state_type::DONE != state )
        {
            eprosima::fastrtps::Duration_t one_second = { 1, 0 };
            if( _reader->wait_for_unread_message( one_second ) )
            {
                dds::topics::raw::device::notification raw_data;
                eprosima::fastdds::dds::SampleInfo info;
                // Process all the samples until no one is returned,
                // We will distinguish info change vs new data by validating using `valid_data` field
                while( ReturnCode_t::RETCODE_OK == _reader->take_next_sample( &raw_data, &info ) )
                {
                    // Only samples for which valid_data is true should be accessed
                    // valid_data indicates that the `take` return an updated sample
                    if( info.valid_data )
                    {
                        topics::device::notification data = raw_data;
                        auto msg_id = static_cast< topics::device::notification::msg_type >( raw_data.id() );
                        switch( msg_id )
                        {
                        case topics::device::notification::msg_type::DEVICE_HEADER:
                            if( state_type::WAIT_FOR_DEVICE_HEADER == state )
                            {
                                _num_of_sensors
                                    = data.get< topics::device::notification::device_header_msg >()->num_of_sensors;
                                LOG_INFO( "got DEVICE_HEADER message with " << _num_of_sensors << " sensors" );
                                state = state_type::WAIT_FOR_SENSOR_HEADER;
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'DEVICE_HEADER' when the state is: " << state );
                            break;
                        case topics::device::notification::msg_type::SENSOR_HEADER:

                            if( state_type::WAIT_FOR_SENSOR_HEADER == state )
                            {
                                auto sensor_header = data.get< topics::device::notification::sensor_header_msg >();
                                LOG_INFO(
                                    "got SENSOR_HEADER message for sensor: "
                                    << sensor_header->name << " of type: "
                                    << ( sensor_header->type
                                                 == topics::device::notification::sensor_header_msg::sensor_type::VIDEO
                                             ? "VIDEO_SENSOR"
                                             : "MOTION_SENSOR" ) );

                                _sensor_index_to_name.emplace( sensor_header->index, sensor_header->name );
                                state = state_type::WAIT_FOR_PROFILES;
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'SENSOR_HEADER' when the state is: " << state );

                            break;

                        case topics::device::notification::msg_type::VIDEO_STREAM_PROFILES:
                            if( state_type::WAIT_FOR_PROFILES == state )
                            {
                                auto video_stream_profiles
                                    = data.get< topics::device::notification::video_stream_profiles_msg >();

                                LOG_INFO( "got VIDEO_STREAM_PROFILES message" );

                                // TODO: Find a way to remove the "emplace" profiles copy
                                topics::device::notification::video_stream_profiles_msg msg;
                                msg.dds_sensor_index = video_stream_profiles->dds_sensor_index;
                                msg.num_of_profiles = video_stream_profiles->num_of_profiles;
                                for( int i = 0; i < video_stream_profiles->num_of_profiles; ++i )
                                    msg.profiles[i] = video_stream_profiles->profiles[i];

                                auto sensor_name_it
                                    = _sensor_index_to_name.find( video_stream_profiles->dds_sensor_index );
                                if( sensor_name_it != _sensor_index_to_name.end() )
                                {
                                    _sensor_to_video_profiles.emplace( video_stream_profiles->dds_sensor_index,
                                                                       std::move( msg ) );

                                    if( _sensor_to_video_profiles.size() + _sensor_to_motion_profiles.size()
                                        < _num_of_sensors )
                                        state = state_type::WAIT_FOR_SENSOR_HEADER;
                                    else
                                        state = state_type::DONE;
                                }
                                else
                                    LOG_ERROR( "Error on video profiles message, DDS sensor with index: "
                                               << video_stream_profiles->dds_sensor_index << " could not be found " );
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'VIDEO_STREAM_PROFILES' when the state is: "
                                           << state );
                            break;
                        case topics::device::notification::msg_type::MOTION_STREAM_PROFILES:
                            if( state_type::WAIT_FOR_PROFILES == state )
                            {
                                LOG_INFO( "got MOTION_STREAM_PROFILES message" );
                                auto motion_stream_profiles
                                    = data.get< topics::device::notification::motion_stream_profiles_msg >();

                                // TODO: Try to save the "emplace" profiles copy
                                topics::device::notification::motion_stream_profiles_msg msg;
                                msg.dds_sensor_index = motion_stream_profiles->dds_sensor_index;
                                msg.num_of_profiles = motion_stream_profiles->num_of_profiles;
                                for( int i = 0; i < motion_stream_profiles->num_of_profiles; ++i )
                                    msg.profiles[i] = motion_stream_profiles->profiles[i];

                                auto sensor_name_it
                                    = _sensor_index_to_name.find( motion_stream_profiles->dds_sensor_index );
                                if( sensor_name_it != _sensor_index_to_name.end() )
                                {
                                    _sensor_to_motion_profiles.emplace( motion_stream_profiles->dds_sensor_index,
                                                                        std::move( msg ) );

                                    if( _sensor_to_video_profiles.size() + _sensor_to_motion_profiles.size()
                                        < _num_of_sensors )
                                        state = state_type::WAIT_FOR_SENSOR_HEADER;
                                    else
                                        state = state_type::DONE;
                                }
                                else
                                    LOG_ERROR( "Error on motion profiles message, DDS sensor with index: "
                                               << motion_stream_profiles->dds_sensor_index << " could not be found " );
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'MOTION_STREAM_PROFILES' when the state is: "
                                           << state );
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
        return ( state_type::DONE == state );
    }
};


}  // namespace dds
}  // namespace librealsense
