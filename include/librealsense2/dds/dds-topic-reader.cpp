// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-topic-reader.h"
#include "dds-topic.h"
#include "dds-participant.h"
#include "dds-utilities.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>


namespace librealsense {
namespace dds {


dds_topic_reader::dds_topic_reader( std::shared_ptr< dds_topic > const & topic )
    : _topic( topic )
{
}


dds_topic_reader::~dds_topic_reader()
{
    if( _subscriber )
    {
        if( _reader )
            DDS_API_CALL_NO_THROW( _subscriber->delete_datareader( _reader ) );
        if( _topic->get_participant()->is_valid() )
            DDS_API_CALL_NO_THROW( _topic->get_participant()->get()->delete_subscriber( _subscriber ) );
    }
}


void dds_topic_reader::run()
{
    // CREATE THE SUBSCRIBER
    _subscriber = DDS_API_CALL(
        _topic->get_participant()->get()->create_subscriber( eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT,
                                                             nullptr ) );

    // CREATE THE READER
    eprosima::fastdds::dds::DataReaderQos rqos = eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;

    // The 'depth' parameter of the History defines how many samples are stored before starting to
    // overwrite them with newer ones.
    rqos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 10;

    // We don't want to miss connection/disconnection events
    rqos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    // The Subscriber receives samples from the moment it comes online, not before
    rqos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
    rqos.data_sharing().off();

    eprosima::fastdds::dds::StatusMask status_mask;
    if( _on_subscription_matched )
        status_mask << eprosima::fastdds::dds::StatusMask::subscription_matched();
    if( _on_data_available )
        status_mask << eprosima::fastdds::dds::StatusMask::data_available();
    _reader = DDS_API_CALL(
        _subscriber->create_datareader( _topic->get(), rqos, status_mask.any() ? this : nullptr, status_mask ) );
}


void dds_topic_reader::on_subscription_matched(
    eprosima::fastdds::dds::DataReader *, eprosima::fastdds::dds::SubscriptionMatchedStatus const & info )
{
    // Called when the subscriber is matched (un)with a Writer
    if( _on_subscription_matched )
        _on_subscription_matched( info );
}

void dds_topic_reader::on_data_available( eprosima::fastdds::dds::DataReader * )
{
    // Called when a new  Data Message is received
    if( _on_data_available )
        _on_data_available();
}


}  // namespace dds
}  // namespace librealsense