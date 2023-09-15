// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/h/rs_sensor.h>
#include <functional>
#include <utility>  // std::pair
#include <cstdint>


namespace librealsense {


struct resolution
{
    uint32_t width, height;
};


using resolution_func = std::function< resolution( resolution res ) >;


struct stream_profile
{
    stream_profile(
        rs2_format fmt = RS2_FORMAT_ANY,
        rs2_stream strm = RS2_STREAM_ANY,
        int idx = 0,
        uint32_t w = 0,
        uint32_t h = 0,
        uint32_t framerate = 0,
        resolution_func res_func = []( resolution res ) { return res; } )
        : format( fmt )
        , stream( strm )
        , index( idx )
        , width( w )
        , height( h )
        , fps( framerate )
        , stream_resolution( res_func )
    {
    }

    rs2_format format;
    rs2_stream stream;
    int index;
    uint32_t width, height, fps;
    resolution_func stream_resolution;  // Calculates the relevant resolution from the given backend resolution.

    std::pair< uint32_t, uint32_t > width_height() const { return std::make_pair( width, height ); }
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
    size_t operator()( const rs2_format & f ) const
    {
        using std::hash;

        return hash< uint32_t >()( f );
    }
};


}  // namespace std
