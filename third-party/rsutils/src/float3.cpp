// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/number/float3.h>

#include <cmath>  // sqrt, sqrtf
#include <ostream>


namespace rsutils {
namespace number {


std::ostream & operator<<( std::ostream & stream, const float3 & elem )
{
    return stream << elem.x << " " << elem.y << " " << elem.z;
}


std::ostream & operator<<( std::ostream & stream, const float4 & elem )
{
    return stream << elem.x << " " << elem.y << " " << elem.z << " " << elem.w;
}


std::ostream & operator<<( std::ostream & stream, const float3x3 & elem )
{
    return stream << elem.x << "\n" << elem.y << "\n" << elem.z;
}


float float2::length() const
{
    return sqrt( x * x + y * y );
}


float2 float2::normalized() const
{
    auto const l = length();
    if( l <= 0.f )
        return *this;
    return { x / l, y / l };
}


float float3::length() const
{
    return sqrt( x * x + y * y + z * z );
}


float3 float3::normalized() const
{
    auto const l = length();
    if( l <= 0.f )
        return *this;
    return { x / l, y / l, z / l };
}


}  // namespace number
}  // namespace rsutils
