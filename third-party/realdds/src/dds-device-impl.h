// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-participant.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-utilities.h>

#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/notification/notification-msg.h>
#include <realdds/topics/control/control-msg.h>

#include <librealsense2/utilities/time/timer.h>

#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>

namespace realdds {


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
    std::shared_ptr< dds_participant > const _participant;

    bool _running = false;

    size_t _num_of_sensors = 0;
    std::unordered_map< size_t, std::string > _sensor_index_to_name;
    std::unordered_map< size_t, topics::device::notification::video_stream_profiles_msg > _sensor_to_video_profiles;
    std::unordered_map< size_t, topics::device::notification::motion_stream_profiles_msg > _sensor_to_motion_profiles;
    std::atomic<uint32_t> _control_message_counter = { 0 };

    std::shared_ptr< dds_topic_reader > _notifications_reader;
    std::shared_ptr< dds_topic_writer > _control_writer;

    impl( std::shared_ptr< dds_participant > const & participant,
          dds_guid const & guid,
          topics::device_info const & info )
        : _info( info )
        , _guid( guid )
        , _participant( participant )
    {
    }

    void run()
    {
        if( _running )
            DDS_THROW( runtime_error, "trying to run() a device that's already running" );

        create_notifications_reader();
        create_control_writer();
        if( ! init() )
            DDS_THROW( runtime_error, "failed getting stream data from '" + _info.topic_root + "'" );

        LOG_DEBUG( "device '" << _info.topic_root << "' initialized successfully" );
        _running = true;
    }

    bool write_control_message( void * msg )
    {
        assert( _control_writer != nullptr );

        return DDS_API_CALL( _control_writer->get()->write( msg ) );
    }

private:
    void create_notifications_reader()
    {
        if( _notifications_reader )
            return;

        auto topic = topics::device::notification::create_topic( _participant, _info.topic_root + "/notification" );

        _notifications_reader = std::make_shared< dds_topic_reader >( topic );
        _notifications_reader->run( dds_topic_reader::reader_qos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS ) );
    }

    void create_control_writer()
    {
        if( _control_writer )
            return;

        auto topic = topics::device::control::create_topic( _participant, _info.topic_root + "/control" );
        _control_writer = std::make_shared< dds_topic_writer >( topic );
        dds_topic_writer::writer_qos wqos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS );
        wqos.history().depth = 10;  // default is 1
        _control_writer->run( wqos );
    }

    bool init()
    {
        // We expect to receive all of the sensors data under a timeout
        utilities::time::timer t( std::chrono::seconds( 30 ) );  // TODO: refine time out
        state_type state = state_type::WAIT_FOR_DEVICE_HEADER;

        while( ! t.has_expired() && state_type::DONE != state )
        {
            eprosima::fastrtps::Duration_t one_second = { 1, 0 };
            if( _notifications_reader->get()->wait_for_unread_message( one_second ) )
            {
                topics::device::notification data;
                eprosima::fastdds::dds::SampleInfo info;
                while( topics::device::notification::take_next( *_notifications_reader, &data, &info ) )
                {
                    if( ! data.is_valid() )
                        continue;

                    switch( data._msg_type )
                    {
                    case topics::device::notification::msg_type::DEVICE_HEADER:
                        if( state_type::WAIT_FOR_DEVICE_HEADER == state )
                        {
                            auto device_header = data.get< topics::device::notification::device_header_msg >();
                            if( ! device_header )
                            {
                                LOG_ERROR( "Got DEVICE_HEADER with no data" );
                                break;
                            }
                            _num_of_sensors = device_header->num_of_sensors;
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
                            if( ! sensor_header )
                            {
                                LOG_ERROR( "Got SENSOR_HEADER with no data" );
                                break;
                            }
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
                            if( ! video_stream_profiles )
                            {
                                LOG_ERROR( "Got VIDEO_STREAM_PROFILES with no data" );
                                break;
                            }
                            LOG_INFO( "got VIDEO_STREAM_PROFILES message" );

                            // TODO: Find a way to remove the "emplace" profiles copy
                            topics::device::notification::video_stream_profiles_msg msg;
                            msg.dds_sensor_index = video_stream_profiles->dds_sensor_index;
                            msg.num_of_profiles = video_stream_profiles->num_of_profiles;
                            for( int i = 0; i < video_stream_profiles->num_of_profiles; ++i )
                                msg.profiles[i] = video_stream_profiles->profiles[i];

                            auto sensor_name_it = _sensor_index_to_name.find( video_stream_profiles->dds_sensor_index );
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
                            LOG_ERROR(
                                "Wrong message received, got 'VIDEO_STREAM_PROFILES' when the state is: " << state );
                        break;

                    case topics::device::notification::msg_type::MOTION_STREAM_PROFILES:
                        if( state_type::WAIT_FOR_PROFILES == state )
                        {
                            auto motion_stream_profiles
                                = data.get< topics::device::notification::motion_stream_profiles_msg >();
                            if( ! motion_stream_profiles )
                            {
                                LOG_ERROR( "Got MOTION_STREAM_PROFILES with no data" );
                                break;
                            }
                            LOG_INFO( "got MOTION_STREAM_PROFILES message" );

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
                            LOG_ERROR(
                                "Wrong message received, got 'MOTION_STREAM_PROFILES' when the state is: " << state );
                        break;

                    default:
                        LOG_ERROR( "Wrong message received, got " << (int)data._msg_type << " when the state is: " << state );
                        break;
                    }
                }
            }
        }
        return ( state_type::DONE == state );
    }
};


}  // namespace realdds
