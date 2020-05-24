// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>  // all the basic types (uint8_t)
#include <vector>  // all the basic types (uint8_t)


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    struct double3
    {
        double x, y, z;
        double & operator [] ( int i ) { return (&x)[i]; }
        bool operator == (const double3 d) { return x == d.x && y == d.y && z == d.z; }
        bool operator != (const double3 d) { return !(*this == d); }
    };

    struct double2
    {
        double x, y;
        double & operator [] ( int i ) { return (&x)[i]; };
        bool operator == (const double2 d) { return x == d.x && y == d.y; }
        bool operator != (const double2 d) { return !(*this == d); }
    };

        struct double3x3
        {
                double mat[3][3];
                double3x3 transpose()
                {
                        double3x3 res = { 0 };

                        for (auto i = 0; i < 3; i++)
                        {
                                for (auto j = 0; j < 3; j++)
                                {
                                        res.mat[i][j] = mat[j][i];
                                }
                        }
                        return res;
                }

                double3x3 operator*(const double3x3& other)
                {
                        double3x3 res = { 0 };

                        for (auto i = 0; i < 3; i++)
                        {
                                for (auto j = 0; j < 3; j++)
                                {                                        
                                        double sum = 0;
                                        for (auto l = 0; l < 3; l++)
                                        {
                                                sum += mat[i][l] * other.mat[l][j];
                                        }
                                        res.mat[i][j] = sum;
                                }
                        }
                        return res;
                }

                double3 operator*(const double3& other)
                {
                        double3 res = { 0 };
        
                        res.x = mat[0][0] * other.x + mat[0][1] * other.y + mat[0][2] * other.z;
                        res.y = mat[1][0] * other.x + mat[1][1] * other.y + mat[1][2] * other.z;
                        res.z = mat[2][0] * other.x + mat[2][1] * other.y + mat[2][2] * other.z;
                        return res;
                }

                std::vector<double> to_vector()
                {
                        std::vector<double> res;
                        for (auto i = 0; i < 3; i++)
                        {
                                for (auto j = 0; j < 3; j++)
                                {
                                        res.push_back(mat[i][j]);
                                }
                        }
                        return res;
                }
        };

    enum direction :uint8_t
    {
        //deg_111, //to be aligned with matlab (maybe should be removed later) 
        deg_0, // 0, 1
        deg_45, //1, 1
        deg_90, //1, 0
        deg_135, //1, -1
        deg_180, //
        deg_225, //
        deg_270, //
        deg_315, //
        deg_none
    };
    enum svm_model :uint8_t
    {
        linear,
        gaussian
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

    struct k_matrix
    {
        double fx;
        double fy;
        double ppx;
        double ppy;
    };

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
