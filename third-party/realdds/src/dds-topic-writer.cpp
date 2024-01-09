// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-serialization.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-guid.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include <rsutils/time/timer.h>
#include <rsutils/json.h>


namespace realdds {


dds_topic_writer::dds_topic_writer( std::shared_ptr< dds_topic > const & topic )
    : dds_topic_writer( topic, std::make_shared< dds_publisher >( topic->get_participant() ) )
{
}


dds_topic_writer::dds_topic_writer( std::shared_ptr< dds_topic > const & topic,
                                    std::shared_ptr< dds_publisher > const & publisher )
    : _topic( topic )
    , _publisher( publisher )
    , _n_readers( 0 )
{
}


dds_topic_writer::~dds_topic_writer()
{
    if( _writer )
        DDS_API_CALL_NO_THROW( _publisher->get()->delete_datawriter( _writer ) );
}


dds_guid const & dds_topic_writer::guid() const
{
    return _writer ? _writer->guid() : unknown_guid;
}


dds_topic_writer::qos::qos( eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability_kind,
                            eprosima::fastdds::dds::DurabilityQosPolicyKind durability_kind )
    : super( eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT )
{
    // NOTE: might want to match these with the corresponding values in dds_topic_reader

    reliability().kind = reliability_kind;  // default is reliable
    durability().kind = durability_kind;  // default is transient local
    //publish_mode().kind = eprosima::fastdds::dds::SYNCHRONOUS_PUBLISH_MODE;  // the default

    //history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;  // the default
    //history().depth = 1;

    // Does not allocate for every sample but still gives flexibility. See:
    //     https://github.com/eProsima/Fast-DDS/discussions/2707
    endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

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
}


void dds_topic_writer::qos::override_from_json( rsutils::json const & qos_settings )
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


void dds_topic_writer::run( qos const & wqos )
{
    eprosima::fastdds::dds::StatusMask status_mask;
    status_mask << eprosima::fastdds::dds::StatusMask::publication_matched();
    _writer = DDS_API_CALL( _publisher->get()->create_datawriter( _topic->get(), wqos, this, status_mask ) );
}


bool dds_topic_writer::wait_for_readers( dds_time timeout )
{
    // Better to use on_publication_matched, but that would require additional data members etc.
    // For now, keep it simple:
    rsutils::time::timer timer( std::chrono::nanoseconds( timeout.to_ns() ) );
    while( _n_readers.load() < 1 )
    {
        if( timer.has_expired() )
            return false;
        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
    }
    return true;
}


bool dds_topic_writer::wait_for_acks( dds_time timeout )
{
    return !! _writer->wait_for_acknowledgments( timeout );
}


void dds_topic_writer::on_publication_matched( eprosima::fastdds::dds::DataWriter *,
                                               eprosima::fastdds::dds::PublicationMatchedStatus const & info )
{
    // Called when the subscriber is (un)matched with a Writer
    _n_readers = info.current_count;
    if( _on_publication_matched )
        _on_publication_matched( info );
}


}  // namespace realdds
