// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include <realdds/topics/ros2/parameter-events-msg.h>
#include <realdds/topics/ros2/rcl_interfaces/msg/ParameterEventPubSubTypes.h>

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
parameter_events_msg::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new parameter_events_msg::type ),
                                          topic_name );
}


bool parameter_events_msg::take_next( dds_topic_reader & reader, dds_sample * sample )
{
    dds_sample sample_;
    if( ! sample )
        sample = &sample_;  // use the local copy if the user hasn't provided their own
    auto status = reader->take_next_sample( &_raw, sample );
    if( status == ReturnCode_t::RETCODE_OK )
    {
        // Only samples for which valid_data is true should be accessed
        // valid_data indicates that the instance is still ALIVE and the `take` return an
        // updated sample
        if( ! sample->valid_data )
            invalidate();

        return true;
    }
    if ( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "parameter_events_msg::take_next", status );
}


dds_sequence_number parameter_events_msg::write_to( dds_topic_writer & writer ) const
{
    eprosima::fastrtps::rtps::WriteParams params;
    void const * pcv_data = &_raw;
    bool success = DDS_API_CALL( writer.get()->write( const_cast< void * >( pcv_data ), params ) );
    if( ! success )
    {
        LOG_ERROR( "Error writing message" );
        return 0;
    }
    // The params will contain, after the write, the sequence number (incremented automatically) for the sample that was
    // sent. The source_timestamp is always INVALID for some reason.
    return params.sample_identity().sequence_number().to64long();
}


}  // namespace ros2
}  // namespace topics
}  // namespace realdds
