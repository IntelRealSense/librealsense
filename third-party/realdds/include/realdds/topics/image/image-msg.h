// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


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


namespace topics {
namespace raw {
namespace device {
class imagePubSubType;
class image;
}  // namespace device
}  // namespace raw

    
    
namespace device {

    
class image
{
public:
    using type = raw::device::imagePubSubType;

    image() = default;
    image( const raw::device::image & );
    image( const image & );
    image( image && );

    image & operator=( image && rhs );
    image & operator=( const raw::device::image & );

    bool is_valid() const { return width != 0 && height != 0; }
    void invalidate() { width = 0; }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );

    // This helper method will take the next sample from a reader. 
    // 
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    //Note - copies the data.
    //TODO - add an API for a function that loans the data and enables the user to free it later.
    static bool take_next( dds_topic_reader &,
                           image * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    std::vector<uint8_t> raw_data;
    int width = 0;
    int height = 0;
    int size;
    int format;
};


}  // namespace device
}  // namespace topics
}  // namespace realdds
