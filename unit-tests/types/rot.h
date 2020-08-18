#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

inline
float3x3 rotx( float a )
{
    double rad = a * M_PI / 180.;
    return { 
        { 1,      0,               0           },
        { 0, float(cos(rad)), float(-sin(rad)) },
        { 0, float(sin(rad)), float(cos(rad))  }
    };
}




