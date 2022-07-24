// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "imagePubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
namespace device {
class image
{
public:
    using type = raw::device::imagePubSubType;

    static std::string construct_stream_topic_name( const std::string& topic_root, const std::string& stream )
    {
        return topic_root + "/" + stream;
    }

    image( const raw::device::image & image )
        : raw_data(image.raw_data()) // TODO: avoid image copy?
        , width( image.width())
        , height( image.height() )
        , size( image.size() )
        , format( static_cast<rs2_format>( image.format() ) ) // Enum conversion, we assume the conversion is legal
    {
    }

    std::vector<uint8_t> raw_data;
    int width;
    int height;
    int size;
    rs2_format format;
};

}  // namespace device
}  // namespace topics
}  // namespace dds
}  // namespace librealsense
