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
#ifdef __SSSE3__
    class image_transform
    {
    public:

        image_transform(const rs2_intrinsics& from,
            float depth_scale);

        inline void align_depth_to_other(const uint16_t* z_pixels,
            uint16_t* dest, int bpp, 
            const rs2_intrinsics& depth,
            const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);

        inline void align_other_to_depth(const uint16_t* z_pixels,
            const byte* source,
            byte* dest, int bpp, const rs2_intrinsics& to,
            const rs2_extrinsics& from_to_other);

        void pre_compute_x_y_map_corners();

    private:

        const rs2_intrinsics _depth;
        float _depth_scale;

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
            uint16_t* dest, const rs2_intrinsics& depth,
            const rs2_intrinsics& to,
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

    };
#endif

    class align : public generic_processing_block
    {
    public:
        align(rs2_stream to_stream) : _to_stream_type(to_stream){}

    protected:
        bool should_process(const rs2::frame& frame) override;
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        std::shared_ptr<rs2::video_stream_profile> create_aligned_profile(
            rs2::video_stream_profile& original_profile,
            rs2::video_stream_profile& to_profile);
        int get_unique_id(rs2::video_stream_profile& original_profile,
            rs2::video_stream_profile& to_profile,
            rs2::video_stream_profile& aligned_profile);
        rs2_stream _to_stream_type;
        std::map<std::pair<int, int>, int> _align_stream_unique_ids;
        std::pair<int, int> _prev_depth_res;

#ifdef __SSSE3__
        std::shared_ptr<image_transform> _stream_transform;
#endif
    };
}
