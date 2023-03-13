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


dds_topic_reader_thread::dds_topic_reader_thread( std::shared_ptr< dds_topic > const & topic,
                                                  std::shared_ptr< dds_subscriber > const & subscriber )
    : super( topic, subscriber )
    , _th(
          [this]( dispatcher::cancellable_timer )
          {
              if( ! _reader )
                  return;
              eprosima::fastrtps::Duration_t const one_second = { 1, 0 };
              if( _reader->wait_for_unread_message( one_second ) )
                  _on_data_available();
          } )
{
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


}  // namespace realdds
