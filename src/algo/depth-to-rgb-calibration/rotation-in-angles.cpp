//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "calibration-types.h"
#include <cmath>
#include <cstdlib>


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    rotation extract_rotation_from_angles( const rotation_in_angles & rot_angles )
    {
        rotation res;

        auto sin_a = sin( rot_angles.alpha );
        auto sin_b = sin( rot_angles.beta );
        auto sin_g = sin( rot_angles.gamma );

        auto cos_a = cos( rot_angles.alpha );
        auto cos_b = cos( rot_angles.beta );
        auto cos_g = cos( rot_angles.gamma );

        //% function [R] = calcRmatRromAngs(xAlpha,yBeta,zGamma)
        //%     R = [cos( yBeta )*cos( zGamma ), -cos( yBeta )*sin( zGamma ), sin( yBeta ); ...
        //%          cos( xAlpha )*sin( zGamma ) + cos( zGamma )*sin( xAlpha )*sin( yBeta ), cos( xAlpha )*cos( zGamma ) - sin( xAlpha )*sin( yBeta )*sin( zGamma ), -cos( yBeta )*sin( xAlpha ); ...
        //%          sin( xAlpha )*sin( zGamma ) - cos( xAlpha )*cos( zGamma )*sin( yBeta ), cos( zGamma )*sin( xAlpha ) + cos( xAlpha )*sin( yBeta )*sin( zGamma ), cos( xAlpha )*cos( yBeta )];
        //% end
        // -> note the transposing of the coordinates

        res.rot[0] = cos_b * cos_g;
        res.rot[3] = -cos_b * sin_g;
        res.rot[6] = sin_b;
        res.rot[1] = cos_a * sin_g + cos_g * sin_a*sin_b;
        res.rot[4] = cos_a * cos_g - sin_a * sin_b*sin_g;
        res.rot[7] = -cos_b * sin_a;
        res.rot[2] = sin_a * sin_g - cos_a * cos_g*sin_b;
        res.rot[5] = cos_g * sin_a + cos_a * sin_b*sin_g;
        res.rot[8] = cos_a * cos_b;

        return res;
    }

    rotation_in_angles extract_angles_from_rotation( const double r[9] )
    {
        rotation_in_angles res;
        //% function [xAlpha,yBeta,zGamma] = extractAnglesFromRotMat(R)
        //%     xAlpha = atan2(-R(2,3),R(3,3));
        //%     yBeta = asin(R(1,3));
        //%     zGamma = atan2(-R(1,2),R(1,1));
        // -> we transpose the coordinates!
        res.alpha = atan2( -r[7], r[8] );
        res.beta = asin( r[6] );
        res.gamma = atan2( -r[3], r[0] );
        // -> additional code is in the original function that catches some corner
        //    case, but since these have never occurred we were told not to use it 

        // TODO Sanity-check: can be removed
        rotation rot = extract_rotation_from_angles( res );
        double sum = 0;
        for( auto i = 0; i < 9; i++ )
        {
            sum += rot.rot[i] - r[i];
        }
        auto epsilon = 0.00001;
        if( (abs( sum )) > epsilon )
            throw "No fit";
        return res;
    }


}
}
}
