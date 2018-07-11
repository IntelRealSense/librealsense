// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>
#include "core/processing.h"
#include "proc/synthetic-stream.h"
#include "image.h"
#include "source.h"

namespace librealsense
{
    class image_transform
    {
    public:

        image_transform(const rs2_intrinsics& from,
            float depth_scale);

        inline void align_depth_to_other(const uint16_t* z_pixels,
            uint16_t* dest, int bpp, const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);

        inline void align_other_to_depth(const uint16_t* z_pixels,
            const byte* source,
            byte* dest, int bpp, const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);

        void pre_compute_x_y_map(float offset = 0);
        void pre_compute_x_y_map_corners();

    private:

        const rs2_intrinsics _depth;
        float _depth_scale;

        std::vector<float> _pre_compute_map_x;
        std::vector<float> _pre_compute_map_y;
        std::vector<float> _pre_compute_map_x_top_left;
        std::vector<float> _pre_compute_map_y_top_left;
        std::vector<float> _pre_compute_map_x_bottom_right;
        std::vector<float> _pre_compute_map_y_bottom_right;

        std::vector<int2> _pixel_top_left_int;
        std::vector<int2> _pixel_bottom_right_int;

        void pre_compute_x_y_map(std::vector<float>& pre_compute_map_x,
            std::vector<float>& pre_compute_map_y,
            float offset = 0);

        template<rs2_distortion dist = RS2_DISTORTION_NONE>
        inline void align_depth_to_other_sse(const uint16_t* z_pixels,
            uint16_t* dest, const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);

        template<rs2_distortion dist = RS2_DISTORTION_NONE>
        inline void align_other_to_depth_sse(const uint16_t* z_pixels,
            const byte* source,
            byte* dest, int bpp, const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);

        inline void move_depth_to_other(const uint16_t* z_pixels,
            uint16_t* dest, const rs2_intrinsics& to,
            const std::vector<int2>& pixel_top_left_int,
            const std::vector<int2>& pixel_bottom_right_int);

        template<class T >
        inline void move_other_to_depth(const uint16_t* z_pixels,
            const T* source,
            T* dest, const rs2_intrinsics& to,
            const std::vector<int2>& pixel_top_left_int,
            const std::vector<int2>& pixel_bottom_right_int);

        template<rs2_distortion dist>
        inline void distorte_x_y(const __m128& x, const __m128& y, __m128* distorted_x, __m128* distorted_y, const rs2_intrinsics& to);

        template<rs2_distortion dist>
        inline void get_texture_map_sse(const uint16_t * depth,
            const unsigned int size,
            const float * pre_compute_x,
            const float * pre_compute_y,
            byte* pixels_ptr_int, const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);
    };

    class align : public processing_block
    {
    public:
        align(rs2_stream align_to);

    private:
        void on_frame(frame_holder frameset, librealsense::synthetic_source_interface* source);
        std::shared_ptr<stream_profile_interface> create_aligned_profile(
            const std::shared_ptr<stream_profile_interface>& original_profile,
            const std::shared_ptr<stream_profile_interface>& to_profile);
        int get_unique_id(const std::shared_ptr<stream_profile_interface>& original_profile,
            const std::shared_ptr<stream_profile_interface>& to_profile,
            const std::shared_ptr<stream_profile_interface>& aligned_profile);
        rs2_stream _to_stream_type;
        std::map<std::pair<int, int>, int> align_stream_unique_ids;
        std::shared_ptr<image_transform> _stream_transform;
    };
}
