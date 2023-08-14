// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>


namespace realdds {


dds_topic_reader::dds_topic_reader( std::shared_ptr< dds_topic > const & topic )
    : dds_topic_reader( topic, std::make_shared< dds_subscriber >( topic->get_participant() ) )
{
}


dds_topic_reader::dds_topic_reader( std::shared_ptr< dds_topic > const & topic,
                                    std::shared_ptr< dds_subscriber > const & subscriber )
    : _topic( topic )
    , _subscriber( subscriber )
{
}


dds_topic_reader::~dds_topic_reader()
{
    if( _subscriber )
    {
        if( _reader )
            DDS_API_CALL_NO_THROW( _subscriber->get()->delete_datareader( _reader ) );
    }
}


dds_topic_reader::qos::qos( eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability_kind,
                            eprosima::fastdds::dds::DurabilityQosPolicyKind durability_kind )
    : super( eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT )
{
    // The 'depth' parameter of the History defines how many samples are stored before starting to
    // overwrite them with newer ones.
    history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    history().depth = 10;  // default is 1

    // reliable = We don't want to miss connection/disconnection events
    // (default is best-effort)
    reliability().kind = reliability_kind;

    // volatile = The Subscriber receives samples from the moment it comes online, not before
    // (this is the default, but bears repeating)
    durability().kind = durability_kind;

    // The writer-reader handshake is always done on UDP, even when data_sharing (shared memory) is used for
    // actual messaging. This means it is possible to send a message and receive it on the reader's
    // side even before the UDP handshake is complete:
    // 
    //   1. The writer goes up and broadcasts its presence periodically; no readers exist
    //   2. The reader joins and broadcasts its presence, again periodically; it doesn<92>t know about the writer yet
    //   3. The writer sees the reader (in-between broadcasts) so sends a message
    //   4. The reader gets the message and discards it because it does not yet recognize the writer
    // 
    // This depends on timing. When shared memory is on, step 3 is so fast that this miscommunication is much more
    // likely. This is a known gap in the DDS standard.
    //
    // We can either insert a sleep between writer creation and message sending or we can disable
    // data_sharing for this topic, which we did here.
    // 
    // See https://github.com/eProsima/Fast-DDS/issues/2641
    //
    data_sharing().off();

    // Does not allocate for every sample but still gives flexibility. See:
    //     https://github.com/eProsima/Fast-DDS/discussions/2707
    // (default is PREALLOCATED_MEMORY_MODE)
    endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
}


void dds_topic_reader::run( qos const & rqos )
{
    // The DataReader is the Entity used by the application to subscribe to updated values of the data
    eprosima::fastdds::dds::StatusMask status_mask;
    status_mask << eprosima::fastdds::dds::StatusMask::subscription_matched();
    status_mask << eprosima::fastdds::dds::StatusMask::data_available();
    _reader = DDS_API_CALL( _subscriber->get()->create_datareader( _topic->get(), rqos, this, status_mask ) );
}


void dds_topic_reader::stop()
{
    if( _subscriber )
    {
        if( _reader )
        {
            DDS_API_CALL_NO_THROW( _subscriber->get()->delete_datareader( _reader ) );
            _reader = nullptr;
        }
    }
    assert( ! is_running() );
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


}  // namespace realdds
