// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>  // all the basic types (uint8_t)


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    struct double3
    {
        double x, y, z;
        double & operator [] ( int i ) { return (&x)[i]; }
        bool operator == ( const double3 d ) { return x == d.x && y == d.y && z == d.z; }
        bool operator != ( const double3 d ) { return !(*this == d); }
    };

    struct double2
    {
        double x, y;
        double & operator [] ( int i ) { return (&x)[i]; };
        bool operator == ( const double2 d ) { return x == d.x && y == d.y; }
        bool operator != ( const double2 d ) { return !(*this == d); }
    };

    enum direction :uint8_t
    {
        deg_0, //0, 1
        deg_45, //1, 1
        deg_90, //1, 0
        deg_135, //1, -1
        deg_none
    };

    struct translation
    {
        double t1;
        double t2;
        double t3;
    };

    struct rotation
    {
        double rot[9];
    };

    struct rotation_in_angles
    {
        double alpha;
        double beta;
        double gamma;

        bool operator==(const rotation_in_angles& other)
        {
            return alpha == other.alpha && beta == other.beta && gamma == other.gamma;
        }
        bool operator!=(const rotation_in_angles& other)
        {
            return !(*this == other);
        }
        bool operator<(const rotation_in_angles& other)
        {
            return (alpha < other.alpha) ||
                (alpha == other.alpha && beta < other.beta) ||
                (alpha == other.alpha && beta == other.beta && gamma < other.gamma);

        }
    };

    rotation extract_rotation_from_angles( const rotation_in_angles & rot_angles );
    rotation_in_angles extract_angles_from_rotation( const double r[9] );

    struct p_matrix
    {
        double vals[12];
    };

    struct k_matrix
    {
        double fx;
        double fy;
        double ppx;
        double ppy;

        bool operator==(const k_matrix& other)
        {
            return fx == other.fx && fy == other.fy && ppx == other.ppx && ppy == other.ppy;
        }
        bool operator!=(const k_matrix& other)
        {
            return !(*this == other);
        }
        bool operator<(const k_matrix& other)
        {
            return (fx < other.fx) ||
                (fx == other.fx && fy < other.fy) ||
                (fx == other.fx && fy == other.fy && ppx < other.ppx) ||
                (fx == other.fx && fy == other.fy && ppx == other.ppx && ppy < other.ppy);
                
        }
    };

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
