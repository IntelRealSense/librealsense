// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/flexible/flexible-writer.h>

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <rsutils/time/timer.h>
#include <rsutils/json.h>


namespace realdds {
namespace topics {


flexible_writer::flexible_writer( std::shared_ptr< dds_topic > const & topic )
    : _writer( std::make_shared< dds_topic_writer >( topic ) )
{
    _writer->on_publication_matched( [this]( eprosima::fastdds::dds::PublicationMatchedStatus const & status ) {
        this->on_publication_matched( status );
    } );
    _writer->run();
}


std::string const & flexible_writer::name() const
{
    return _writer->topic()->get()->get_name();
}


void flexible_writer::wait_for_readers( int n_readers, std::chrono::seconds timeout )
{
    rsutils::time::timer timer( timeout );
    while( _n_readers < n_readers )
    {
        if( timer.has_expired() )
            DDS_THROW( runtime_error, "timed out waiting for " + std::to_string( n_readers ) + " readers" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
}


void flexible_writer::write( rsutils::json && json )
{
    std::string json_string = json.dump();
    flexible_msg msg( std::move( json ) );
    auto write_time = now();
    std::move( msg ).write_to( *_writer );
    LOG_DEBUG( name() << ".write JSON " << json_string << " @" << timestr( write_time, timestr::no_suffix )
                      << timestr( now(), write_time ) );
}


void flexible_writer::on_publication_matched( eprosima::fastdds::dds::PublicationMatchedStatus const & status )
{
    _n_readers += status.current_count_change;
    LOG_DEBUG( name() << ".on_publication_matched " << ( status.current_count_change > 0 ? "+" : "" )
                      << status.current_count_change << " -> " << _n_readers );
}


}  // namespace topics
}  // namespace realdds
