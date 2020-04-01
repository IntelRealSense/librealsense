// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "proc/synthetic-stream.h"
#include "proc/occlusion-filter.h"
#include  "../../common/tiny-profiler.h"
#include <vector>
#include <cmath>

#define ROTATION_BUFFER_SIZE 8
#define VERTICAL_SCAN_WINDOW_SIZE 64
#define DEPTH_OCCLUSION_THRESHOLD 1

namespace librealsense
{
    occlusion_filter::occlusion_filter() : _occlusion_filter(occlusion_monotonic_scan) , _occlusion_scanning(horizontal)
    {
    }

    void occlusion_filter::set_texel_intrinsics(const rs2_intrinsics& in)
    {
        _texels_intrinsics = in;
        _texels_depth.resize(_texels_intrinsics.value().width*_texels_intrinsics.value().height);
    }

   void occlusion_filter::process(float3* points, float2* uv_map, const std::vector<float2> & pix_coord, const rs2::depth_frame& depth) const
    {
        switch (_occlusion_filter)
        {
        case occlusion_none:
            break;
        case occlusion_monotonic_scan:
            monotonic_heuristic_invalidation(points, uv_map, pix_coord, depth);
            break;
        default:
            throw std::runtime_error(to_string() << "Unsupported occlusion filter type " << _occlusion_filter << " requested");
            break;
        }
    }

