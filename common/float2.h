// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <cmath>  // sqrt, sqrtf


namespace rs2 {


struct float2
{
    float x, y;

    float length() const { return sqrt( x * x + y * y ); }

    float2 normalize() const { return { x / length(), y / length() }; }
};

inline float dot( const float2 & a, const float2 & b )
{
    return a.x * b.x + a.y * b.y;
}


}  // namespace rs2
