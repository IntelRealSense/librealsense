// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {


dds_topic_writer::dds_topic_writer( std::shared_ptr< dds_topic > const & topic )
    : dds_topic_writer( topic, std::make_shared< dds_publisher >( topic->get_participant() ) )
{
}


dds_topic_writer::dds_topic_writer( std::shared_ptr< dds_topic > const & topic,
                                    std::shared_ptr< dds_publisher > const & publisher )
    : _topic( topic )
    , _publisher( publisher )
{
}


dds_topic_writer::~dds_topic_writer()
{
    if( _writer )
        DDS_API_CALL_NO_THROW( _publisher->get()->delete_datawriter( _writer ) );
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


void dds_topic_writer::run( qos const & wqos )
{
    eprosima::fastdds::dds::StatusMask status_mask;
    status_mask << eprosima::fastdds::dds::StatusMask::publication_matched();
    _writer = DDS_API_CALL( _publisher->get()->create_datawriter( _topic->get(), wqos, this, status_mask ) );
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
