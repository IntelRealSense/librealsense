// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/notification/notification-msg.h>
#include <realdds/topics/notification/notificationPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {
namespace device {


notification::notification( const raw::device::notification & main )
    : _msg_type( static_cast< msg_type >( main.id() ) )
    , _raw_data( main.raw_data() )  // TODO: avoid data copy?
    , _size( main.size() )
{
    if( ! is_valid() )
    {
        DDS_THROW( runtime_error, "unsupported message type received, id:" + main.id() );
    }
}


/*static*/ std::shared_ptr< dds_topic >
notification::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new notification::type ),
                                          topic_name );
}


/*static*/ bool
notification::take_next( dds_topic_reader & reader, notification * output, eprosima::fastdds::dds::SampleInfo * info )
{
    raw::device::notification raw_data;
    eprosima::fastdds::dds::SampleInfo info_;
    if( ! info )
        info = &info_;  // use the local copy if the user hasn't provided their own
    auto status = reader->take_next_sample( &raw_data, info );
    if( status == ReturnCode_t::RETCODE_OK )
    {
        // We have data
        if( output )
        {
            // Only samples for which valid_data is true should be accessed
            // valid_data indicates that the instance is still ALIVE and the `take` return an
            // updated sample
            if( ! info->valid_data )
                output->invalidate();
            else
                *output = raw_data;
        }
        return true;
    }
    if( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "notification::take_next", status );
}


}  // namespace device
}  // namespace topics
}  // namespace realdds
