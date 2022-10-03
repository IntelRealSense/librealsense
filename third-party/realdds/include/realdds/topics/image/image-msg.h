// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "imagePubSubTypes.h"

namespace realdds {
namespace topics {
namespace device {

    
class image
{
public:
    using type = raw::device::imagePubSubType;

    static std::string construct_topic_name( const std::string & topic_root, const std::string & stream )
    {
        return topic_root + "/" + stream;
    }

    image( const raw::device::image & image )
        : raw_data(image.raw_data()) // TODO: avoid image copy?
        , width( image.width())
        , height( image.height() )
        , size( image.size() )
        , format( image.format() )
    {
    }

    std::vector<uint8_t> raw_data;
    int width;
    int height;
    int size;
    int format;
};


}  // namespace device
}  // namespace topics
}  // namespace realdds
