// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-participant.h"
#include "dds-utilities.h"

#include "topics/device-info/device-info-msg.h"
#include "topics/notification/notification-msg.h"
#include "topics/control/control-msg.h"

#include <librealsense2/utilities/time/timer.h>

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>

namespace realdds {


namespace {

enum class state_type
{
    WAIT_FOR_DEVICE_HEADER,
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

    size_t _num_of_profiles = 0;
    std::vector< topics::device::notification::video_stream_profile > _video_profiles;
    std::vector< topics::device::notification::motion_stream_profile > _motion_profiles;
    std::unordered_map< size_t, std::string > _profile_to_profile_group;
    std::unordered_map< std::string, std::vector< size_t > > _video_profile_indexes_in_profile_group;
    std::unordered_map< std::string, std::vector< size_t > > _motion_profile_indexes_in_profile_group;
    std::atomic<uint32_t> _control_message_counter = { 0 };

    eprosima::fastdds::dds::Subscriber * _subscriber = nullptr;
    eprosima::fastdds::dds::Publisher  * _publisher = nullptr;
    eprosima::fastdds::dds::DataReader * _notifications_reader = nullptr;
    eprosima::fastdds::dds::DataWriter* _control_writer = nullptr;
    eprosima::fastdds::dds::Topic* _notifications_topic = nullptr;
    eprosima::fastdds::dds::Topic* _control_topic = nullptr;

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
            throw std::runtime_error( "trying to run() a device that's already running" );

        create_notifications_reader();
        create_control_writer();
        if( ! init() )
            throw std::runtime_error( "failed getting sensors data from device: " + _info.topic_root );

        LOG_DEBUG( "Device" << _info.topic_root << " was initialized successfully" );
        _running = true;
    }

    bool write_control_message( void* msg )
    {
        assert( _control_writer != nullptr );

        return DDS_API_CALL( _control_writer->write( msg ) );
    }

private:
    void create_notifications_reader()
    {
        using namespace eprosima::fastdds::dds;

        // Create the subscriber
        if (_subscriber == nullptr)
        {
            _subscriber = DDS_API_CALL( _participant->get()->create_subscriber( SUBSCRIBER_QOS_DEFAULT, nullptr ) );
        }

        // Create and register topic type
        eprosima::fastdds::dds::TypeSupport topic_type( new topics::device::notification::type );
        DDS_API_CALL( topic_type.register_type( _participant->get() ) );

        // Create the topic
        std::string topic_name = topics::device::notification::construct_topic_name( _info.topic_root );
        //TODO - When the last dds_device destruct we should delete the topic
        eprosima::fastdds::dds::Topic* _notifications_topic = DDS_API_CALL( _participant->get()->create_topic( topic_name,
                                                                                                               topic_type->getName(),
                                                                                                               TOPIC_QOS_DEFAULT ) );

        // Create the reader
        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.data_sharing().off();
        rqos.reliability().kind               = RELIABLE_RELIABILITY_QOS;
        rqos.durability().kind                = VOLATILE_DURABILITY_QOS;
        rqos.history().kind                   = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth                  = 10;
        //Does not allocate for every sample but still gives flexibility. See https://github.com/eProsima/Fast-DDS/discussions/2707
        rqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        _notifications_reader = DDS_API_CALL( _subscriber->create_datareader( _notifications_topic, rqos, nullptr ) );

        LOG_DEBUG( "topic '" << topic_name << "' and reader created" );
    }

    void create_control_writer()
    {
        using namespace eprosima::fastdds::dds;

        if (_publisher == nullptr)
        {
            _publisher = DDS_API_CALL( _participant->get()->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr ) );
        }

        // Create and register topic type
        eprosima::fastdds::dds::TypeSupport topic_type( new topics::device::control::type );
        DDS_API_CALL( topic_type.register_type( _participant->get() ) );

        // Create the topic
        std::string topic_name = topics::device::control::construct_topic_name( _info.topic_root );
        //TODO - When the last dds_device destruct we should delete the topic
        eprosima::fastdds::dds::Topic* _control_topic = DDS_API_CALL( _participant->get()->create_topic( topic_name,
                                                                                                         topic_type->getName(),
                                                                                                         TOPIC_QOS_DEFAULT ) );

