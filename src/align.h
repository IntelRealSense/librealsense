// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

//#include "core/processing.h"
//#include "processing_block.h"
//#include "image.h"
//#include "source.h"

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

        std::shared_ptr<stream_profile_interface> _stream, _mapped;
        stream_profile_interface* _depth_stream = nullptr;
    };

    class align : public processing_block
    {
    public:
        align(rs2_stream stream);

    private:
        std::mutex              _mutex;

        const rs2_intrinsics*   _depth_intrinsics_ptr;
        const float*            _depth_units_ptr;
        const rs2_intrinsics*   _other_intrinsics_ptr;
        const rs2_extrinsics*   _depth_to_other_extrinsics_ptr;

        rs2_intrinsics          _depth_intrinsics;
        rs2_intrinsics          _other_intrinsics;
        float                   _depth_units;
        rs2_extrinsics          _depth_to_other_extrinsics;
        rs2_stream              _other_stream_type;
        int                     _width, _height;
        int                     _other_bytes_per_pixel;
        int*                    _other_bytes_per_pixel_ptr;
        std::shared_ptr<stream_profile_interface> _depth_stream_profile, _other_stream_profile;
    };
}
