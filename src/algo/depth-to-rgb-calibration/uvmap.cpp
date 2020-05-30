//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "uvmap.h"
#include <limits>
#include "debug.h"

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    static void transform_point_to_uv( double pixel[2], const p_matrix & pmat, const double point[3] )
    {
        auto p = pmat.vals;
        // pmat * point
        double x = p[0] * point[0] + p[1] * point[1] + p[2] * point[2] + p[3];
        double y = p[4] * point[0] + p[5] * point[1] + p[6] * point[2] + p[7];
        double z = p[8] * point[0] + p[9] * point[1] + p[10] * point[2] + p[11];

        pixel[0] = x / z;
        pixel[1] = y / z;
    }


    /* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
    static void distort_pixel( double pixel[2], const rs2_intrinsics_double * intrin, const double point[2] )
    {
        double x = (point[0] - intrin->ppx) / intrin->fx;
        double y = (point[1] - intrin->ppy) / intrin->fy;

        if( intrin->model == RS2_DISTORTION_BROWN_CONRADY )
        {
            double r2 = x * x + y * y;
            double r2c = 1 + intrin->coeffs[0] * r2 + intrin->coeffs[1] * r2*r2 + intrin->coeffs[4] * r2*r2*r2;

            double xcd = x * r2c;
            double ycd = y * r2c;

            double dx = xcd + 2 * intrin->coeffs[2] * x*y + intrin->coeffs[3] * (r2 + 2 * x*x);
            double dy = ycd + 2 * intrin->coeffs[3] * x*y + intrin->coeffs[2] * (r2 + 2 * y*y);

            x = dx;
            y = dy;
        }

        pixel[0] = x * (double)intrin->fx + (double)intrin->ppx;
        pixel[1] = y * (double)intrin->fy + (double)intrin->ppy;
    }


    uvmap_t get_texture_map(
        std::vector< double3 > const & points,
        const calib & cal,
        const p_matrix & p_mat
    )
    {
        auto intrinsics = cal.get_intrinsics();

        std::vector< double2 > uv_map( points.size() );

        for( auto i = 0; i < points.size(); ++i )
        {
            double2 uv;
            transform_point_to_uv( &uv.x, p_mat, &points[i].x );

            double2 uvmap;
            distort_pixel( &uvmap.x, &intrinsics, &uv.x );
            uv_map[i] = uvmap;
        }

        return uv_map;
    }


    std::vector< double > biliniar_interp(
        std::vector< double > const & vals,
        size_t width,
        size_t height,
        uvmap_t const & uv
    )
    {
        std::vector< double > res( uv.size() );

        for( auto i = 0; i < uv.size(); i++ )
        {
            auto x = uv[i].x;
            auto x1 = floor( x );
            auto x2 = ceil( x );
            auto y = uv[i].y;
            auto y1 = floor( y );
            auto y2 = ceil( y );

            if( x1 < 0 || x1 >= width || x2 < 0 || x2 >= width ||
                y1 < 0 || y1 >= height || y2 < 0 || y2 >= height )
            {
                res[i] = std::numeric_limits<double>::max();
                continue;
            }

            // x1 y1    x2 y1
            // x1 y2    x2 y2

            // top_l    top_r
            // bot_l    bot_r

            auto top_l = vals[int( y1*width + x1 )];
            auto top_r = vals[int( y1*width + x2 )];
            auto bot_l = vals[int( y2*width + x1 )];
            auto bot_r = vals[int( y2*width + x2 )];

            double interp_x_top, interp_x_bot;
            if( x1 == x2 )
            {
                interp_x_top = top_l;
                interp_x_bot = bot_l;
            }
            else
            {
                interp_x_top = ((double)(x2 - x) / (double)(x2 - x1))*(double)top_l + ((double)(x - x1) / (double)(x2 - x1))*(double)top_r;
                interp_x_bot = ((double)(x2 - x) / (double)(x2 - x1))*(double)bot_l + ((double)(x - x1) / (double)(x2 - x1))*(double)bot_r;
            }

            if( y1 == y2 )
            {
                res[i] = interp_x_bot;
                continue;
            }

            auto interp_y_x = ((double)(y2 - y) / (double)(y2 - y1))*(double)interp_x_top + ((double)(y - y1) / (double)(y2 - y1))*(double)interp_x_bot;
            res[i] = interp_y_x;
        }

#if 0
        std::ofstream f;
        f.open( "interp_y_x" );
        f.precision( 16 );
        for( auto i = 0; i < res.size(); i++ )
        {
            f << res[i] << std::endl;
        }
        f.close();
#endif
        return res;
    }


}
}
}