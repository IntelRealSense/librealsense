// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-utilities.h>

#include <rsutils/time/stopwatch.h>

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

    eprosima::fastdds::dds::StatusMask status_mask;
    status_mask << eprosima::fastdds::dds::StatusMask::subscription_matched();
    //status_mask << eprosima::fastdds::dds::StatusMask::data_available();
    _reader = DDS_API_CALL( _subscriber->get()->create_datareader( _topic->get(), rqos, this, status_mask ) );
    
    _th = std::thread(
        [this, name = _topic->get()->get_name()]()
        {
            eprosima::fastdds::dds::WaitSet wait_set;
            auto & condition = _reader->get_statuscondition();
            condition.set_enabled_statuses( eprosima::fastdds::dds::StatusMask::data_available() );
            wait_set.attach_condition( condition );

            wait_set.attach_condition( _stopped );

            while( ! _stopped.get_trigger_value() )
            {
                eprosima::fastdds::dds::ConditionSeq active_conditions;
                wait_set.wait( active_conditions, eprosima::fastrtps::c_TimeInfinite );

                if( _stopped.get_trigger_value() )
                    break;

                rsutils::time::stopwatch stopwatch;
                _on_data_available();
                if( stopwatch.get_elapsed() > std::chrono::milliseconds( 500 ) )
                    LOG_WARNING( "<---- '" << name << "' callback took too long!" );
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
