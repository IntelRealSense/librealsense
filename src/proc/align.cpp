// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "../include/librealsense2/rsutil.h"

#include "core/video.h"
#include "proc/synthetic-stream.h"
#include "environment.h"
#include "align.h"
#include "stream.h"

#ifdef __SSSE3__
#include <tmmintrin.h> // For SSE3 intrinsic used in unpack_yuy2_sse
#endif

namespace librealsense
{
    template<int N> struct bytes {
        byte b[N];
    };

#ifdef __SSSE3__
    template<rs2_distortion dist>
    inline void distorte_x_y(const __m128 & x, const __m128 & y, __m128 * distorted_x, __m128 * distorted_y, const rs2_intrinsics& to)
    {
        *distorted_x = x;
        *distorted_y = y;
    }
    template<>
    inline void distorte_x_y<RS2_DISTORTION_MODIFIED_BROWN_CONRADY>(const __m128& x, const __m128& y, __m128* distorted_x, __m128* distorted_y, const rs2_intrinsics& to)
    {
        __m128 c[5];
        auto one = _mm_set_ps1(1);
        auto two = _mm_set_ps1(2);

        for (int i = 0; i < 5; ++i)
        {
            c[i] = _mm_set_ps1(to.coeffs[i]);
        }
        auto r2_0 = _mm_add_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y));
        auto r3_0 = _mm_add_ps(_mm_mul_ps(c[1], _mm_mul_ps(r2_0, r2_0)), _mm_mul_ps(c[4], _mm_mul_ps(r2_0, _mm_mul_ps(r2_0, r2_0))));
        auto f_0 = _mm_add_ps(one, _mm_add_ps(_mm_mul_ps(c[0], r2_0), r3_0));

        auto x_f0 = _mm_mul_ps(x, f_0);
        auto y_f0 = _mm_mul_ps(y, f_0);

        auto r4_0 = _mm_mul_ps(c[3], _mm_add_ps(r2_0, _mm_mul_ps(two, _mm_mul_ps(x_f0, x_f0))));
        auto d_x0 = _mm_add_ps(x_f0, _mm_add_ps(_mm_mul_ps(two, _mm_mul_ps(c[2], _mm_mul_ps(x_f0, y_f0))), r4_0));

        auto r5_0 = _mm_mul_ps(c[2], _mm_add_ps(r2_0, _mm_mul_ps(two, _mm_mul_ps(y_f0, y_f0))));
        auto d_y0 = _mm_add_ps(y_f0, _mm_add_ps(_mm_mul_ps(two, _mm_mul_ps(c[3], _mm_mul_ps(x_f0, y_f0))), r4_0));

        *distorted_x = d_x0;
        *distorted_y = d_y0;
    }


    template<rs2_distortion dist>
    inline void get_texture_map_sse(const uint16_t * depth,
        float depth_scale,
        const unsigned int size,
        const float * pre_compute_x, const float * pre_compute_y,
        byte * pixels_ptr_int,
        const rs2_intrinsics& to,
        const rs2_extrinsics& from_to_other)
    {
        //mask for shuffle
        const __m128i mask0 = _mm_set_epi8((char)0xff, (char)0xff, (char)7, (char)6, (char)0xff, (char)0xff, (char)5, (char)4,
            (char)0xff, (char)0xff, (char)3, (char)2, (char)0xff, (char)0xff, (char)1, (char)0);
        const __m128i mask1 = _mm_set_epi8((char)0xff, (char)0xff, (char)15, (char)14, (char)0xff, (char)0xff, (char)13, (char)12,
            (char)0xff, (char)0xff, (char)11, (char)10, (char)0xff, (char)0xff, (char)9, (char)8);

        auto zerro = _mm_set_ps1(0);
        auto scale = _mm_set_ps1(depth_scale);

        auto mapx = pre_compute_x;
        auto mapy = pre_compute_y;

        auto res = reinterpret_cast<__m128i*>(pixels_ptr_int);

        __m128 r[9];
        __m128 t[3];
        __m128 c[5];

        for (int i = 0; i < 9; ++i)
        {
            r[i] = _mm_set_ps1(from_to_other.rotation[i]);
        }
        for (int i = 0; i < 3; ++i)
        {
            t[i] = _mm_set_ps1(from_to_other.translation[i]);
        }
        for (int i = 0; i < 5; ++i)
        {
            c[i] = _mm_set_ps1(to.coeffs[i]);
        }
        auto zero = _mm_set_ps1(0);
        auto fx = _mm_set_ps1(to.fx);
        auto fy = _mm_set_ps1(to.fy);
        auto ppx = _mm_set_ps1(to.ppx);
        auto ppy = _mm_set_ps1(to.ppy);

        for (unsigned int i = 0; i < size; i += 8)
        {
            auto x0 = _mm_load_ps(mapx + i);
            auto x1 = _mm_load_ps(mapx + i + 4);

            auto y0 = _mm_load_ps(mapy + i);
            auto y1 = _mm_load_ps(mapy + i + 4);


            __m128i d = _mm_load_si128((__m128i const*)(depth + i));        //d7 d7 d6 d6 d5 d5 d4 d4 d3 d3 d2 d2 d1 d1 d0 d0

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

            auto p_x0 = _mm_add_ps(_mm_mul_ps(r[0], p0x), _mm_add_ps(_mm_mul_ps(r[3], p0y), _mm_add_ps(_mm_mul_ps(r[6], depth0), t[0])));
            auto p_y0 = _mm_add_ps(_mm_mul_ps(r[1], p0x), _mm_add_ps(_mm_mul_ps(r[4], p0y), _mm_add_ps(_mm_mul_ps(r[7], depth0), t[1])));
            auto p_z0 = _mm_add_ps(_mm_mul_ps(r[2], p0x), _mm_add_ps(_mm_mul_ps(r[5], p0y), _mm_add_ps(_mm_mul_ps(r[8], depth0), t[2])));

            auto p_x1 = _mm_add_ps(_mm_mul_ps(r[0], p1x), _mm_add_ps(_mm_mul_ps(r[3], p1y), _mm_add_ps(_mm_mul_ps(r[6], depth1), t[0])));
            auto p_y1 = _mm_add_ps(_mm_mul_ps(r[1], p1x), _mm_add_ps(_mm_mul_ps(r[4], p1y), _mm_add_ps(_mm_mul_ps(r[7], depth1), t[1])));
            auto p_z1 = _mm_add_ps(_mm_mul_ps(r[2], p1x), _mm_add_ps(_mm_mul_ps(r[5], p1y), _mm_add_ps(_mm_mul_ps(r[8], depth1), t[2])));

            p_x0 = _mm_div_ps(p_x0, p_z0);
            p_y0 = _mm_div_ps(p_y0, p_z0);

            p_x1 = _mm_div_ps(p_x1, p_z1);
            p_y1 = _mm_div_ps(p_y1, p_z1);

            distorte_x_y<dist>(p_x0, p_y0, &p_x0, &p_y0, to);
            distorte_x_y<dist>(p_x1, p_y1, &p_x1, &p_y1, to);

            //zero the x and y if z is zero
            auto cmp = _mm_cmpneq_ps(depth0, zero);
            p_x0 = _mm_and_ps(_mm_add_ps(_mm_mul_ps(p_x0, fx), ppx), cmp);
            p_y0 = _mm_and_ps(_mm_add_ps(_mm_mul_ps(p_y0, fy), ppy), cmp);


            p_x1 = _mm_add_ps(_mm_mul_ps(p_x1, fx), ppx);
            p_y1 = _mm_add_ps(_mm_mul_ps(p_y1, fy), ppy);

            cmp = _mm_cmpneq_ps(depth0, zero);
            auto half = _mm_set_ps1(0.5);
            auto u_round0 = _mm_and_ps(_mm_add_ps(p_x0, half), cmp);
            auto v_round0 = _mm_and_ps(_mm_add_ps(p_y0, half), cmp);

            auto uuvv1_0 = _mm_shuffle_ps(u_round0, v_round0, _MM_SHUFFLE(1, 0, 1, 0));
            auto uuvv2_0 = _mm_shuffle_ps(u_round0, v_round0, _MM_SHUFFLE(3, 2, 3, 2));

            auto res1_0 = _mm_shuffle_ps(uuvv1_0, uuvv1_0, _MM_SHUFFLE(3, 1, 2, 0));
            auto res2_0 = _mm_shuffle_ps(uuvv2_0, uuvv2_0, _MM_SHUFFLE(3, 1, 2, 0));

            auto res1_int0 = _mm_cvtps_epi32(res1_0);
            auto res2_int0 = _mm_cvtps_epi32(res2_0);

            _mm_stream_si128(&res[0], res1_int0);
            _mm_stream_si128(&res[1], res2_int0);
            res += 2;

            cmp = _mm_cmpneq_ps(depth1, zero);
            auto u_round1 = _mm_and_ps(_mm_add_ps(p_x1, half), cmp);
            auto v_round1 = _mm_and_ps(_mm_add_ps(p_y1, half), cmp);

            auto uuvv1_1 = _mm_shuffle_ps(u_round1, v_round1, _MM_SHUFFLE(1, 0, 1, 0));
            auto uuvv2_1 = _mm_shuffle_ps(u_round1, v_round1, _MM_SHUFFLE(3, 2, 3, 2));

            auto res1 = _mm_shuffle_ps(uuvv1_1, uuvv1_1, _MM_SHUFFLE(3, 1, 2, 0));
            auto res2 = _mm_shuffle_ps(uuvv2_1, uuvv2_1, _MM_SHUFFLE(3, 1, 2, 0));

            auto res1_int1 = _mm_cvtps_epi32(res1);
            auto res2_int1 = _mm_cvtps_epi32(res2);

            _mm_stream_si128(&res[0], res1_int1);
            _mm_stream_si128(&res[1], res2_int1);
            res += 2;
        }
    }


    image_transform::image_transform(const rs2_intrinsics& from, float depth_scale)
        :_depth(from),
        _depth_scale(depth_scale),
        _pixel_top_left_int(from.width*from.height),
        _pixel_bottom_right_int(from.width*from.height)
    {
    }

    void image_transform::pre_compute_x_y_map_corners()
    {
        pre_compute_x_y_map(_pre_compute_map_x_top_left, _pre_compute_map_y_top_left, -0.5f);
        pre_compute_x_y_map(_pre_compute_map_x_bottom_right, _pre_compute_map_y_bottom_right, 0.5f);
    }

    void image_transform::pre_compute_x_y_map(std::vector<float>& pre_compute_map_x,
        std::vector<float>& pre_compute_map_y,
        float offset)
    {
        pre_compute_map_x.resize(_depth.width*_depth.height);
        pre_compute_map_y.resize(_depth.width*_depth.height);

        for (int h = 0; h < _depth.height; ++h)
        {
            for (int w = 0; w < _depth.width; ++w)
            {
                const float pixel[] = { (float)w + offset, (float)h + offset };

                float x = (pixel[0] - _depth.ppx) / _depth.fx;
                float y = (pixel[1] - _depth.ppy) / _depth.fy;

                if (_depth.model == RS2_DISTORTION_INVERSE_BROWN_CONRADY)
                {
                    float r2 = x*x + y*y;
                    float f = 1 + _depth.coeffs[0] * r2 + _depth.coeffs[1] * r2*r2 + _depth.coeffs[4] * r2*r2*r2;
                    float ux = x*f + 2 * _depth.coeffs[2] * x*y + _depth.coeffs[3] * (r2 + 2 * x*x);
                    float uy = y*f + 2 * _depth.coeffs[3] * x*y + _depth.coeffs[2] * (r2 + 2 * y*y);
                    x = ux;
                    y = uy;
                }

                pre_compute_map_x[h*_depth.width + w] = x;
                pre_compute_map_y[h*_depth.width + w] = y;
            }
        }
    }

    void image_transform::align_depth_to_other(const uint16_t* z_pixels, uint16_t* dest, int bpp, const rs2_intrinsics& depth, const rs2_intrinsics& to,
        const rs2_extrinsics& from_to_other)
    {
        switch (to.model)
        {
        case RS2_DISTORTION_MODIFIED_BROWN_CONRADY:
            align_depth_to_other_sse<RS2_DISTORTION_MODIFIED_BROWN_CONRADY>(z_pixels, dest, depth, to, from_to_other);
            break;
        default:
            align_depth_to_other_sse(z_pixels, dest, depth, to, from_to_other);
            break;
        }
    }

    inline void image_transform::move_depth_to_other(const uint16_t* z_pixels, uint16_t* dest, const rs2_intrinsics& to,
        const std::vector<int2>& pixel_top_left_int,
        const std::vector<int2>& pixel_bottom_right_int)
    {
        for (int y = 0; y < _depth.height; ++y)
        {
            for (int x = 0; x < _depth.width; ++x)
            {
                auto depth_pixel_index = y*_depth.width + x;
                // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
                if (z_pixels[depth_pixel_index])
                {
                    for (int other_y = pixel_top_left_int[depth_pixel_index].y; other_y <= pixel_bottom_right_int[depth_pixel_index].y; ++other_y)
                    {
                        for (int other_x = pixel_top_left_int[depth_pixel_index].x; other_x <= pixel_bottom_right_int[depth_pixel_index].x; ++other_x)
                        {
                            if (other_x < 0 || other_y < 0 || other_x >= to.width || other_y >= to.height)
                                continue;
                            auto other_ind = other_y * to.width + other_x;

                            dest[other_ind] = dest[other_ind] ? std::min(dest[other_ind], z_pixels[depth_pixel_index]) : z_pixels[depth_pixel_index];
                        }
                    }
                }
            }
        }
    }

    void image_transform::align_other_to_depth(const uint16_t* z_pixels, const byte* source, byte* dest, int bpp, const rs2_intrinsics& to,
        const rs2_extrinsics& from_to_other)
    {
        switch (to.model)
        {
        case RS2_DISTORTION_MODIFIED_BROWN_CONRADY:
            align_other_to_depth_sse<RS2_DISTORTION_MODIFIED_BROWN_CONRADY>(z_pixels, source, dest, bpp, to, from_to_other);
            break;
        default:
            align_other_to_depth_sse(z_pixels, source, dest, bpp, to, from_to_other);
            break;
        }
    }

    bool is_special_resolution(const rs2_intrinsics& depth, const rs2_intrinsics& to)
    {
        if ((depth.width == 640 && depth.height == 240 && to.width == 320 && to.height == 180) ||
            (depth.width == 640 && depth.height == 480 && to.width == 640 && to.height == 360))
            return true;
        return false;
    }

    template<rs2_distortion dist>
    inline void image_transform::align_depth_to_other_sse(const uint16_t * z_pixels, uint16_t * dest, const rs2_intrinsics& depth, const rs2_intrinsics& to,
        const rs2_extrinsics& from_to_other)
    {
        get_texture_map_sse<dist>(z_pixels, _depth_scale, _depth.height*_depth.width, _pre_compute_map_x_top_left.data(),
            _pre_compute_map_y_top_left.data(), (byte*)_pixel_top_left_int.data(), to, from_to_other);

        float fov[2];
        rs2_fov(&depth, fov);
        float2 pixels_per_angle_depth = { (float)depth.width / fov[0], (float)depth.height / fov[1] };

        rs2_fov(&to, fov);
        float2 pixels_per_angle_target = { (float)to.width / fov[0], (float)to.height / fov[1] };

        if (pixels_per_angle_depth.x < pixels_per_angle_target.x || pixels_per_angle_depth.y < pixels_per_angle_target.y || is_special_resolution(depth, to))
        {
            get_texture_map_sse<dist>(z_pixels, _depth_scale, _depth.height*_depth.width, _pre_compute_map_x_bottom_right.data(),
                _pre_compute_map_y_bottom_right.data(), (byte*)_pixel_bottom_right_int.data(), to, from_to_other);

            move_depth_to_other(z_pixels, dest, to, _pixel_top_left_int, _pixel_bottom_right_int);
        }
        else
        {
            move_depth_to_other(z_pixels, dest, to, _pixel_top_left_int, _pixel_top_left_int);
        }

    }

    template<rs2_distortion dist>
    inline void image_transform::align_other_to_depth_sse(const uint16_t * z_pixels, const byte * source, byte * dest, int bpp, const rs2_intrinsics& to,
        const rs2_extrinsics& from_to_other)
    {
        get_texture_map_sse<dist>(z_pixels, _depth_scale, _depth.height*_depth.width, _pre_compute_map_x_top_left.data(),
            _pre_compute_map_y_top_left.data(), (byte*)_pixel_top_left_int.data(), to, from_to_other);

        std::vector<int2>& bottom_right = _pixel_top_left_int;
        if (to.height < _depth.height && to.width < _depth.width)
        {
            get_texture_map_sse<dist>(z_pixels, _depth_scale, _depth.height*_depth.width, _pre_compute_map_x_bottom_right.data(),
                _pre_compute_map_y_bottom_right.data(), (byte*)_pixel_bottom_right_int.data(), to, from_to_other);

            bottom_right = _pixel_bottom_right_int;
        }

        switch (bpp)
        {
        case 1:
            move_other_to_depth(z_pixels, reinterpret_cast<const bytes<1>*>(source), reinterpret_cast<bytes<1>*>(dest), to,
               _pixel_top_left_int, bottom_right);
            break;
        case 2:
            move_other_to_depth(z_pixels, reinterpret_cast<const bytes<2>*>(source), reinterpret_cast<bytes<2>*>(dest), to,
                _pixel_top_left_int, bottom_right);
            break;
        case 3:
            move_other_to_depth(z_pixels, reinterpret_cast<const bytes<3>*>(source), reinterpret_cast<bytes<3>*>(dest), to,
                _pixel_top_left_int, bottom_right);
            break;
        default:
            break;
        }
    }

    template<class T >
    void image_transform::move_other_to_depth(const uint16_t* z_pixels,
        const T* source,
        T* dest, const rs2_intrinsics& to,
        const std::vector<int2>& pixel_top_left_int,
        const std::vector<int2>& pixel_bottom_right_int)
    {
        // Iterate over the pixels of the depth image
        for (int y = 0; y < _depth.height; ++y)
        {
            for (int x = 0; x < _depth.width; ++x)
            {
                auto depth_pixel_index = y*_depth.width + x;
                // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
                if (z_pixels[depth_pixel_index])
                {
                    for (int other_y = pixel_top_left_int[depth_pixel_index].y; other_y <= pixel_bottom_right_int[depth_pixel_index].y; ++other_y)
                    {
                        for (int other_x = pixel_top_left_int[depth_pixel_index].x; other_x <= pixel_bottom_right_int[depth_pixel_index].x; ++other_x)
                        {
                            if (other_x < 0 || other_y < 0 || other_x >= to.width || other_y >= to.height)
                                continue;
                            auto other_ind = other_y * to.width + other_x;

                            dest[depth_pixel_index] = source[other_ind];
                        }
                    }
                }
            }
        }
    }

