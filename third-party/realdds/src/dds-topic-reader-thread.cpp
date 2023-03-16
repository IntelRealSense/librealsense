// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {


dds_topic_reader_thread::dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic )
    : dds_topic_reader_thread( topic, std::make_shared< dds_subscriber >( topic->get_participant() ) )
{
}


dds_topic_reader_thread::dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic,
                                                  std::shared_ptr< dds_subscriber > const & subscriber )
    : super( topic, subscriber )
    , _th(
          [this, name = topic->get()->get_name()]( dispatcher::cancellable_timer )
          {
              if( ! _reader )
                  return;
              eprosima::fastrtps::Duration_t const one_second = { 1, 0 };
              LOG_DEBUG( "----> '" << name << "' waiting for message" );
              if( _reader->wait_for_unread_message( one_second ) )
              {
                  LOG_DEBUG( "----> '" << name << "' callback" );
                  _on_data_available();
                  LOG_DEBUG( "<---- '" << name << "' callback" );
              }
          } )
{
}


dds_topic_reader_thread::~dds_topic_reader_thread()
{
    LOG_DEBUG( "xxxxx '" << ( _reader ? _reader->get_topicdescription()->get_name() : "unknown" ) << "' dtor" );
}


void dds_topic_reader_thread::run( qos const & rqos )
{
    if( ! _on_data_available )
        DDS_THROW( runtime_error, "on-data-available must be provided" );

    eprosima::fastdds::dds::StatusMask status_mask;
    status_mask << eprosima::fastdds::dds::StatusMask::subscription_matched();
    //status_mask << eprosima::fastdds::dds::StatusMask::data_available();
    _reader = DDS_API_CALL( _subscriber->get()->create_datareader( _topic->get(), rqos, this, status_mask ) );
    _th.start();
}


void dds_topic_reader_thread::stop()
{
    _th.stop();
    super::stop();
}


}  // namespace realdds
