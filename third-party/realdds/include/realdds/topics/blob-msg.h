// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "blob/blob.h"
#include <realdds/dds-defines.h>

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


namespace udds {
class blobPubSubType;
}  // namespace raw


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;


namespace topics {


class blob_msg : public udds::blob
{
public:
    using type = udds::blobPubSubType;

    blob_msg() = default;
    blob_msg( std::vector< uint8_t > && data_ ) { data( std::move( data_ ) ); }

    bool is_valid() const { return ! data().empty(); }
    void invalidate() { data().clear(); }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );
    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      std::string const & topic_name )
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
                           blob_msg * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    // Returns some unique (to the writer) identifier for the sample that was sent, or 0 if unsuccessful
    dds_sequence_number write_to( dds_topic_writer & ) const;

    // Cast the raw data to the desired type
    template< typename T >
    T const * custom_data() const
    {
        return data().size() > 0 ? reinterpret_cast< T const * >( data().data() ) : nullptr;
    }
    template< typename T >
    T * custom_data()
    {
        return data().size() > 0 ? reinterpret_cast< T * >( data().data() ) : nullptr;
    }
};


}  // namespace topics
}  // namespace realdds
