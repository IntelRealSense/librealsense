// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#include <realdds/topics/ros2/list-parameters-msg.h>
#include <realdds/topics/ros2/rcl_interfaces/srv/ListParameters.h>
#include <realdds/topics/ros2/rcl_interfaces/srv/ListParametersPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include <rsutils/ios/field.h>
using field = rsutils::ios::field;


namespace realdds {
namespace topics {
namespace ros2 {


/*static*/ std::shared_ptr< dds_topic >
list_parameters_request_msg::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new list_parameters_request_msg::type ),
                                          topic_name );
}


bool list_parameters_request_msg::take_next( dds_topic_reader & reader, dds_sample * sample )
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
    if( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "list_parameters_request_msg::take_next", status );
}


std::ostream & operator<<( std::ostream & os, list_parameters_request_msg const & msg )
{
    os << field::separator << "depth" << field::value << msg.depth();
    for( auto & p : msg.prefixes() )
        os << field::separator << p;
    return os;
}


}  // namespace ros2
}  // namespace topics
}  // namespace realdds
