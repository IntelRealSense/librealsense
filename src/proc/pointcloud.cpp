// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "environment.h"
#include "proc/occlusion-filter.h"
#include "proc/pointcloud.h"
#include "option.h"
#include "environment.h"
#include "context.h"

#include <iostream>

#ifdef RS2_USE_CUDA
#include "../cuda/cuda-pointcloud.cuh"
#endif
#ifdef __SSSE3__
#include <tmmintrin.h> // For SSSE3 intrinsics
#endif


namespace librealsense
{
    template<class MAP_DEPTH> void deproject_depth(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, MAP_DEPTH map_depth)
    {
        for (int y = 0; y < intrin.height; ++y)
        {
            for (int x = 0; x < intrin.width; ++x)
            {
                const float pixel[] = { (float)x, (float)y };
                rs2_deproject_pixel_to_point(points, &intrin, pixel, map_depth(*depth++));
                points += 3;
            }
        }
    }

    const float3 * depth_to_points(uint8_t* image, const rs2_intrinsics &depth_intrinsics, const uint16_t * depth_image, float depth_scale)
    {
#ifdef RS2_USE_CUDA
        rscuda::deproject_depth_cuda(reinterpret_cast<float *>(image), depth_intrinsics, depth_image, depth_scale);
#else
        deproject_depth(reinterpret_cast<float *>(image), depth_intrinsics, depth_image, [depth_scale](uint16_t z) { return depth_scale * z; });
#endif
        return reinterpret_cast<float3 *>(image);
    }

    float3 transform(const rs2_extrinsics *extrin, const float3 &point) { float3 p = {}; rs2_transform_point_to_point(&p.x, extrin, &point.x); return p; }
    float2 project(const rs2_intrinsics *intrin, const float3 & point) { float2 pixel = {}; rs2_project_point_to_pixel(&pixel.x, intrin, &point.x); return pixel; }
    float2 pixel_to_texcoord(const rs2_intrinsics *intrin, const float2 & pixel) { return{ pixel.x / (intrin->width), pixel.y / (intrin->height) }; }
    float2 project_to_texcoord(const rs2_intrinsics *intrin, const float3 & point) { return pixel_to_texcoord(intrin, project(intrin, point)); }

    void pointcloud::set_extrinsics()
    {
        if (_output_stream && _other_stream && !_extrinsics)
        {
            rs2_extrinsics ex;
            const rs2_stream_profile* ds = _output_stream;
            const rs2_stream_profile* os = _other_stream.get_profile();
            if (environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(
                *ds->profile, *os->profile, &ex))
            {
                _extrinsics = ex;
            }
        }
    }

    void pointcloud::inspect_depth_frame(const rs2::frame& depth)
    {
        if (!_output_stream || _depth_stream.get_profile().get() != depth.get_profile().get())
        {
            _output_stream = depth.get_profile().as<rs2::video_stream_profile>().clone(
                RS2_STREAM_DEPTH, depth.get_profile().stream_index(), RS2_FORMAT_XYZ32F);
            _depth_stream = depth;
            _depth_intrinsics = optional_value<rs2_intrinsics>();
            _depth_units = optional_value<float>();
            _extrinsics = optional_value<rs2_extrinsics>();
        }

        bool found_depth_intrinsics = false;
        bool found_depth_units = false;

        if (!_depth_intrinsics)
        {
            auto stream_profile = depth.get_profile();
            if (auto video = stream_profile.as<rs2::video_stream_profile>())
            {
                _depth_intrinsics = video.get_intrinsics();
                _pixels_map.resize(_depth_intrinsics->height*_depth_intrinsics->width);
                _occlusion_filter->set_depth_intrinsics(_depth_intrinsics.value());
#ifdef __SSSE3__
                pre_compute_x_y_map(); //compute the x and y map once for optimization
#endif
                found_depth_intrinsics = true;
            }
        }

        if (!_depth_units)
        {
            auto sensor = ((frame_interface*)depth.get())->get_sensor().get();
            _depth_units = sensor->get_option(RS2_OPTION_DEPTH_UNITS).query();
            found_depth_units = true;
        }

        set_extrinsics();
    }

