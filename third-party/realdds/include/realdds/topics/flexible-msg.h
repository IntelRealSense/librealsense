// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include "flexible/flexible.h"
#include <realdds/dds-defines.h>

#include <rsutils/json-fwd.h>
#include <string>
#include <memory>
#include <vector>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SampleInfo;
}
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {

    
namespace raw {
class flexiblePubSubType;
}  // namespace raw


class flexible_msg
{
public:
    using type = raw::flexiblePubSubType;

    enum class data_format
    {
        // Note: these need to be kept in sync with the raw version
        JSON = raw::FLEXIBLE_DATA_JSON,
        CBOR = raw::FLEXIBLE_DATA_CBOR,
        CUSTOM = raw::FLEXIBLE_DATA_CUSTOM,
    };

    bool is_valid() const { return ! _data.empty(); }
    void invalidate() { _data.clear(); }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const& participant,
        char const* topic_name );
    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const& participant,
        std::string const& topic_name )
    {
        return create_topic( participant, topic_name.c_str() );
    }

    // This helper method will take the next sample from a reader.
    //
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    static bool take_next( dds_topic_reader &,
                           flexible_msg * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    // WARNING: this moves the message content!
    raw::flexible to_raw() &&;
    // WARNING: this moves the message content!
    // Returns some unique (to the writer) identifier for the sample that was sent, or 0 if unsuccessful
    dds_sequence_number write_to( dds_topic_writer & ) &&;

    flexible_msg() = default;
    flexible_msg( raw::flexible && );
    flexible_msg( rsutils::json const &, uint32_t version = 0 );
    flexible_msg( data_format format, rsutils::json const &, uint32_t version = 0 );

    rsutils::json json_data() const;

    // Get the custom data with casting to the desired type
    // Syntax: auto stream_info = msg.custom_data< STREAM_INFO >();
    //         if ( msg ) do something with it;
    template< typename T >
    T const * custom_data() const
    {
        return _data.size() > 0 ? reinterpret_cast< T const * >( _data.data() ) : nullptr;
    }
    template< typename T >
    T * custom_data()
    {
        return _data.size() > 0 ? reinterpret_cast< T * >( _data.data() ) : nullptr;
    }

    data_format _data_format;
    uint32_t _version = 0;
    std::vector< uint8_t > _data;
};


}  // namespace topics
}  // namespace realdds
