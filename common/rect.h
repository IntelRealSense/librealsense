// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include "float2.h"

#include <algorithm>  // max,min
#include <cmath>  // floor


namespace rs2 {


template < typename T > T normalizeT( const T & in_val, const T & min, const T & max )
{
    if( min >= max )
        return 0;
    return ( ( in_val - min ) / ( max - min ) );
}

template < typename T > T unnormalizeT( const T & in_val, const T & min, const T & max )
{
    if( min == max )
        return min;
    return ( ( in_val * ( max - min ) ) + min );
}


struct rect
{
    float x, y;
    float w, h;

    void operator=( const rect & other )
    {
        x = other.x;
        y = other.y;
        w = other.w;
        h = other.h;
    }

    operator bool() const { return w * w > 0 && h * h > 0; }

    bool operator==( const rect & other ) const { return x == other.x && y == other.y && w == other.w && h == other.h; }

    bool operator!=( const rect & other ) const { return ! ( *this == other ); }

    rect normalize( const rect & normalize_to ) const
    {
        return rect{ normalizeT( x, normalize_to.x, normalize_to.x + normalize_to.w ),
                     normalizeT( y, normalize_to.y, normalize_to.y + normalize_to.h ),
                     normalizeT( w, 0.f, normalize_to.w ),
                     normalizeT( h, 0.f, normalize_to.h ) };
    }

    rect unnormalize( const rect & unnormalize_to ) const
    {
        return rect{ unnormalizeT( x, unnormalize_to.x, unnormalize_to.x + unnormalize_to.w ),
                     unnormalizeT( y, unnormalize_to.y, unnormalize_to.y + unnormalize_to.h ),
                     unnormalizeT( w, 0.f, unnormalize_to.w ),
                     unnormalizeT( h, 0.f, unnormalize_to.h ) };
    }

    // Calculate the intersection between two rects
    // If the intersection is empty, a rect with width and height zero will be returned
    rect intersection( const rect & other ) const
    {
        auto x1 = std::max( x, other.x );
        auto y1 = std::max( y, other.y );
        auto x2 = std::min( x + w, other.x + other.w );
        auto y2 = std::min( y + h, other.y + other.h );

        return { x1, y1, std::max( x2 - x1, 0.f ), std::max( y2 - y1, 0.f ) };
    }

    // Calculate the area of the rect
    float area() const { return w * h; }

    rect cut_by( const rect & r ) const
    {
        auto x1 = x;
        auto y1 = y;
        auto x2 = x + w;
        auto y2 = y + h;

        x1 = std::max( x1, r.x );
        x1 = std::min( x1, r.x + r.w );
        y1 = std::max( y1, r.y );
        y1 = std::min( y1, r.y + r.h );

        x2 = std::max( x2, r.x );
        x2 = std::min( x2, r.x + r.w );
        y2 = std::max( y2, r.y );
        y2 = std::min( y2, r.y + r.h );

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    bool contains( const float2 & p ) const
    {
        return ( p.x >= x ) && ( p.x < x + w ) && ( p.y >= y ) && ( p.y < y + h );
    }

    rect pan( const float2 & p ) const { return { x - p.x, y - p.y, w, h }; }

    rect center() const { return { x + w / 2.f, y + h / 2.f, 0, 0 }; }

    rect adjust_ratio( float2 size ) const
    {
        auto H = static_cast< float >( h ), W = static_cast< float >( h ) * size.x / size.y;
        if( W > w )
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return { float( floor( x + floor( w - W ) / 2 ) ), float( floor( y + floor( h - H ) / 2 ) ), W, H };
    }

    rect scale( float factor ) const { return { x, y, w * factor, h * factor }; }

    rect grow( int pixels ) const { return { x - pixels, y - pixels, w + pixels * 2, h + pixels * 2 }; }

    rect grow( int dx, int dy ) const { return { x - dx, y - dy, w + dx * 2, h + dy * 2 }; }

    rect shrink_by( float2 pixels ) const { return { x + pixels.x, y + pixels.y, w - pixels.x * 2, h - pixels.y * 2 }; }

    rect center_at( const float2 & new_center ) const
    {
        auto c = center();
        auto diff_x = new_center.x - c.x;
        auto diff_y = new_center.y - c.y;

        return { x + diff_x, y + diff_y, w, h };
    }

    rect fit( rect r ) const
    {
        float new_w = w;
        float new_h = h;

        if( w < r.w )
            new_w = r.w;

        if( h < r.h )
            new_h = r.h;

        auto res = rect{ x, y, new_w, new_h };
        return res.adjust_ratio( { w, h } );
    }

    rect zoom( float zoom_factor ) const
    {
        auto c = center();
        return scale( zoom_factor ).center_at( { c.x, c.y } );
    }

    rect enclose_in( rect in_rect ) const
    {
        rect out_rect{ x, y, w, h };
        if( w > in_rect.w || h > in_rect.h )
        {
            return in_rect;
        }

        if( x < in_rect.x )
        {
            out_rect.x = in_rect.x;
        }

        if( y < in_rect.y )
        {
            out_rect.y = in_rect.y;
        }


        if( x + w > in_rect.x + in_rect.w )
        {
            out_rect.x = in_rect.x + in_rect.w - w;
        }

        if( y + h > in_rect.y + in_rect.h )
        {
            out_rect.y = in_rect.y + in_rect.h - h;
        }

        return out_rect;
    }

    bool intersects( const rect & other ) const
    {
        return other.contains( { x, y } ) || other.contains( { x + w, y } ) || other.contains( { x, y + h } )
            || other.contains( { x + w, y + h } ) || contains( { other.x, other.y } );
    }
};


}  // namespace rs2