    void pointcloud::inspect_other_frame(const rs2::frame& other)
    {
        if (_stream_filter != _prev_stream_filter)
        {
            _prev_stream_filter = _stream_filter;
        }

        if (_extrinsics.has_value() && other.get_profile().get() == _other_stream.get_profile().get())
            return;

        _other_stream = other;
        _other_intrinsics = optional_value<rs2_intrinsics>();
        _extrinsics = optional_value<rs2_extrinsics>();

        if (!_other_intrinsics)
        {
            auto stream_profile = _other_stream.get_profile();
            if (auto video = stream_profile.as<rs2::video_stream_profile>())
            {
                _other_intrinsics = video.get_intrinsics();
                _occlusion_filter->set_texel_intrinsics(_other_intrinsics.value());
            }
        }

        set_extrinsics();
    }

    void pointcloud::pre_compute_x_y_map()
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

#ifdef __SSSE3__
    const float3* get_points_sse(const uint16_t* depth,
        const unsigned int size,
        float* pre_compute_x,
        float* pre_compute_y,
        float depth_scale,
        float3* points)
    {
        auto point = reinterpret_cast<float*>(points);

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
        return points;
    }

    void get_texture_map_sse(const float3* points,
        const unsigned int width,
        const unsigned int height,
        const rs2_intrinsics &other_intrinsics,
        const rs2_extrinsics& extr,
        float2* tex_ptr,
        float2* pixels_ptr)
    {
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
        auto mask_brown_conrady = _mm_set_ps1(RS2_DISTORTION_MODIFIED_BROWN_CONRADY);
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

            auto cmp = _mm_cmpeq_ps(mask_brown_conrady, dist);

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
    }

#endif

    void get_texture_map(const float3* points,
        const unsigned int width,
        const unsigned int height,
        const rs2_intrinsics &other_intrinsics,
        const rs2_extrinsics& extr,
        float2* tex_ptr,
        float2* pixels_ptr)
    {
        for (unsigned int y = 0; y < height; ++y)
        {
            for (unsigned int x = 0; x < width; ++x)
            {
                if (points->z)
                {
                    auto trans = transform(&extr, *points);
                    //auto tex_xy = project_to_texcoord(&mapped_intr, trans);
                    // Store intermediate results for poincloud filters
                    *pixels_ptr = project(&other_intrinsics, trans);
                    auto tex_xy = pixel_to_texcoord(&other_intrinsics, *pixels_ptr);

                    *tex_ptr = tex_xy;
                }
                else
                {
                    *tex_ptr = { 0.f, 0.f };
                    *pixels_ptr = { 0.f, 0.f };
                }
                ++points;
                ++tex_ptr;
                ++pixels_ptr;
            }
        }
    }

    rs2::frame pointcloud::process_depth_frame(const rs2::frame_source& source, const rs2::depth_frame& depth)
    {
        auto res = source.allocate_points(_output_stream, depth);
        auto pframe = (librealsense::points*)(res.get());

        auto depth_data = (const uint16_t*)depth.get_data();

        const float3* points;
#ifdef __SSSE3__
        points = get_points_sse(depth_data, _depth_intrinsics->height*_depth_intrinsics->width, _pre_compute_map_x.data(), _pre_compute_map_y.data(), *_depth_units, pframe->get_vertices());
#else
        points = depth_to_points((uint8_t*)pframe->get_vertices(), *_depth_intrinsics, depth_data, *_depth_units);
#endif

        auto vid_frame = depth.as<rs2::video_frame>();
        float2* tex_ptr = pframe->get_texture_coordinates();
        // Pixels calculated in the mapped texture. Used in post-processing filters
        float2* pixels_ptr = _pixels_map.data();
        rs2_intrinsics mapped_intr;
        rs2_extrinsics extr;
        bool map_texture = false;
        {
            if (_extrinsics && _other_intrinsics)
            {
                mapped_intr = *_other_intrinsics;
                extr = *_extrinsics;
                map_texture = true;
            }
        }

        if (map_texture)
        {
            auto height = vid_frame.get_height();
            auto width = vid_frame.get_width();

#ifdef __SSSE3__
            get_texture_map_sse(points, width, height, mapped_intr, extr, tex_ptr, pixels_ptr);
#else
            get_texture_map(points, width, height, mapped_intr, extr, tex_ptr, pixels_ptr);
#endif

            if (_occlusion_filter->active())
            {
                _occlusion_filter->process(pframe->get_vertices(), pframe->get_texture_coordinates(), _pixels_map);
            }
        }
        return res;
    }

