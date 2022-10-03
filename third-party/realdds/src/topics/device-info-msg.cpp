// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/device-info/deviceInfoPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {


device_info::device_info( raw::device_info const & dev )
    : name( dev.name().data() )
    , serial( dev.serial_number().data() )
    , product_line( dev.product_line().data() )
    , topic_root( dev.topic_root().data() )
    , locked( dev.locked() )
{
}


/*static*/ std::shared_ptr< dds_topic >
device_info::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new device_info::type ),
                                          topic_name );
}


/*static*/ bool
device_info::take_next( dds_topic_reader & reader, device_info * output, eprosima::fastdds::dds::SampleInfo * info )
{
    raw::device_info raw_data;
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
    auto err = get_dds_error( status );
    LOG_ERROR( "DDS API CALL 'take_next_sample'" << err );
    throw std::runtime_error( "take_next_sample" + err );
}


}  // namespace topics
}  // namespace realdds
