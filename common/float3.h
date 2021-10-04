// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <cmath>  // sqrt, sqrtf


namespace rs2 {


struct float3
{
    float x, y, z;

    float length() const { return sqrt( x * x + y * y + z * z ); }

    float3 normalize() const { return ( length() > 0 ) ? float3{ x / length(), y / length(), z / length() } : *this; }
};

inline float3 cross( const float3 & a, const float3 & b )
{
    return { a.y * b.z - b.y * a.z, a.x * b.z - b.x * a.z, a.x * b.y - a.y * b.x };
}

inline float3 operator*( const float3 & a, float t )
{
    return { a.x * t, a.y * t, a.z * t };
}

inline float3 operator/( const float3 & a, float t )
{
    return { a.x / t, a.y / t, a.z / t };
}

inline float3 operator+( const float3 & a, const float3 & b )
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline float3 operator-( const float3 & a, const float3 & b )
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}


}  // namespace rs2
