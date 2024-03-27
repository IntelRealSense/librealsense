// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/number/float3.h>
#include <rsutils/json.h>

#include <cmath>  // sqrt, sqrtf
#include <ostream>

using rsutils::json;


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


void to_json( json & j, float2 const & f2 )
{
    j = json::array( { f2.x, f2.y } );
}


void to_json( json & j, float3 const & f3 )
{
    j = json::array( { f3.x, f3.y, f3.z } );
}


void from_json( json const & j, float2 & f2 )
{
    if( ! j.is_array() || 2 != j.size() )
        throw rsutils::json::type_error::create( 317, "expected float2 array [x,y]", &j );
    j[0].get_to( f2.x );
    j[1].get_to( f2.y );
}


void from_json( json const & j, float3 & f3 )
{
    if( ! j.is_array() || 3 != j.size() )
        throw rsutils::json::type_error::create( 317, "expected float3 array [x,y,z]", &j );
    j[0].get_to( f3.x );
    j[1].get_to( f3.y );
    j[2].get_to( f3.z );
}


}  // namespace number
}  // namespace rsutils
