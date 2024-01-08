// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/flexible/flexible-reader.h>

#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <rsutils/json.h>


namespace realdds {
namespace topics {


flexible_reader::flexible_reader( std::shared_ptr< dds_topic > const & topic )
    : _reader( std::make_shared< dds_topic_reader >( topic ) )
{
    _reader->on_subscription_matched( [this]( eprosima::fastdds::dds::SubscriptionMatchedStatus const & status ) {
        this->on_subscription_matched( status );
    } );
    _reader->on_data_available( [this]() { this->on_data_available(); } );
    _reader->run( dds_topic_reader::qos() );
}


std::string const & flexible_reader::name() const
{
    return _reader->topic()->get()->get_name();
}


void flexible_reader::wait_for_data()
{
    std::unique_lock< std::mutex > lock( _data_mutex );
    _data_cv.wait( lock );
}


void flexible_reader::wait_for_data( std::chrono::seconds timeout )
{
    std::unique_lock< std::mutex > lock( _data_mutex );
    if( std::cv_status::timeout == _data_cv.wait_for( lock, timeout ) )
        DDS_THROW( runtime_error, "timed out waiting for data" );
}


flexible_reader::data_t flexible_reader::read()
{
    wait_for_data();
    data_t data;
    {
        std::lock_guard< std::mutex > lock( _data_mutex );
        data = std::move( _data.front() );
        _data.pop();
    }
    LOG_DEBUG( name() << ".read_sample @" << timestr( now(), time_from( data.sample.reception_timestamp ) ) );
    return data;
}

flexible_reader::data_t flexible_reader::read( std::chrono::seconds timeout )
{
    wait_for_data( timeout );
    data_t data;
    {
        std::lock_guard< std::mutex > lock( _data_mutex );
        data = std::move( _data.front() );
        _data.pop();
    }
    LOG_DEBUG( name() << ".read_sample @" << timestr( now(), time_from( data.sample.reception_timestamp ) ) );
    return data;
}


void flexible_reader::on_subscription_matched( eprosima::fastdds::dds::SubscriptionMatchedStatus const & status )
{
    _n_writers += status.current_count_change;
    LOG_DEBUG( name() << ".on_subscription_matched " << ( status.current_count_change > 0 ? "+" : "" )
                      << status.current_count_change << " -> " << _n_writers );
}


void flexible_reader::on_data_available()
{
    auto notify_time = now();
    bool got_something = false;
    while( 1 )
    {
        data_t data;
        if( ! flexible_msg::take_next( *_reader, &data.msg, &data.sample ) )
            assert( ! data.msg.is_valid() );
        if( ! data.msg.is_valid() )
        {
            if( ! got_something )
                DDS_THROW( runtime_error, "expected message not received!" );
            break;
        }
        auto received = time_from( data.sample.reception_timestamp );
        LOG_DEBUG( name() << ".on_data_available @" << timestr( received.to_ns(), timestr::no_suffix )
                          << timestr( notify_time, received, timestr::no_suffix ) << timestr( now(), notify_time ) << " "
                          << data.msg.json_data().dump() );
        got_something = true;
        {
            std::lock_guard< std::mutex > lock( _data_mutex );
            _data.emplace( std::move( data ) );
        }
        _data_cv.notify_one();
    }
}


}  // namespace topics
}  // namespace realdds
