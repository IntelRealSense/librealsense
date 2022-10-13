// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <string>
#include <memory>
#include <vector>


namespace realdds {


class dds_participant;
class dds_topic;


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

    image( const raw::device::image & );

    bool is_valid() const { return width != 0 && height != 0; }
    void invalidate() { width = 0; }

    static std::shared_ptr< dds_topic > create_topic( std::shared_ptr< dds_participant > const & participant,
                                                      char const * topic_name );

    std::vector<uint8_t> raw_data;
    int width = 0;
    int height = 0;
    int size;
    int format;
};


}  // namespace device
}  // namespace topics
}  // namespace realdds
