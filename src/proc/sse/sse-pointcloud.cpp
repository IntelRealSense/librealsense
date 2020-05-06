// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "environment.h"
#include "proc/occlusion-filter.h"
#include "proc/sse/sse-pointcloud.h"
#include "option.h"
#include "environment.h"
#include "context.h"

#include <iostream>

#ifdef __SSSE3__

#include <tmmintrin.h> // For SSSE3 intrinsics

#endif

namespace librealsense
{
    pointcloud_sse::pointcloud_sse() : pointcloud("Pointcloud (SSE3)") {}

    void pointcloud_sse::preprocess()
    {
        _pre_compute_map_x.resize(_depth_intrinsics->width*_depth_intrinsics->height);
        _pre_compute_map_y.resize(_depth_intrinsics->width*_depth_intrinsics->height);

        for (int h = 0; h < _depth_intrinsics->height; ++h)
        {
            for (int w = 0; w < _depth_intrinsics->width; ++w)
            {
                const float pixel[] = { (float)w, (float)h };

                float x = (pixel[0] - _depth_intrinsics->ppx) / _depth_intrinsics->fx;
                float y = (pixel[1] - _depth_intrinsics->ppy) / _depth_intrinsics->fy;


                if (_depth_intrinsics->model == RS2_DISTORTION_INVERSE_BROWN_CONRADY)
                {
                    float r2 = x * x + y * y;
                    float f = 1 + _depth_intrinsics->coeffs[0] * r2 + _depth_intrinsics->coeffs[1] * r2*r2 + _depth_intrinsics->coeffs[4] * r2*r2*r2;
                    float ux = x * f + 2 * _depth_intrinsics->coeffs[2] * x*y + _depth_intrinsics->coeffs[3] * (r2 + 2 * x*x);
                    float uy = y * f + 2 * _depth_intrinsics->coeffs[3] * x*y + _depth_intrinsics->coeffs[2] * (r2 + 2 * y*y);
                    x = ux;
                    y = uy;
                }

                _pre_compute_map_x[h*_depth_intrinsics->width + w] = x;
                _pre_compute_map_y[h*_depth_intrinsics->width + w] = y;
            }
        }
    }

    const float3* pointcloud_sse::depth_to_points(rs2::points output,
            const rs2_intrinsics &depth_intrinsics, 
            const rs2::depth_frame& depth_frame,
            float depth_scale)
    {
#ifdef __SSSE3__

        auto depth_image = (const uint16_t*)depth_frame.get_data();

        float* pre_compute_x = _pre_compute_map_x.data();
        float* pre_compute_y = _pre_compute_map_y.data();

        uint32_t size = depth_intrinsics.height * depth_intrinsics.width;

        auto point = (float*)output.get_vertices();

        //mask for shuffle
        const __m128i mask0 = _mm_set_epi8((char)0xff, (char)0xff, (char)7, (char)6, (char)0xff, (char)0xff, (char)5, (char)4,
            (char)0xff, (char)0xff, (char)3, (char)2, (char)0xff, (char)0xff, (char)1, (char)0);
        const __m128i mask1 = _mm_set_epi8((char)0xff, (char)0xff, (char)15, (char)14, (char)0xff, (char)0xff, (char)13, (char)12,
            (char)0xff, (char)0xff, (char)11, (char)10, (char)0xff, (char)0xff, (char)9, (char)8);

        auto scale = _mm_set_ps1(depth_scale);

        auto mapx = pre_compute_x;
        auto mapy = pre_compute_y;

        for (unsigned int i = 0; i < size; i += 8)
        {
            auto x0 = _mm_load_ps(mapx + i);
            auto x1 = _mm_load_ps(mapx + i + 4);

            auto y0 = _mm_load_ps(mapy + i);
            auto y1 = _mm_load_ps(mapy + i + 4);

            __m128i d = _mm_load_si128((__m128i const*)(depth_image + i));        //d7 d7 d6 d6 d5 d5 d4 d4 d3 d3 d2 d2 d1 d1 d0 d0

                                                                            //split the depth pixel to 2 registers of 4 floats each
            __m128i d0 = _mm_shuffle_epi8(d, mask0);        // 00 00 d3 d3 00 00 d2 d2 00 00 d1 d1 00 00 d0 d0
            __m128i d1 = _mm_shuffle_epi8(d, mask1);        // 00 00 d7 d7 00 00 d6 d6 00 00 d5 d5 00 00 d4 d4

            __m128 depth0 = _mm_cvtepi32_ps(d0); //convert depth to float
            __m128 depth1 = _mm_cvtepi32_ps(d1); //convert depth to float

            depth0 = _mm_mul_ps(depth0, scale);
            depth1 = _mm_mul_ps(depth1, scale);

            auto p0x = _mm_mul_ps(depth0, x0);
            auto p0y = _mm_mul_ps(depth0, y0);

            auto p1x = _mm_mul_ps(depth1, x1);
            auto p1y = _mm_mul_ps(depth1, y1);

            //scattering of the x y z
            auto x_y0 = _mm_shuffle_ps(p0x, p0y, _MM_SHUFFLE(2, 0, 2, 0));
            auto z_x0 = _mm_shuffle_ps(depth0, p0x, _MM_SHUFFLE(3, 1, 2, 0));
            auto y_z0 = _mm_shuffle_ps(p0y, depth0, _MM_SHUFFLE(3, 1, 3, 1));

            auto xyz01 = _mm_shuffle_ps(x_y0, z_x0, _MM_SHUFFLE(2, 0, 2, 0));
            auto xyz02 = _mm_shuffle_ps(y_z0, x_y0, _MM_SHUFFLE(3, 1, 2, 0));
            auto xyz03 = _mm_shuffle_ps(z_x0, y_z0, _MM_SHUFFLE(3, 1, 3, 1));

            auto x_y1 = _mm_shuffle_ps(p1x, p1y, _MM_SHUFFLE(2, 0, 2, 0));
            auto z_x1 = _mm_shuffle_ps(depth1, p1x, _MM_SHUFFLE(3, 1, 2, 0));
            auto y_z1 = _mm_shuffle_ps(p1y, depth1, _MM_SHUFFLE(3, 1, 3, 1));

            auto xyz11 = _mm_shuffle_ps(x_y1, z_x1, _MM_SHUFFLE(2, 0, 2, 0));
            auto xyz12 = _mm_shuffle_ps(y_z1, x_y1, _MM_SHUFFLE(3, 1, 2, 0));
            auto xyz13 = _mm_shuffle_ps(z_x1, y_z1, _MM_SHUFFLE(3, 1, 3, 1));


            //store 8 points of x y z
            _mm_stream_ps(&point[0], xyz01);
            _mm_stream_ps(&point[4], xyz02);
            _mm_stream_ps(&point[8], xyz03);
            _mm_stream_ps(&point[12], xyz11);
            _mm_stream_ps(&point[16], xyz12);
            _mm_stream_ps(&point[20], xyz13);
            point += 24;
        }
#endif
        return (float3*)output.get_vertices();
    }

