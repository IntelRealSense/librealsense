#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

inline
float3x3 rotx( float a )
{
    a = a * M_PI / 180.;
    return { { 1,0,0 }, { 0,cos( a ),-sin( a ) }, { 0,sin(a),cos(a) } };
}




