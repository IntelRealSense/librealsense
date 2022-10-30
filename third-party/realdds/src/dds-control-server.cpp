// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-control-server.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topics.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <librealsense2/utilities/concurrency/concurrency.h>


using namespace eprosima::fastdds::dds;

namespace realdds {


dds_control_server::dds_control_server( std::shared_ptr< dds_subscriber > const & subscriber, const std::string & topic_name )
    : _subscriber( subscriber )
{
    auto topic = topics::device::control::create_topic( subscriber->get_participant(), topic_name.c_str() );
    _reader = std::make_shared< dds_topic_reader >( topic, subscriber );

    dds_topic_reader::qos rqos( RELIABLE_RELIABILITY_QOS );
    rqos.history().depth = 10;                                                                      // default is 1
    rqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::DYNAMIC_RESERVE_MEMORY_MODE;
    _reader->run( rqos );
}


dds_control_server::~dds_control_server()
{
}


void dds_control_server::on_control_message_received( on_control_message_received_callback callback )
{
    if ( !_reader )
        DDS_THROW( runtime_error, "setting callback when reader is not created" );

    _reader->on_data_available( callback );
}


}  // namespace realdds