    void pointcloud_sse::get_texture_map(rs2::points output,
        const float3* points,
        const unsigned int width,
        const unsigned int height,
        const rs2_intrinsics &other_intrinsics,
        const rs2_extrinsics& extr,
        float2* pixels_ptr)
    {
        auto tex_ptr = (float2*)output.get_texture_coordinates();

#ifdef __SSSE3__
        auto point = reinterpret_cast<const float*>(points);
        auto res = reinterpret_cast<float*>(tex_ptr);
        auto res1 = reinterpret_cast<float*>(pixels_ptr);

        __m128 r[9];
        __m128 t[3];
        __m128 c[5];

        for (int i = 0; i < 9; ++i)
        {
            r[i] = _mm_set_ps1(extr.rotation[i]);
        }
        for (int i = 0; i < 3; ++i)
        {
            t[i] = _mm_set_ps1(extr.translation[i]);
        }
        for (int i = 0; i < 5; ++i)
        {
            c[i] = _mm_set_ps1(other_intrinsics.coeffs[i]);
        }

        auto fx = _mm_set_ps1(other_intrinsics.fx);
        auto fy = _mm_set_ps1(other_intrinsics.fy);
        auto ppx = _mm_set_ps1(other_intrinsics.ppx);
        auto ppy = _mm_set_ps1(other_intrinsics.ppy);
        auto w = _mm_set_ps1(other_intrinsics.width);
        auto h = _mm_set_ps1(other_intrinsics.height);
        auto mask_inv_brown_conrady = _mm_set_ps1(RS2_DISTORTION_INVERSE_BROWN_CONRADY);
        auto zero = _mm_set_ps1(0);
        auto one = _mm_set_ps1(1);
        auto two = _mm_set_ps1(2);

        for (auto i = 0UL; i < height*width * 3; i += 12)
        {
            //load 4 points (x,y,z)
            auto xyz1 = _mm_load_ps(point + i);
            auto xyz2 = _mm_load_ps(point + i + 4);
            auto xyz3 = _mm_load_ps(point + i + 8);


            //gather x,y,z
            auto yz = _mm_shuffle_ps(xyz1, xyz2, _MM_SHUFFLE(1, 0, 2, 1));
            auto xy = _mm_shuffle_ps(xyz2, xyz3, _MM_SHUFFLE(2, 1, 3, 2));

            auto x = _mm_shuffle_ps(xyz1, xy, _MM_SHUFFLE(2, 0, 3, 0));
            auto y = _mm_shuffle_ps(yz, xy, _MM_SHUFFLE(3, 1, 2, 0));
            auto z = _mm_shuffle_ps(yz, xyz3, _MM_SHUFFLE(3, 0, 3, 1));

            auto p_x = _mm_add_ps(_mm_mul_ps(r[0], x), _mm_add_ps(_mm_mul_ps(r[3], y), _mm_add_ps(_mm_mul_ps(r[6], z), t[0])));
            auto p_y = _mm_add_ps(_mm_mul_ps(r[1], x), _mm_add_ps(_mm_mul_ps(r[4], y), _mm_add_ps(_mm_mul_ps(r[7], z), t[1])));
            auto p_z = _mm_add_ps(_mm_mul_ps(r[2], x), _mm_add_ps(_mm_mul_ps(r[5], y), _mm_add_ps(_mm_mul_ps(r[8], z), t[2])));

            p_x = _mm_div_ps(p_x, p_z);
            p_y = _mm_div_ps(p_y, p_z);

            // if(model == RS2_DISTORTION_MODIFIED_BROWN_CONRADY)
            auto dist = _mm_set_ps1(other_intrinsics.model);

            auto r2 = _mm_add_ps(_mm_mul_ps(p_x, p_x), _mm_mul_ps(p_y, p_y));
            auto r3 = _mm_add_ps(_mm_mul_ps(c[1], _mm_mul_ps(r2, r2)), _mm_mul_ps(c[4], _mm_mul_ps(r2, _mm_mul_ps(r2, r2))));
            auto f = _mm_add_ps(one, _mm_add_ps(_mm_mul_ps(c[0], r2), r3));

            auto x_f = _mm_mul_ps(p_x, f);
            auto y_f = _mm_mul_ps(p_y, f);

            auto r4 = _mm_mul_ps(c[3], _mm_add_ps(r2, _mm_mul_ps(two, _mm_mul_ps(x_f, x_f))));
            auto d_x = _mm_add_ps(x_f, _mm_add_ps(_mm_mul_ps(two, _mm_mul_ps(c[2], _mm_mul_ps(x_f, y_f))), r4));

            auto r5 = _mm_mul_ps(c[2], _mm_add_ps(r2, _mm_mul_ps(two, _mm_mul_ps(y_f, y_f))));
            auto d_y = _mm_add_ps(y_f, _mm_add_ps(_mm_mul_ps(two, _mm_mul_ps(c[3], _mm_mul_ps(x_f, y_f))), r4));

            auto cmp = _mm_cmpeq_ps(mask_inv_brown_conrady, dist);

            p_x = _mm_or_ps(_mm_and_ps(cmp, d_x), _mm_andnot_ps(cmp, p_x));
            p_y = _mm_or_ps(_mm_and_ps(cmp, d_y), _mm_andnot_ps(cmp, p_y));

            //TODO: add handle to RS2_DISTORTION_FTHETA

            //zero the x and y if z is zero
            cmp = _mm_cmpneq_ps(z, zero);
            p_x = _mm_and_ps(_mm_add_ps(_mm_mul_ps(p_x, fx), ppx), cmp);
            p_y = _mm_and_ps(_mm_add_ps(_mm_mul_ps(p_y, fy), ppy), cmp);

            //scattering of the x y before normalize and store in pixels_ptr
            auto xx_yy01 = _mm_shuffle_ps(p_x, p_y, _MM_SHUFFLE(2, 0, 2, 0));
            auto xx_yy23 = _mm_shuffle_ps(p_x, p_y, _MM_SHUFFLE(3, 1, 3, 1));

            auto xyxy1 = _mm_shuffle_ps(xx_yy01, xx_yy23, _MM_SHUFFLE(2, 0, 2, 0));
            auto xyxy2 = _mm_shuffle_ps(xx_yy01, xx_yy23, _MM_SHUFFLE(3, 1, 3, 1));

            _mm_stream_ps(res1, xyxy1);
            _mm_stream_ps(res1 + 4, xyxy2);
            res1 += 8;

            //normalize x and y
            p_x = _mm_div_ps(p_x, w);
            p_y = _mm_div_ps(p_y, h);

            //scattering of the x y after normalize and store in tex_ptr
            xx_yy01 = _mm_shuffle_ps(p_x, p_y, _MM_SHUFFLE(2, 0, 2, 0));
            xx_yy23 = _mm_shuffle_ps(p_x, p_y, _MM_SHUFFLE(3, 1, 3, 1));

            xyxy1 = _mm_shuffle_ps(xx_yy01, xx_yy23, _MM_SHUFFLE(2, 0, 2, 0));
            xyxy2 = _mm_shuffle_ps(xx_yy01, xx_yy23, _MM_SHUFFLE(3, 1, 3, 1));

            _mm_stream_ps(res, xyxy1);
            _mm_stream_ps(res + 4, xyxy2);
            res += 8;
        }
#endif

    }
}
