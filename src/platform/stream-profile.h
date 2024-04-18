// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <functional>  // std::hash
#include <tuple>


namespace librealsense {
namespace platform {


typedef std::tuple< uint32_t, uint32_t, uint32_t, uint32_t > stream_profile_tuple;


struct stream_profile
{
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t format;

    operator stream_profile_tuple() const { return std::make_tuple( width, height, fps, format ); }
};


inline bool operator==( const stream_profile & a, const stream_profile & b )
{
    return ( a.width == b.width ) && ( a.height == b.height ) && ( a.fps == b.fps ) && ( a.format == b.format );
}


}  // namespace platform
}  // namespace librealsense


namespace std {


template<>
struct hash< librealsense::platform::stream_profile >
{
    size_t operator()( const librealsense::platform::stream_profile & k ) const
    {
        using std::hash;

        return ( hash< uint32_t >()( k.height ) ) ^ ( hash< uint32_t >()( k.width ) ) ^ ( hash< uint32_t >()( k.fps ) )
             ^ ( hash< uint32_t >()( k.format ) );
    }
};


}  // namespace std
