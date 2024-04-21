// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <iosfwd>
#include <cassert>


namespace rsutils {
namespace number {


////////////////////////////////////////////
// World's tiniest linear algebra library //
////////////////////////////////////////////


#pragma pack( push, 1 )
struct int2
{
    int x, y;
};
struct float2
{
    float x, y;
    float & operator[]( int i )
    {
        assert( i >= 0 );
        assert( i < 2 );
        return *( &x + i );
    }
    float length() const;
    float2 normalized() const;
};
struct float3
{
    float x, y, z;
    float & operator[]( int i )
    {
        assert( i >= 0 );
        assert( i < 3 );
        return ( *( &x + i ) );
    }
    float length() const;
    float3 normalized() const;
};
struct float4
{
    float x, y, z, w;
    float & operator[]( int i )
    {
        assert( i >= 0 );
        assert( i < 4 );
        return ( *( &x + i ) );
    }
};
struct float3x3
{
    float3 x, y, z;
    float & operator()( int i, int j )
    {
        assert( i >= 0 );
        assert( i < 3 );
        assert( j >= 0 );
        assert( j < 3 );
        return ( *( &x[0] + j * sizeof( float3 ) / sizeof( float ) + i ) );
    }
};  // column-major
#pragma pack( pop )


inline float dot( const float2 & a, const float2 & b )
{
    return a.x * b.x + a.y * b.y;
}


inline bool operator==( const float3 & a, const float3 & b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline float3 operator+( const float3 & a, const float3 & b )
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}
inline float3 operator-( const float3 & a, const float3 & b )
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
inline float3 operator*( const float3 & a, float b )
{
    return { a.x * b, a.y * b, a.z * b };
}
inline float3 operator/( const float3 & a, float t )
{
    return { a.x / t, a.y / t, a.z / t };
}
inline float operator*( const float3 & a, const float3 & b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline float3 cross( const float3 & a, const float3 & b )
{
    return { a.y * b.z - b.y * a.z, a.x * b.z - b.x * a.z, a.x * b.y - a.y * b.x };
}


inline bool operator==( const float4 & a, const float4 & b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
inline float4 operator+( const float4 & a, const float4 & b )
{
    return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}
inline float4 operator-( const float4 & a, const float4 & b )
{
    return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}


inline bool operator==( const float3x3 & a, const float3x3 & b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline float3 operator*( const float3x3 & a, const float3 & b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline float3x3 operator*( const float3x3 & a, const float3x3 & b )
{
    return { a * b.x, a * b.y, a * b.z };
}
inline float3x3 transpose( const float3x3 & a )
{
    return { { a.x.x, a.y.x, a.z.x }, { a.x.y, a.y.y, a.z.y }, { a.x.z, a.y.z, a.z.z } };
}


std::ostream & operator<<( std::ostream &, const float3 & );
std::ostream & operator<<( std::ostream &, const float4 & );
std::ostream & operator<<( std::ostream &, const float3x3 & );


}  // namespace number
}  // namespace rsutils