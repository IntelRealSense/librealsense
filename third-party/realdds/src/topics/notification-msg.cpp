// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/notification/notification-msg.h>
#include <realdds/topics/notification/notificationPubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include <third-party/json.hpp>
using nlohmann::json;


namespace realdds {
namespace topics {


notification::notification( raw::notification && main )
    : _data_format( static_cast< data_format >( main.data_format() ) )
    , _data( std::move( main.data() ))
    , _version(( main.version()[0] << 24 ) + ( main.version()[1] << 16 ) + ( main.version()[2] << 8 ) + main.version()[3] )
{
}


notification::notification( json const & j, uint32_t version )
    : _data_format( data_format::JSON )
    , _version( version )
{
    std::string json_as_string = j.dump();
    _data.resize( json_as_string.length() );
    json_as_string.copy( (char *)_data.data(), json_as_string.length() );
}


notification::notification( data_format format, json const & j, uint32_t version )
    : _data_format( format )
    , _version( version )
{
    if( data_format::CBOR == format )
    {
        _data = std::move( json::to_cbor( j ) );
    }
    else if( data_format::JSON == format )
    {
        std::string json_as_string = j.dump();
        _data.resize( json_as_string.length() );
        json_as_string.copy( (char*) _data.data(), json_as_string.length() );
    }
    else
    {
        DDS_THROW( runtime_error, "invalid format for json notification" );
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
    raw::notification raw_data;
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
                *output = std::move( raw_data );
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


json notification::json_data() const
{
    if( _data_format != data_format::JSON )
        DDS_THROW( runtime_error, "non-json notification data is still unsupported" );
    char const * begin = (char const *)_data.data();
    char const * end = begin + _data.size();
    return json::parse( begin, end );
}



}  // namespace topics
}  // namespace realdds