    pointcloud::pointcloud()
    {
        _occlusion_filter = std::make_shared<occlusion_filter>();

        auto occlusion_invalidation = std::make_shared<ptr_option<uint8_t>>(
            occlusion_none,
            occlusion_max - 1, 1,
            occlusion_none,
            (uint8_t*)&_occlusion_filter->_occlusion_filter,
            "Occlusion removal");
        occlusion_invalidation->on_set([this, occlusion_invalidation](float val)
        {
            if (!occlusion_invalidation->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported occlusion filtering requiested " << val << " is out of range.");

            _occlusion_filter->set_mode(static_cast<uint8_t>(val));

        });

        occlusion_invalidation->set_description(0.f, "Off");
        occlusion_invalidation->set_description(1.f, "Heuristic");
        occlusion_invalidation->set_description(2.f, "Exhaustive");
        register_option(RS2_OPTION_FILTER_MAGNITUDE, occlusion_invalidation);
    }

    bool pointcloud::should_process(const rs2::frame& frame)
    {
        if (!frame)
            return false;

        auto set = frame.as<rs2::frameset>();

        if (set)
        {
            //process composite frame only if it contains both a depth frame and the requested texture frame
            if (_stream_filter.stream == RS2_STREAM_ANY)
                return false;

            auto tex = set.first_or_default(_stream_filter.stream, _stream_filter.format);
            if (!tex)
                return false;
            auto depth = set.first_or_default(RS2_STREAM_DEPTH, RS2_FORMAT_Z16);
            if (!depth)
                return false;
        }
        else
        {
            if (frame.get_profile().stream_type() == RS2_STREAM_DEPTH && frame.get_profile().format() == RS2_FORMAT_Z16)
                return true;

            auto p = frame.get_profile();
            if (p.stream_type() == _stream_filter.stream && p.format() == _stream_filter.format && p.stream_index() == _stream_filter.index)
                return true;
            return false;

            //TODO: switch to this code when map_to api is removed
            //if (_stream_filter != RS2_STREAM_ANY)
            //    return false;
            //process single frame only if it is a depth frame
            //if (frame.get_profile().stream_type() != RS2_STREAM_DEPTH || frame.get_profile().format() != RS2_FORMAT_Z16)
            //    return false;
        }

        return true;
    }

    rs2::frame pointcloud::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        rs2::frame rv;
        if (auto composite = f.as<rs2::frameset>())
        {
            auto texture = composite.first(_stream_filter.stream);
            inspect_other_frame(texture);

            auto depth = composite.first(RS2_STREAM_DEPTH, RS2_FORMAT_Z16);
            inspect_depth_frame(depth);
            rv = process_depth_frame(source, depth);
        }
        else
        {
            if (f.is<rs2::depth_frame>())
            {
                inspect_depth_frame(f);
                rv = process_depth_frame(source, f);
            }
            if (f.get_profile().stream_type() == _stream_filter.stream && f.get_profile().format() == _stream_filter.format)
            {
                inspect_other_frame(f);
            }
        }
        return rv;
    }
}