    template<size_t SIZE>
    void rotate_image_counterclockwise(byte* dest[], const byte* source, int width, int height)
    {
        {
            scoped_timer t1("Rotation Time - counterclockwise");
            auto width_out = height;
            auto height_out = width;

            auto out = dest[0];
            byte buffer[ROTATION_BUFFER_SIZE][ROTATION_BUFFER_SIZE * SIZE]; // = { 0 };
            for (int i = 0; i <= height - ROTATION_BUFFER_SIZE; i = i + ROTATION_BUFFER_SIZE)
            {
                for (int j = 0; j <= width - ROTATION_BUFFER_SIZE; j = j + ROTATION_BUFFER_SIZE)
                {
                    for (int ii = 0; ii < ROTATION_BUFFER_SIZE; ++ii)
                    {
                        auto out_index = (((height_out - ROTATION_BUFFER_SIZE - j + 1) * width_out) - i - ROTATION_BUFFER_SIZE + (ii)*width_out);
                        memcpy(&(buffer[ii]), &source[(out_index)*SIZE], ROTATION_BUFFER_SIZE * SIZE);
                    }
                    for (int ii = 0; ii < ROTATION_BUFFER_SIZE; ++ii)
                    {
                        for (int jj = 0; jj < ROTATION_BUFFER_SIZE; ++jj)
                        {
                            auto source_index = ((j + jj) + (width * (i + ii))) * SIZE;
                            memcpy(&out[source_index], (void*)(&buffer[ROTATION_BUFFER_SIZE - 1 - jj][(ROTATION_BUFFER_SIZE - 1 - ii) * SIZE]), SIZE);
                        }
                    }

                }
            }
        }
    }
    // IMPORTANT! This implementation is based on the assumption that the RGB sensor is positioned strictly to the left of the depth sensor.
    // namely D415/D435 and SR300. The implementation WILL NOT work properly for different setups
    // Heuristic occlusion invalidation algorithm:
    // -  Use the uv texels calculated when projecting depth to color
    // -  Scan each line from left to right and check the the U coordinate in the mapping is raising monotonically.
    // -  The occlusion is designated as U coordinate for a given pixel is less than the U coordinate of the predecessing pixel.
    // -  The UV mapping for the occluded pixel is reset to (0,0). Later on the (0,0) coordinate in the texture map is overwritten
    //    with a invalidation color such as black/magenta according to the purpose (production/debugging)
    void occlusion_filter::monotonic_heuristic_invalidation(float3* points, float2* uv_map, const std::vector<float2>& pix_coord, const rs2::depth_frame& depth) const
    {
        float occZTh = 0.1f; //meters
        int occDilationSz = 1;
        auto points_width = _depth_intrinsics->width;
        auto points_height = _depth_intrinsics->height;
        auto pixels_ptr = pix_coord.data();
        auto points_ptr = points;
        auto uv_map_ptr = uv_map;
        float maxInLine = -1;
        float maxZ = 0;

        auto frame_size = static_cast<int>(_depth_intrinsics->width * _depth_intrinsics->height);
        std::allocator<byte> alloc;
        byte* depth_planes[1];

        if (_occlusion_scanning == horizontal)
        {

            {
                scoped_timer t1("Horizontal Scan");
                for (size_t y = 0; y < points_height; ++y)
                {

                    maxInLine = -1;
                    maxZ = 0;
                    int occDilationLeft = 0;

                    for (size_t x = 0; x < points_width; ++x)
                    {
                        if (points_ptr->z)
                        {
                            //Occlusion detection
                            if (pixels_ptr->x < maxInLine || (pixels_ptr->x == maxInLine && (points_ptr->z - maxZ) > occZTh))
                            {
                                *points_ptr = { 0, 0, 0 };
                                occDilationLeft = occDilationSz;
                            }
                            else
                            {
                                maxInLine = pixels_ptr->x;
                                maxZ = points_ptr->z;
                                if (occDilationLeft > 0)
                                {
                                    *points_ptr = { 0, 0, 0 };
                                    occDilationLeft--;
                                }
                            }
                        }
                        ++points_ptr;
                        ++uv_map_ptr;
                        ++pixels_ptr;
                    }
                }
            }
        }
        else if (_occlusion_scanning == vertical)
        {
            depth_planes[0] = (byte*)alloc.allocate(depth.get_bytes_per_pixel() * frame_size);
            int bpp = depth.get_bytes_per_pixel();
            rotate_image_counterclockwise<2>(depth_planes, (const byte*)(depth.get_data()), points_width, points_height);

            // scan depth frame after clockwise rotation: check if there is a significant jump between adjacen pixels in Z-axis (depth), it means there could be occlusion.
            // scan over a small window so that points depth is the same
            // save suspected points and run occlusion-invalidation vertical scan only on them
            // after rotation : height = points_width , width = points_height

            auto rotated_depth_width = _depth_intrinsics->height;
            auto rotated_depth_height = _depth_intrinsics->width;

            for (int i = 0; i < rotated_depth_height; i++)
            {
                for (int j = 0; j < rotated_depth_width - VERTICAL_SCAN_WINDOW_SIZE; j = j + VERTICAL_SCAN_WINDOW_SIZE)
                {
                    // before rotation: occlusion detected in the positive direction of Y
                    // after counterclockwise rotation : scan from right to left (positive direction of X) to detect occlusion 
                    // compare depth each pixel only with the pixel on its right (i,j+1) 
                    auto index = i * rotated_depth_width + j;
                    auto uv_i = j;
                    auto uv_j = rotated_depth_height - i;
                    auto uv_index = uv_i * rotated_depth_height + uv_j; // 90 degrees rotation transform from rotated depth
                    auto index_right = index + 1;
                    float diff_right = abs((depth_planes[0])[index] - (depth_planes[0])[index_right]);

                    if ((diff_right > DEPTH_OCCLUSION_THRESHOLD))
                    {
                        pixels_ptr = pix_coord.data() + uv_index;
                        points_ptr = points + uv_index;
                        uv_map_ptr = uv_map + uv_index;
                        maxInLine = -1;
                        maxZ = 0;
                        int occDilationUp = 0;

                        for (size_t y = 0; y <= VERTICAL_SCAN_WINDOW_SIZE; ++y)
                        {
                            if (((pixels_ptr + y * points_width)->y < maxInLine))
                            {
                                *(points_ptr + y * points_width) = { 0.f, 0.f, 0.f };
                            }
                            else
                            {
                                maxInLine = (pixels_ptr + y * points_width)->y;
                            }

                        }
                    }
                }
            }
        }
    }
    // Prepare texture map without occlusion that for every texture coordinate there no more than one depth point that is mapped to it
    // i.e. for every (u,v) map coordinate we select the depth point with minimum Z. all other points that are mapped to this texel will be invalidated
    // Algo input data:
    // Vector of 3D [xyz] coordinates of depth_width*depth_height size
    // Vector of 2D [i,j] coordinates where the val[i,j] stores the texture coordinate (s,t) for the corresponding (i,j) pixel in depth frame
    // Algo intermediate data:
    // Vector of depth values (floats) in size of the mapped texture (different from depth width*height) where
    // each (i,j) cell holds the minimal Z among all the depth pixels that are mapped to the specific texel
    void occlusion_filter::comprehensive_invalidation(float3* points, float2* uv_map, const std::vector<float2> & pix_coord) const
    {
        auto depth_points = points;
        auto mapped_pix = pix_coord.data();
        size_t mapped_tex_width = _texels_intrinsics->width;
        size_t mapped_tex_height = _texels_intrinsics->height;
        size_t points_width = _depth_intrinsics->width;
        size_t points_height = _depth_intrinsics->height;

        static const float z_threshold = 0.05f; // Compensate for temporal noise when comparing Z values

        // Clear previous data
        memset((void*)(_texels_depth.data()), 0, _texels_depth.size() * sizeof(float));

        // Pass1 -generate texels mapping with minimal depth for each texel involved
        for (size_t i = 0; i < points_height; i++)
        {
            for (size_t j = 0; j < points_width; j++)
            {
                if ((depth_points->z > 0.0001f) &&
                    (mapped_pix->x > 0.f) && (mapped_pix->x < mapped_tex_width) &&
                    (mapped_pix->y > 0.f) && (mapped_pix->y < mapped_tex_height))
                {
                    size_t texel_index = (size_t)(mapped_pix->y)*mapped_tex_width + (size_t)(mapped_pix->x);

                    if ((_texels_depth[texel_index] < 0.0001f) || ((_texels_depth[texel_index] + z_threshold) > depth_points->z))
                    {
                        _texels_depth[texel_index] = depth_points->z;
                    }
                }

                ++depth_points;
                ++mapped_pix;
            }
        }

        mapped_pix = pix_coord.data();
        depth_points = points;
        auto uv_ptr = uv_map;

        // Pass2 -invalidate depth texels with occlusion traits
        for (size_t i = 0; i < points_height; i++)
        {
            for (size_t j = 0; j < points_width; j++)
            {
                if ((depth_points->z > 0.0001f) &&
                    (mapped_pix->x > 0.f) && (mapped_pix->x < mapped_tex_width) &&
                    (mapped_pix->y > 0.f) && (mapped_pix->y < mapped_tex_height))
                {
                    size_t texel_index = (size_t)(mapped_pix->y)*mapped_tex_width + (size_t)(mapped_pix->x);

                    if ((_texels_depth[texel_index] > 0.0001f) && ((_texels_depth[texel_index] + z_threshold) < depth_points->z))
                    {
                        *uv_ptr = { 0.f, 0.f };
                    }
                }

                ++depth_points;
                ++mapped_pix;
                ++uv_ptr;
            }
        }
    }
}
