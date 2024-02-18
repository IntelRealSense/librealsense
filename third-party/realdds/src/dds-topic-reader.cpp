// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-serialization.h>
#include <realdds/dds-utilities.h>

#include <rsutils/time/stopwatch.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>

#include <rsutils/json.h>


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
    // (default is PREALLOCATED_WITH_REALLOC_MEMORY_MODE)
    endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
}


void dds_topic_reader::qos::override_from_json( rsutils::json const & qos_settings )
{
    // Default values should be set before we're called:
    // All we do here is override those - if specified!
    override_reliability_qos_from_json( reliability(), qos_settings.nested( "reliability" ) );
    override_durability_qos_from_json( durability(), qos_settings.nested( "durability" ) );
    override_history_qos_from_json( history(), qos_settings.nested( "history" ) );
    override_liveliness_qos_from_json( liveliness(), qos_settings.nested( "liveliness" ) );
    override_data_sharing_qos_from_json( data_sharing(), qos_settings.nested( "data-sharing" ) );
    override_endpoint_qos_from_json( endpoint(), qos_settings.nested( "endpoint" ) );
}


void dds_topic_reader::run( qos const & rqos )
{
    // The DataReader is the Entity used by the application to subscribe to updated values of the data
    eprosima::fastdds::dds::StatusMask status_mask;
    status_mask << eprosima::fastdds::dds::StatusMask::subscription_matched();
    status_mask << eprosima::fastdds::dds::StatusMask::data_available();
    status_mask << eprosima::fastdds::dds::StatusMask::sample_lost();
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
    _n_writers = info.current_count;
    if( _on_subscription_matched )
        _on_subscription_matched( info );
}


void dds_topic_reader::on_data_available( eprosima::fastdds::dds::DataReader * )
{
    // Called when a new Data Message is received
    if( _on_data_available )
    {
        try
        {
            rsutils::time::stopwatch stopwatch;
            _on_data_available();
            if( stopwatch.get_elapsed() > std::chrono::milliseconds( 500 ) )
                LOG_WARNING( _topic->get()->get_name() << "' callback took too long!" );
        }
        catch( std::exception const & e )
        {
            LOG_ERROR( "[" << _topic->get()->get_name() << "] exception: " << e.what() );
        }
    }
}


void dds_topic_reader::on_sample_lost( eprosima::fastdds::dds::DataReader *, const eprosima::fastdds::dds::SampleLostStatus & status )
{
    // Called when a sample is lost: i.e., when a fragment is received that is a jump in sequence number
    // If such a jump in sequence number (sample) isn't received then we never get here!
    if( _on_sample_lost )
        _on_sample_lost( status );
    else
        LOG_WARNING( "[" << _topic->get()->get_name() << "] " << status.total_count_change << " sample(s) lost" );
}


}  // namespace realdds
