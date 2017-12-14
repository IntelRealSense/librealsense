// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../include/librealsense2/hpp/rs_frame.hpp"
namespace librealsense
{

    class pointcloud : public processing_block
    {
    public:
        pointcloud();

    private:
        std::mutex              _mutex;

        const rs2_intrinsics*   _depth_intrinsics_ptr;
        const float*            _depth_units_ptr;
        const rs2_intrinsics*   _mapped_intrinsics_ptr;
        const rs2_extrinsics*   _extrinsics_ptr;

        rs2_intrinsics          _depth_intrinsics;
        rs2_intrinsics          _mapped_intrinsics;
        float                   _depth_units;
        rs2_extrinsics          _extrinsics;
        std::atomic_bool        _invalidate_mapped;

        std::shared_ptr<stream_profile_interface> _stream, _mapped;
        int                     _mapped_stream_id = -1;
        stream_profile_interface* _depth_stream = nullptr;

        void inspect_depth_frame(const rs2::frame& depth);
        void inspect_other_frame(const rs2::frame& other);
        void process_depth_frame(const rs2::depth_frame& depth);

        bool stream_changed(stream_profile_interface* old, stream_profile_interface* curr);
    };
}
