// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/h/rs_sensor.h>
#include <functional>  // std::hash
#include <utility>     // std::swap
#include <cstdint>


namespace librealsense {


struct stream_profile
{
    rs2_format format;
    rs2_stream stream;
    int index;
    uint32_t width;
    uint32_t height;
    uint32_t fps;

    // Certain cameras may introduce transformed resolutions in their target profiles. E.g., this was the case with the
    // L500 (both rotation, plus expansion for confidence). This is not in use with the D series.
    //
    // See provided implementations below...
    //
    typedef void ( *resolution_transform_fn )( uint32_t & width, uint32_t & height );

    static void same_resolution( uint32_t & w, uint32_t & h ) {}
    static void rotate_resolution( uint32_t & w, uint32_t & h ) { std::swap( w, h ); }

    // Transformation function, to adjust width/height when a profile is cloned as part of a sensor's defined
    // processing block. The formats_converter uses this when doing its thing.
    // 
    resolution_transform_fn resolution_transform;

    stream_profile(
        rs2_format fmt = RS2_FORMAT_ANY,
        rs2_stream strm = RS2_STREAM_ANY,
        int idx = 0,
        uint32_t w = 0,
        uint32_t h = 0,
        uint32_t framerate = 0,
        resolution_transform_fn res_transform = &same_resolution )
        : format( fmt )
        , stream( strm )
        , index( idx )
        , width( w )
        , height( h )
        , fps( framerate )
        , resolution_transform( res_transform )
    {
    }
};


inline bool operator==( const stream_profile & a, const stream_profile & b )
{
    return ( a.width == b.width ) && ( a.height == b.height ) && ( a.fps == b.fps ) && ( a.format == b.format )
        && ( a.index == b.index ) && ( a.stream == b.stream );
}

inline bool operator<( const stream_profile & lhs, const stream_profile & rhs )
{
    if( lhs.format != rhs.format )
        return lhs.format < rhs.format;
    if( lhs.index != rhs.index )
        return lhs.index < rhs.index;
    return lhs.stream < rhs.stream;
}


}  // namespace librealsense


namespace std {


template<>
struct hash< librealsense::stream_profile >
{
    size_t operator()( const librealsense::stream_profile & k ) const
    {
        using std::hash;

        return ( hash< uint32_t >()( k.height ) ) ^ ( hash< uint32_t >()( k.width ) ) ^ ( hash< uint32_t >()( k.fps ) )
             ^ ( hash< uint32_t >()( k.format ) ) ^ ( hash< uint32_t >()( k.stream ) );
    }
};


template<>
struct hash< rs2_format >
{
    size_t operator()( const rs2_format & f ) const { return std::hash< uint32_t >()( f ); }
};


}  // namespace std
