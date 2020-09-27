//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "calibration.h"
#include "debug.h"
#include "utils.h"
#include <math.h>

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    void write_to_file( void const * data, size_t cb, std::string const & dir, char const * filename )
    {
        std::string path = dir + filename;
        std::fstream f( path, std::ios::out | std::ios::binary );
        if( ! f )
            throw std::runtime_error( "failed to open file:\n" + path );
        f.write( (char const *)data, cb );
        f.close();
    }

    double get_norma( const std::vector< double3 > & vec )
    {
        double sum = 0;
        std::for_each( vec.begin(), vec.end(), [&]( double3 const & v ) { sum += v.get_norm(); } );
        return std::sqrt(sum);
    }

    double rad_to_deg(double rad)
    {
        return rad * 180. / M_PI;
    }

    double deg_to_rad(double deg)
    {
        return deg * M_PI / 180.;
    }

    void direct_inv_6x6(const double A[36], const double B[6], double C[6])
    {
        double b_A[36];
        int i0;
        int j;
        signed char ipiv[6];
        int iy;
        int b;
        int jj;
        int jy;
        int jp1j;
        int n;
        int k;
        double smax;
        int ix;
        double s;

        /* INV_MY Summary of this function goes here */
        /*    Detailed explanation goes here */
        memcpy(&b_A[0], &A[0], 36U * sizeof(double));
        for (i0 = 0; i0 < 6; i0++) {
            ipiv[i0] = (signed char)(1 + i0);
        }

        for (j = 0; j < 5; j++) {
            b = j * 7;
            jj = j * 7;
            jp1j = b + 2;
            n = 6 - j;
            jy = 0;
            ix = b;
            smax = std::abs(b_A[b]);
            for (k = 2; k <= n; k++) {
                ix++;
                s = std::abs(b_A[ix]);
                if (s > smax) {
                    jy = k - 1;
                    smax = s;
                }
            }

            if (b_A[jj + jy] != 0.0) {
                if (jy != 0) {
                    iy = j + jy;
                    ipiv[j] = (signed char)(iy + 1);
                    ix = j;
                    for (k = 0; k < 6; k++) {
                        smax = b_A[ix];
                        b_A[ix] = b_A[iy];
                        b_A[iy] = smax;
                        ix += 6;
                        iy += 6;
                    }
                }

                i0 = (jj - j) + 6;
                for (iy = jp1j; iy <= i0; iy++) {
                    b_A[iy - 1] /= b_A[jj];
                }
            }

            n = 4 - j;
            jy = b + 6;
            iy = jj;
            for (b = 0; b <= n; b++) {
                smax = b_A[jy];
                if (b_A[jy] != 0.0) {
                    ix = jj + 1;
                    i0 = iy + 8;
                    jp1j = (iy - j) + 12;
                    for (k = i0; k <= jp1j; k++) {
                        b_A[k - 1] += b_A[ix] * -smax;
                        ix++;
                    }
                }

                jy += 6;
                iy += 6;
            }
        }

        for (iy = 0; iy < 6; iy++) {
            C[iy] = B[iy];
        }

        for (jy = 0; jy < 5; jy++) {
            if (ipiv[jy] != jy + 1) {
                smax = C[jy];
                iy = ipiv[jy] - 1;
                C[jy] = C[iy];
                C[iy] = smax;
            }
        }

        for (k = 0; k < 6; k++) {
            jy = 6 * k;
            if (C[k] != 0.0) {
                i0 = k + 2;
                for (iy = i0; iy < 7; iy++) {
                    C[iy - 1] -= C[k] * b_A[(iy + jy) - 1];
                }
            }
        }

        for (k = 5; k >= 0; k--) {
            jy = 6 * k;
            if (C[k] != 0.0) {
                C[k] /= b_A[k + jy];
                for (iy = 0; iy < k; iy++) {
                    C[iy] -= C[k] * b_A[iy + jy];
                }
            }
        }
    }

    void direct_inv_2x2(const double A[4], const double B[2], double C[2])
    {
        int r1;
        int r2;
        double a21;
        double C_tmp;

        /* INV_MY Summary of this function goes here */
        /*    Detailed explanation goes here */
        if (std::abs(A[1]) > std::abs(A[0])) {
            r1 = 1;
            r2 = 0;
        }
        else {
            r1 = 0;
            r2 = 1;
        }

        a21 = A[r2] / A[r1];
        C_tmp = A[2 + r1];
        C[1] = (B[r2] - B[r1] * a21) / (A[2 + r2] - a21 * C_tmp);
        C[0] = (B[r1] - C[1] * C_tmp) / A[r1];
    }

    void ndgrid_my(const double vec1[5], const double vec2[5], double yScalingGrid
        [25], double xScalingGrid[25])
    {
        int k;
        int b_k;

        /* NDGRID_MY Summary of this function goes here */
        /*    Detailed explanation goes here */
        for (k = 0; k < 5; k++) {
            for (b_k = 0; b_k < 5; b_k++) {
                yScalingGrid[b_k + 5 * k] = vec1[b_k];
            }
        }

        for (k = 0; k < 5; k++) {
            for (b_k = 0; b_k < 5; b_k++) {
                xScalingGrid[b_k + 5 * k] = vec2[k];
            }
        }

        /*  search around last estimated scaling */
    }

    void inv(const double x[9], double y[9])
    {
        double b_x[9];
        int p1;
        int p2;
        int p3;
        double absx11;
        double absx21;
        double absx31;
        int itmp;
        memcpy(&b_x[0], &x[0], 9U * sizeof(double));
        p1 = 0;
        p2 = 3;
        p3 = 6;
        absx11 = std::abs(x[0]);
        absx21 = std::abs(x[1]);
        absx31 = std::abs(x[2]);
        if ((absx21 > absx11) && (absx21 > absx31)) {
            p1 = 3;
            p2 = 0;
            b_x[0] = x[1];
            b_x[1] = x[0];
            b_x[3] = x[4];
            b_x[4] = x[3];
            b_x[6] = x[7];
            b_x[7] = x[6];
        }
        else {
            if (absx31 > absx11) {
                p1 = 6;
                p3 = 0;
                b_x[0] = x[2];
                b_x[2] = x[0];
                b_x[3] = x[5];
                b_x[5] = x[3];
                b_x[6] = x[8];
                b_x[8] = x[6];
            }
        }
    
        b_x[1] /= b_x[0];
        b_x[2] /= b_x[0];
        b_x[4] -= b_x[1] * b_x[3];
        b_x[5] -= b_x[2] * b_x[3];
        b_x[7] -= b_x[1] * b_x[6];
        b_x[8] -= b_x[2] * b_x[6];
        if (std::abs(b_x[5]) > std::abs(b_x[4])) {
            itmp = p2;
            p2 = p3;
            p3 = itmp;
            absx11 = b_x[1];
            b_x[1] = b_x[2];
            b_x[2] = absx11;
            absx11 = b_x[4];
            b_x[4] = b_x[5];
            b_x[5] = absx11;
            absx11 = b_x[7];
            b_x[7] = b_x[8];
            b_x[8] = absx11;
        }
    
        b_x[5] /= b_x[4];
        b_x[8] -= b_x[5] * b_x[7];
        absx11 = (b_x[5] * b_x[1] - b_x[2]) / b_x[8];
        absx21 = -(b_x[1] + b_x[7] * absx11) / b_x[4];
        y[p1] = ((1.0 - b_x[3] * absx21) - b_x[6] * absx11) / b_x[0];
        y[p1 + 1] = absx21;
        y[p1 + 2] = absx11;
        absx11 = -b_x[5] / b_x[8];
        absx21 = (1.0 - b_x[7] * absx11) / b_x[4];
        y[p2] = -(b_x[3] * absx21 + b_x[6] * absx11) / b_x[0];
        y[p2 + 1] = absx21;
        y[p2 + 2] = absx11;
        absx11 = 1.0 / b_x[8];
        absx21 = -b_x[7] * absx11 / b_x[4];
        y[p3] = -(b_x[3] * absx21 + b_x[6] * absx11) / b_x[0];
        y[p3 + 1] = absx21;
        y[p3 + 2] = absx11;
    }

    void transpose(const double x[9], double y[9])
    {
        for (auto i = 0; i < 3; i++)
            for (auto j = 0; j < 3; j++)
                y[i * 3 + j] = x[j * 3 + i];
    }

    void rotate_180(const uint8_t* A, uint8_t* B, uint32_t w, uint32_t h)
    {
        /* ROT Summary of this function goes here */
        /*    Detailed explanation goes here */
        for (int j = 0; j < int(w); j++) {
            for (int i = 0; i < int(h); i++) {
                B[i + h * j] = A[(h * (w - 1 - j) - i) + h - 1];
            }
        }
    }
    
    std::vector< double > interp1( const std::vector< double > & ind,
                                   const std::vector< double > & vals,
                                   const std::vector< double > & intrp )
    {
        std::vector<double> res(intrp.size(), 0);

        for (auto i = 0; i < intrp.size(); i++)
        {
            auto value = intrp[i];
            auto it = std::lower_bound(ind.begin(), ind.end(), value);
            if (it == ind.begin())
            {
                if (*it == ind.front())
                    res[i] = ind.front();
                else
                    res[i] = std::numeric_limits<double>::max();
            }
            else if (it == ind.end())
            {
                if (*it == ind.back())
                    res[i] = ind.back();
                else
                    res[i] = std::numeric_limits<double>::max();
            }
            else
            {
                auto val1 = *(--it);
                auto ind1 = std::distance(ind.begin(), it);
                auto val2= *(++it);
                auto ind2 = std::distance(ind.begin(), it);

                auto target_val1 = vals[ind1];
                auto target_val2 = vals[ind2];

                res[i] = ((val2 - value) / (val2 - val1))*target_val1 + ((value - val1) / (val2 - val1))*target_val2;
            }
        }
        return res;
    }


    double3x3 cholesky3x3( double3x3 const & mat )
    {
        double3x3 res = { 0 };

        for( auto i = 0; i < 3; i++ )
        {
            for( auto j = 0; j <= i; j++ )
            {
                double sum = 0;

                if( i == j )
                {
                    for( auto l = 0; l < i; l++ )
                    {
                        sum += res.mat[i][l] * res.mat[i][l];
                    }
                    res.mat[i][i] = sqrt( mat.mat[i][j] - sum );
                }
                else
                {
                    for( auto l = 0; l < j; l++ )
                    {
                        sum += res.mat[i][l] * res.mat[j][l];
                    }
                    res.mat[i][j] = ( mat.mat[i][j] - sum ) / res.mat[j][j];
                }
            }
        }
        return res;
    }

}
}
}