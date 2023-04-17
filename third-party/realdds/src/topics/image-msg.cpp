// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/ros2/ros2image.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/ros2/ros2imagePubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {
namespace device {


image::image( sensor_msgs::msg::Image && rhs )
{
    raw_data = std::move( rhs.data() );
    frame_id = std::move( rhs.header().frame_id() );
    width    = std::move( rhs.width() );
    height   = std::move( rhs.height() );
    timestamp = dds_time( rhs.header().stamp().sec(), rhs.header().stamp().nanosec() );
}


image & image::operator=( sensor_msgs::msg::Image && rhs )
{
    raw_data = std::move( rhs.data() );
    frame_id = std::move( rhs.header().frame_id() );
    width    = std::move( rhs.width() );
    height   = std::move( rhs.height() );
    timestamp = dds_time( rhs.header().stamp().sec(), rhs.header().stamp().nanosec() );

    return *this;
}


/*static*/ std::shared_ptr< dds_topic >
image::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new image::type ),
                                          topic_name );
}


/*static*/ bool
image::take_next( dds_topic_reader & reader, image * output, eprosima::fastdds::dds::SampleInfo * info )
{
    sensor_msgs::msg::Image raw_data;
    eprosima::fastdds::dds::SampleInfo info_;
    if ( !info )
        info = &info_;  // use the local copy if the user hasn't provided their own
    auto status = reader->take_next_sample( &raw_data, info );
    if ( status == ReturnCode_t::RETCODE_OK )
    {
        // We have data
        if ( output )
        {
            // Only samples for which valid_data is true should be accessed
            // valid_data indicates that the instance is still ALIVE and the `take` return an
            // updated sample
            if ( !info->valid_data )
                output->invalidate();
            else
                *output = std::move( raw_data ); //TODO - optimize copy, use dds loans
        }

        return true;
    }
    if ( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "image::take_next", status );
}

}  // namespace device
}  // namespace topics
}  // namespace realdds
