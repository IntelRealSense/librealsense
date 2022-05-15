// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "imagePubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
class image
{
public:
    using type = raw::imagePubSubType;

    static std::string construct_name( const std::string& model, const std::string& sn, const std::string& stream )
    {
        return model + "/" + sn + "/" + stream;
    }

    image( const std::string & topic_name )
        : _topic_name( topic_name )
        , width( 0 )
        , height( 0 )
        , stride( 0 )
        , bpp( 0 )
        , format( RS2_FORMAT_ANY )
    {
    }

    image( const std::string &topic_name,  const raw::image & image )
        : _topic_name( topic_name )
        , raw_data(image.raw_data()) // TODO: avoid image copy?
        , width( image.width())
        , height( image.height() )
        , stride( image.stride() )
        , bpp( image.bpp() )
        , format( static_cast<rs2_format>( image.format() ) ) // Enum conversion, we assume the conversion is legal
    {
    }

    const std::string _topic_name;
    std::vector<uint8_t> raw_data;
    int width;
    int height;
    int stride;
    int bpp;
    rs2_format format;
};

}  // namespace topics
}  // namespace dds
}  // namespace librealsense
