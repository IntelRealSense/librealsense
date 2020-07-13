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
        double3 operator-(const double3& other) {
            return { x - other.x, y - other.y , z - other.z };
        }

        double get_norm() const
        {
            return x*x + y * y + z * z;
        }

        double3 operator*(const double& scalar) const {
            return { x * scalar , y * scalar , z * scalar };
        }

        double operator*(const double3& s) const {
            return x * s.x + y * s.y + z * s.z;
        }
    };

    struct double2
    {
        double x, y;
        double & operator [] ( int i ) { return (&x)[i]; };
        bool operator == (const double2 & d) const { return x == d.x && y == d.y; }
        bool operator != (const double2 & d) const { return !(*this == d); }
    };

        struct double3x3
    {
        /* double3x3(double arr[9])
         {
             for (auto i = 0; i < 3; i++)
             {
                 for (auto j = 0; j < 3; j++)
                 {
                     mat[i][j] = arr[i * 3 + j];
                 }
             }
         }*/
        double mat[3][3];
        double3x3 transpose()
        {
            double3x3 res = { {0} };

            for( auto i = 0; i < 3; i++ )
            {
                for( auto j = 0; j < 3; j++ )
                {
                    res.mat[i][j] = mat[j][i];
                }
            }
            return res;
        }

        double3x3 operator*( const double3x3 & other )
        {
            double3x3 res = { {0} };

            for( auto i = 0; i < 3; i++ )
            {
                for( auto j = 0; j < 3; j++ )
                {
                    double sum = 0;
                    for( auto l = 0; l < 3; l++ )
                    {
                        sum += mat[i][l] * other.mat[l][j];
                    }
                    res.mat[i][j] = sum;
                }
            }
            return res;
        }

        double3 operator*( const double3 & other )
        {
            double3 res = { 0 };

            res.x = mat[0][0] * other.x + mat[0][1] * other.y + mat[0][2] * other.z;
            res.y = mat[1][0] * other.x + mat[1][1] * other.y + mat[1][2] * other.z;
            res.z = mat[2][0] * other.x + mat[2][1] * other.y + mat[2][2] * other.z;
            return res;
        }

        std::vector< double > to_vector()
        {
            std::vector< double > res;
            for( auto i = 0; i < 3; i++ )
            {
                for( auto j = 0; j < 3; j++ )
                {
                    res.push_back( mat[i][j] );
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

    const int N_BASIC_DIRECTIONS = direction::deg_180;

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

    struct matrix_3x3
    {
        double rot[9];

        matrix_3x3 transposed() const
        {
            return { rot[0], rot[3], rot[6], 
                rot[1], rot[4], rot[7],
                rot[2], rot[5], rot[8] };
        }
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

    matrix_3x3 extract_rotation_from_angles( const rotation_in_angles & rot_angles );
    rotation_in_angles extract_angles_from_rotation( const double r[9] );

    struct k_matrix
    {
        k_matrix() = default;
        k_matrix(matrix_3x3 const & mat)
            :k_mat(mat)
        {}

        double get_fx() const { return k_mat.rot[0]; }
        double get_fy() const { return k_mat.rot[4]; }
        double get_ppx() const { return k_mat.rot[2]; }
        double get_ppy() const { return k_mat.rot[5]; }

        matrix_3x3 k_mat;

        matrix_3x3 as_3x3()
        {
            return k_mat;
        }
    };

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
