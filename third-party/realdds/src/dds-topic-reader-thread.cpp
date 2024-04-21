// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include <fastdds/dds/core/condition/WaitSet.hpp>

namespace realdds {


dds_topic_reader_thread::dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic )
    : dds_topic_reader_thread( topic, std::make_shared< dds_subscriber >( topic->get_participant() ) )
{
}


dds_topic_reader_thread::dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic,
                                                  std::shared_ptr< dds_subscriber > const & subscriber )
    : super( topic, subscriber )
{
}


dds_topic_reader_thread::~dds_topic_reader_thread()
{
    stop();  // Make sure thread is stopped!
}


void dds_topic_reader_thread::run( qos const & rqos )
{
    if( ! _on_data_available )
        DDS_THROW( runtime_error, "on-data-available must be provided" );

    _reader = DDS_API_CALL( _subscriber->get()->create_datareader( _topic->get(), rqos ) );
    
    _th = std::thread(
        [this, name = _topic->get()->get_name()]()
        {
            eprosima::fastdds::dds::WaitSet wait_set;
            auto & condition = _reader->get_statuscondition();
            condition.set_enabled_statuses( eprosima::fastdds::dds::StatusMask::data_available()
                                            << eprosima::fastdds::dds::StatusMask::subscription_matched()
                                            << eprosima::fastdds::dds::StatusMask::sample_lost() );
            wait_set.attach_condition( condition );

            wait_set.attach_condition( _stopped );

            while( ! _stopped.get_trigger_value() )
            {
                eprosima::fastdds::dds::ConditionSeq active_conditions;
                wait_set.wait( active_conditions, eprosima::fastrtps::c_TimeInfinite );

                if( _stopped.get_trigger_value() )
                    break;

                auto & changed = _reader->get_status_changes();
                if( changed.is_active( eprosima::fastdds::dds::StatusMask::sample_lost() ) )
                {
                    eprosima::fastdds::dds::SampleLostStatus status;
                    _reader->get_sample_lost_status( status );
                    on_sample_lost( _reader, status );
                }
                if( changed.is_active( eprosima::fastdds::dds::StatusMask::data_available() ) )
                {
                    on_data_available( _reader );
                }
                if( changed.is_active( eprosima::fastdds::dds::StatusMask::subscription_matched() ) )
                {
                    eprosima::fastdds::dds::SubscriptionMatchedStatus status;
                    _reader->get_subscription_matched_status( status );
                    on_subscription_matched( _reader, status );
                }
            }
        } );
}


void dds_topic_reader_thread::stop()
{
    if( _th.joinable() )
    {
        _stopped.set_trigger_value( true );
        _th.join();
    }
    super::stop();
}


}  // namespace realdds