#endif

    template<class GET_DEPTH, class TRANSFER_PIXEL>
    void align_images(const rs2_intrinsics& depth_intrin, const rs2_extrinsics& depth_to_other,
        const rs2_intrinsics& other_intrin, GET_DEPTH get_depth, TRANSFER_PIXEL transfer_pixel)
    {
        // Iterate over the pixels of the depth image
#pragma omp parallel for schedule(dynamic)
        for (int depth_y = 0; depth_y < depth_intrin.height; ++depth_y)
        {
            int depth_pixel_index = depth_y * depth_intrin.width;
            for (int depth_x = 0; depth_x < depth_intrin.width; ++depth_x, ++depth_pixel_index)
            {
                // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
                if (float depth = get_depth(depth_pixel_index))
                {
                    // Map the top-left corner of the depth pixel onto the other image
                    float depth_pixel[2] = { depth_x - 0.5f, depth_y - 0.5f }, depth_point[3], other_point[3], other_pixel[2];
                    rs2_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
                    rs2_transform_point_to_point(other_point, &depth_to_other, depth_point);
                    rs2_project_point_to_pixel(other_pixel, &other_intrin, other_point);
                    const int other_x0 = static_cast<int>(other_pixel[0] + 0.5f);
                    const int other_y0 = static_cast<int>(other_pixel[1] + 0.5f);

                    // Map the bottom-right corner of the depth pixel onto the other image
                    depth_pixel[0] = depth_x + 0.5f; depth_pixel[1] = depth_y + 0.5f;
                    rs2_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
                    rs2_transform_point_to_point(other_point, &depth_to_other, depth_point);
                    rs2_project_point_to_pixel(other_pixel, &other_intrin, other_point);
                    const int other_x1 = static_cast<int>(other_pixel[0] + 0.5f);
                    const int other_y1 = static_cast<int>(other_pixel[1] + 0.5f);

                    if (other_x0 < 0 || other_y0 < 0 || other_x1 >= other_intrin.width || other_y1 >= other_intrin.height)
                        continue;

                    // Transfer between the depth pixels and the pixels inside the rectangle on the other image
                    for (int y = other_y0; y <= other_y1; ++y)
                    {
                        for (int x = other_x0; x <= other_x1; ++x)
                        {
                            transfer_pixel(depth_pixel_index, y * other_intrin.width + x);
                        }
                    }
                }
            }
        }
    }

    void align_z_to_other(byte* z_aligned_to_other, const uint16_t* z_pixels, float z_scale, const rs2_intrinsics& z_intrin, const rs2_extrinsics& z_to_other, const rs2_intrinsics& other_intrin)
    {
        auto out_z = (uint16_t *)(z_aligned_to_other);
        align_images(z_intrin, z_to_other, other_intrin,
            [z_pixels, z_scale](int z_pixel_index)
        {
            return z_scale * z_pixels[z_pixel_index];
        },
            [out_z, z_pixels](int z_pixel_index, int other_pixel_index)
        {
            out_z[other_pixel_index] = out_z[other_pixel_index] ?
                std::min((int)out_z[other_pixel_index], (int)z_pixels[z_pixel_index]) :
                z_pixels[z_pixel_index];
        });
    }

   

    template<int N, class GET_DEPTH>
    void align_other_to_depth_bytes(byte* other_aligned_to_depth, GET_DEPTH get_depth, const rs2_intrinsics& depth_intrin, const rs2_extrinsics& depth_to_other, const rs2_intrinsics& other_intrin, const byte* other_pixels)
    {
        auto in_other = (const bytes<N> *)(other_pixels);
        auto out_other = (bytes<N> *)(other_aligned_to_depth);
        align_images(depth_intrin, depth_to_other, other_intrin, get_depth,
            [out_other, in_other](int depth_pixel_index, int other_pixel_index) { out_other[depth_pixel_index] = in_other[other_pixel_index]; });
    }

    template<class GET_DEPTH>
    void align_other_to_depth(byte* other_aligned_to_depth, GET_DEPTH get_depth, const rs2_intrinsics& depth_intrin, const rs2_extrinsics & depth_to_other, const rs2_intrinsics& other_intrin, const byte* other_pixels, rs2_format other_format)
    {
        switch (other_format)
        {
        case RS2_FORMAT_Y8:
            align_other_to_depth_bytes<1>(other_aligned_to_depth, get_depth, depth_intrin, depth_to_other, other_intrin, other_pixels);
            break;
        case RS2_FORMAT_Y16:
        case RS2_FORMAT_Z16:
            align_other_to_depth_bytes<2>(other_aligned_to_depth, get_depth, depth_intrin, depth_to_other, other_intrin, other_pixels);
            break;
        case RS2_FORMAT_RGB8:
        case RS2_FORMAT_BGR8:
            align_other_to_depth_bytes<3>(other_aligned_to_depth, get_depth, depth_intrin, depth_to_other, other_intrin, other_pixels);
            break;
        case RS2_FORMAT_RGBA8:
        case RS2_FORMAT_BGRA8:
            align_other_to_depth_bytes<4>(other_aligned_to_depth, get_depth, depth_intrin, depth_to_other, other_intrin, other_pixels);
            break;
        default:
            assert(false); // NOTE: rs2_align_other_to_depth_bytes<2>(...) is not appropriate for RS2_FORMAT_YUYV/RS2_FORMAT_RAW10 images, no logic prevents U/V channels from being written to one another
        }
    }

    void align_other_to_z(byte* other_aligned_to_z, const uint16_t* z_pixels, float z_scale, const rs2_intrinsics& z_intrin, const rs2_extrinsics& z_to_other, const rs2_intrinsics& other_intrin, const byte* other_pixels, rs2_format other_format)
    {
        align_other_to_depth(other_aligned_to_z, [z_pixels, z_scale](int z_pixel_index) { return z_scale * z_pixels[z_pixel_index]; }, z_intrin, z_to_other, other_intrin, other_pixels, other_format);
    }

    int align::get_unique_id(const std::shared_ptr<stream_profile_interface>& original_profile,
        const std::shared_ptr<stream_profile_interface>& to_profile,
        const std::shared_ptr<stream_profile_interface>& aligned_profile)
    {
        //align_stream_unique_ids holds a cache of mapping between the 2 streams that created the new aligned stream
        // to it stream id.
        //When an aligned frame is created from other streams (but with the same instance of this class)
        // the from_to pair will be different so a new id will be added to the cache.
        //This allows the user to pass different streams to this class and for every pair of from_to
        //the user will always get the same stream id for the aligned stream.
        auto from_to = std::make_pair(original_profile->get_unique_id(), to_profile->get_unique_id());
        auto it = align_stream_unique_ids.find(from_to);
        if (it != align_stream_unique_ids.end())
        {
            return it->second;
        }
        else
        {
            int new_id = aligned_profile->get_unique_id();
            align_stream_unique_ids[from_to] = new_id;
            return new_id;
        }
    }
    std::shared_ptr<stream_profile_interface> align::create_aligned_profile(
        const std::shared_ptr<stream_profile_interface>& original_profile,
        const std::shared_ptr<stream_profile_interface>& to_profile)
    {
        auto aligned_profile = original_profile->clone();
        int aligned_unique_id = get_unique_id(original_profile, to_profile, aligned_profile);
        aligned_profile->set_unique_id(aligned_unique_id);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*aligned_profile, *original_profile);
        aligned_profile->set_stream_index(original_profile->get_stream_index());
        aligned_profile->set_stream_type(original_profile->get_stream_type());
        aligned_profile->set_format(original_profile->get_format());
        aligned_profile->set_framerate(original_profile->get_framerate());
        if (auto original_video_profile = As<video_stream_profile_interface>(original_profile))
        {
            if (auto to_video_profile = As<video_stream_profile_interface>(to_profile))
            {
                if (auto aligned_video_profile = As<video_stream_profile_interface>(aligned_profile))
                {
                    aligned_video_profile->set_dims(to_video_profile->get_width(), to_video_profile->get_height());
                    auto aligned_intrinsics = original_video_profile->get_intrinsics();
                    aligned_intrinsics.width = to_video_profile->get_width();
                    aligned_intrinsics.height = to_video_profile->get_height();
                    aligned_video_profile->set_intrinsics([aligned_intrinsics]() { return aligned_intrinsics; });
                }
            }
        }
        return aligned_profile;
    }
    void align::on_frame(frame_holder frameset, librealsense::synthetic_source_interface* source)
    {
        auto composite = As<composite_frame>(frameset.frame);
        if (composite == nullptr)
        {
            LOG_WARNING("Trying to align a non composite frame");
            return;
        }

        if (composite->get_embedded_frames_count() < 2)
        {
            LOG_WARNING("Trying to align a single frame");
            return;
        }

        librealsense::video_frame* depth_frame = nullptr;
        std::vector<librealsense::video_frame*> other_frames;
        //Find the depth frame
        for (int i = 0; i < composite->get_embedded_frames_count(); i++)
        {
            frame_interface* f = composite->get_frame(i);
            if (f->get_stream()->get_stream_type() == RS2_STREAM_DEPTH)
            {
                assert(depth_frame == nullptr); // Trying to align multiple depth frames is not supported, in release we take the last one
                depth_frame = As<librealsense::video_frame>(f);
                if (depth_frame == nullptr)
                {
                    LOG_ERROR("Given depth frame is not a librealsense::video_frame");
                    return;
                }
            }
            else
            {
                auto other_video_frame = As<librealsense::video_frame>(f);
                auto other_stream_profile = f->get_stream();
                assert(other_stream_profile != nullptr);

                if (other_video_frame == nullptr)
                {
                    LOG_ERROR("Given frame of type " << other_stream_profile->get_stream_type() << ", is not a librealsense::video_frame, ignoring it");
                    return;
                }

                if (_to_stream_type == RS2_STREAM_DEPTH)
                {
                    //In case of alignment to depth, we will align any image given in the frameset to the depth one
                    other_frames.push_back(other_video_frame);
                }
                else
                {
                    //In case of alignment from depth to other, we only want the other frame with the stream type that was requested to align to
                    if (other_stream_profile->get_stream_type() == _to_stream_type)
                    {
                        assert(other_frames.size() == 0); // Trying to align depth to multiple frames is not supported, in release we take the last one
                        other_frames.push_back(other_video_frame);
                    }
                }
            }
        }

        if (depth_frame == nullptr)
        {
            LOG_WARNING("No depth frame provided to align");
            return;
        }

        if (other_frames.empty())
        {
            LOG_WARNING("Only depth frame provided to align");
            return;
        }

        auto depth_profile = As<video_stream_profile_interface>(depth_frame->get_stream());
        if (depth_profile == nullptr)
        {
            LOG_ERROR("Depth profile is not a video stream profile");
            return;
        }
        rs2_intrinsics depth_intrinsics = depth_profile->get_intrinsics();
        std::vector<frame_holder> output_frames;

        if (_to_stream_type == RS2_STREAM_DEPTH)
        {
            //Storing the original depth frame for output frameset
            depth_frame->acquire();
            output_frames.push_back(depth_frame);
        }

        for (librealsense::video_frame* other_frame : other_frames)
        {
            auto other_profile = As<video_stream_profile_interface>(other_frame->get_stream());
            if (other_profile == nullptr)
            {
                LOG_WARNING("Other frame with type " << other_frame->get_stream()->get_stream_type() << ", is not a video stream profile. Ignoring it");
                continue;
            }

            rs2_intrinsics other_intrinsics = other_profile->get_intrinsics();
            rs2_extrinsics depth_to_other_extrinsics{};
            if (!environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(*depth_profile, *other_profile, &depth_to_other_extrinsics))
            {
                LOG_WARNING("Failed to get extrinsics from depth to " << other_profile->get_stream_type() << ", ignoring it");
                continue;
            }

            auto sensor = depth_frame->get_sensor();
            if (sensor == nullptr)
            {
                LOG_ERROR("Failed to get sensor from depth frame");
                return;
            }

            if (sensor->supports_option(RS2_OPTION_DEPTH_UNITS) == false)
            {
                LOG_ERROR("Sensor of depth frame does not provide depth units");
                return;
            }

            float depth_scale = sensor->get_option(RS2_OPTION_DEPTH_UNITS).query();

            frame_holder aligned_frame{ nullptr };
            if (_to_stream_type == RS2_STREAM_DEPTH)
            {
                //Align a stream to depth
                auto aligned_bytes_per_pixel = other_frame->get_bpp() / 8;
                auto aligned_profile = create_aligned_profile(other_profile, depth_profile);
                aligned_frame = source->allocate_video_frame(
                    aligned_profile,
                    other_frame,
                    aligned_bytes_per_pixel,
                    depth_frame->get_width(),
                    depth_frame->get_height(),
                    depth_frame->get_width() * aligned_bytes_per_pixel,
                    RS2_EXTENSION_VIDEO_FRAME);

                if (aligned_frame == nullptr)
                {
                    LOG_ERROR("Failed to allocate frame for aligned output");
                    return;
                }

                byte* other_aligned_to_depth = const_cast<byte*>(aligned_frame.frame->get_frame_data());
                memset(other_aligned_to_depth, 0, depth_intrinsics.height * depth_intrinsics.width * aligned_bytes_per_pixel);
#ifdef __SSSE3__
                if (_stream_transform == nullptr)
                {
                    _stream_transform = std::make_shared<image_transform>(depth_intrinsics,
                        depth_scale);

                    _stream_transform->pre_compute_x_y_map_corners();
                }

                _stream_transform->align_other_to_depth(reinterpret_cast<const uint16_t*>(depth_frame->get_frame_data()),
                    reinterpret_cast<const byte*>(other_frame->get_frame_data()),
                    other_aligned_to_depth, other_frame->get_bpp()/8,
                    other_intrinsics,
                    depth_to_other_extrinsics);
#else
                align_other_to_z(other_aligned_to_depth,
                    reinterpret_cast<const uint16_t*>(depth_frame->get_frame_data()),
                    depth_scale,
                    depth_intrinsics,
                    depth_to_other_extrinsics,
                    other_intrinsics,
                    other_frame->get_frame_data(),
                    other_profile->get_format());
#endif
            }
            else
            {
                //Align depth to some stream
                auto aligned_bytes_per_pixel = depth_frame->get_bpp() / 8;
                auto aligned_profile = create_aligned_profile(depth_profile, other_profile);
                aligned_frame = source->allocate_video_frame(
                    aligned_profile,
                    depth_frame,
                    aligned_bytes_per_pixel,
                    other_intrinsics.width,
                    other_intrinsics.height,
                    other_intrinsics.width * aligned_bytes_per_pixel,
                    RS2_EXTENSION_DEPTH_FRAME);

                if (aligned_frame == nullptr)
                {
                    LOG_ERROR("Failed to allocate frame for aligned output");
                    return;
                }
                byte* z_aligned_to_other = const_cast<byte*>(aligned_frame.frame->get_frame_data());
                memset(z_aligned_to_other, 0, other_intrinsics.height * other_intrinsics.width * aligned_bytes_per_pixel);
                auto data = (int16_t*)depth_frame->get_frame_data();

#ifdef __SSSE3__
                if (_stream_transform == nullptr)
                {
                    _stream_transform = std::make_shared<image_transform>(depth_intrinsics,
                        depth_scale);

                    _stream_transform->pre_compute_x_y_map_corners();
                }
                _stream_transform->align_depth_to_other(reinterpret_cast<const uint16_t*>(depth_frame->get_frame_data()),
                    reinterpret_cast<uint16_t*>(z_aligned_to_other), depth_frame->get_bpp() / 8,
                    depth_intrinsics, other_intrinsics,
                    depth_to_other_extrinsics);

#else
                    align_z_to_other(z_aligned_to_other,
                        reinterpret_cast<const uint16_t*>(depth_frame->get_frame_data()),
                        depth_scale,
                        depth_intrinsics,
                        depth_to_other_extrinsics,
                        other_intrinsics);
#endif
                assert(output_frames.size() == 0); //When aligning depth to other, only 2 frames are expected in the output.
                other_frame->acquire();
                output_frames.push_back(other_frame);
            }
            output_frames.push_back(std::move(aligned_frame));
        }
        auto new_composite = source->allocate_composite_frame(std::move(output_frames));
        source->frame_ready(std::move(new_composite));
    }

    align::align(rs2_stream to_stream) : _to_stream_type(to_stream)
    {
        auto cb = [this](frame_holder frameset, librealsense::synthetic_source_interface* source) { on_frame(std::move(frameset), source); };
        auto callback = new internal_frame_processor_callback<decltype(cb)>(cb);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }

        }