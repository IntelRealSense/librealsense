// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include <realdds/topics/ros2/participant-entities-info-msg.h>
#include <realdds/topics/ros2/rmw_dds_common/msg/ParticipantEntitiesInfoPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {
namespace ros2 {


/*static*/ std::shared_ptr< dds_topic >
participant_entities_info_msg::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new participant_entities_info_msg::type ),
                                          topic_name );
}


/*static*/ bool
participant_entities_info_msg::take_next( dds_topic_reader & reader, participant_entities_info_msg * output, dds_sample * sample )
{
    participant_entities_info_msg output_;
    if( ! output )
        output = &output_;  // use the local copy if the user hasn't provided their own
    dds_sample sample_;
    if( ! sample )
        sample = &sample_;  // use the local copy if the user hasn't provided their own
    auto status = reader->take_next_sample( &output->_raw, sample );
    if( status == ReturnCode_t::RETCODE_OK )
    {
        // Only samples for which valid_data is true should be accessed
        // valid_data indicates that the instance is still ALIVE and the `take` return an
        // updated sample
        if( ! sample->valid_data )
            output->invalidate();

        return true;
    }
    if ( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "participant_entities_info_msg::take_next", status );
}


dds_sequence_number participant_entities_info_msg::write_to( dds_topic_writer & writer )
{
    eprosima::fastrtps::rtps::WriteParams params;
    bool success = DDS_API_CALL( writer.get()->write( &_raw, params ) );
    if( ! success )
    {
        LOG_ERROR( "Error writing message" );
        return 0;
    }
    // The params will contain, after the write, the sequence number (incremented automatically) for the sample that was
    // sent. The source_timestamp is always INVALID for some reason.
    return params.sample_identity().sequence_number().to64long();
}


// RMW gid is 24-bytes long, but only the first 16 seem to be of use
// NOTE: careful! This changed in ROS2 Iron (gid is 16 bytes), meaning we need to decide which version we're compatible
// with...
#define RMW_GID_STORAGE_SIZE 16u
// From rmw/types.h:
// "We define the GID as 16 bytes (128 bits).  This should be enough to ensure uniqueness amongst all entities in the
// system.  It is up to the individual RMW implementations to fill that in, either from the underlying middleware or
// from the RMW layer itself."


dds_guid participant_entities_info_msg::gid() const
{
    // See rmw_dds_common::convert_msg_to_gid()
    dds_guid guid;
    std::memcpy( &guid, _raw.gid().data().data(), RMW_GID_STORAGE_SIZE );
    return guid;
}


void participant_entities_info_msg::set_gid( dds_guid const & guid )
{
    // See rmw_dds_common::convert_gid_to_msg()
    rmw_dds_common::msg::Gid gid;
    std::memcpy( gid.data().data(), &guid, RMW_GID_STORAGE_SIZE );
    _raw.gid( std::move( gid ) );
}


void participant_entities_info_msg::node::add_reader( dds_guid const & guid )
{
    rmw_dds_common::msg::Gid gid;
    std::memcpy( gid.data().data(), &guid, RMW_GID_STORAGE_SIZE );
    reader_gid_seq().push_back( std::move( gid ) );
}


void participant_entities_info_msg::node::add_writer( dds_guid const & guid )
{
    rmw_dds_common::msg::Gid gid;
    std::memcpy( gid.data().data(), &guid, RMW_GID_STORAGE_SIZE );
    writer_gid_seq().push_back( std::move( gid ) );
}


}  // namespace ros2
}  // namespace topics
}  // namespace realdds