        // Create the writer
        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.data_sharing().off();
        wqos.reliability().kind               = RELIABLE_RELIABILITY_QOS;
        wqos.durability().kind                = VOLATILE_DURABILITY_QOS;
        wqos.history().kind                   = KEEP_LAST_HISTORY_QOS;
        wqos.history().depth                  = 10;
        wqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        _control_writer = DDS_API_CALL( _publisher->create_datawriter( _control_topic, wqos, nullptr ) );

        LOG_DEBUG( "topic '" << topic_name << "' and writer created" );
    }

    bool init()
    {
        // We expect to receive all of the sensors data under a timeout
        utilities::time::timer t( std::chrono::seconds( 30 ) );  // TODO: refine time out
        state_type state = state_type::WAIT_FOR_DEVICE_HEADER;

        while( ! t.has_expired() && state_type::DONE != state )
        {
            eprosima::fastrtps::Duration_t one_second = { 1, 0 };
            if( _notifications_reader->wait_for_unread_message( one_second ) )
            {
                topics::raw::device::notification raw_data;
                eprosima::fastdds::dds::SampleInfo info;
                // Process all the samples until no one is returned,
                // We will distinguish info change vs new data by validating using `valid_data` field
                while( ReturnCode_t::RETCODE_OK == _notifications_reader->take_next_sample( &raw_data, &info ) )
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
                                _num_of_profiles = data.get< topics::device::notification::device_header_msg >()->num_of_streams;
                                LOG_INFO( "got DEVICE_HEADER message with " << _num_of_profiles << " streams" );
                                state = state_type::WAIT_FOR_PROFILES;
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'DEVICE_HEADER' when the state is: " << state );
                            break;

                        case topics::device::notification::msg_type::VIDEO_STREAM_PROFILES:
                            if( state_type::WAIT_FOR_PROFILES == state )
                            {
                                LOG_INFO( "got VIDEO_STREAM_PROFILES message" );

                                auto profiles_msg = data.get< topics::device::notification::video_stream_profiles_msg >();

                                for ( size_t i = 0; i < profiles_msg->num_of_profiles; ++i )
                                {
                                    _video_profiles.push_back( profiles_msg->profiles[i] );
                                    _profile_to_profile_group[profiles_msg->profiles[i].uid] = profiles_msg->group_name;
                                    _video_profile_indexes_in_profile_group[profiles_msg->group_name].push_back( profiles_msg->profiles[i].uid );
                                }

                                size_t received_profiles_num = _video_profiles.size() + _motion_profiles.size();
                                if (received_profiles_num >= _num_of_profiles )
                                {
                                    state = state_type::DONE;
                                    if (received_profiles_num > _num_of_profiles)
                                    {
                                        LOG_INFO( "Received more streams (" << received_profiles_num
                                                  << ") than in device header (" << _num_of_profiles << ")" );
                                    }
                                }
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'VIDEO_STREAM_PROFILES' when the state is: "
                                           << state );
                            break;
                        case topics::device::notification::msg_type::MOTION_STREAM_PROFILES:
                            if( state_type::WAIT_FOR_PROFILES == state )
                            {
                                LOG_INFO( "got MOTION_STREAM_PROFILES message" );

                                auto profiles_msg = data.get< topics::device::notification::motion_stream_profiles_msg >();

                                for (size_t i = 0; i < profiles_msg->num_of_profiles; ++i)
                                {
                                    _motion_profiles.push_back( profiles_msg->profiles[i] );
                                    _profile_to_profile_group[profiles_msg->profiles[i].uid] = profiles_msg->group_name;
                                    _motion_profile_indexes_in_profile_group[profiles_msg->group_name].push_back( profiles_msg->profiles[i].uid );
                                }

                                size_t received_profiles_num = _video_profiles.size() + _motion_profiles.size();
                                if (received_profiles_num >= _num_of_profiles)
                                {
                                    state = state_type::DONE;
                                    if (received_profiles_num > _num_of_profiles)
                                    {
                                        LOG_INFO( "Received more streams (" << received_profiles_num
                                                  << ") than in device header (" << _num_of_profiles << ")" );
                                    }
                                }
                            }
                            else
                                LOG_ERROR( "Wrong message received, got 'MOTION_STREAM_PROFILES' when the state is: "
                                           << state );
                            break;
                        default:
                            LOG_ERROR( "Wrong message received, got " << static_cast<uint16_t>(msg_id)
                                       << " when the state is: " << state );
                            break;
                        }
                    }
                }
            }
        }

        return ( state_type::DONE == state );
    }
}; //class dds_device::impl


}  // namespace realdds
