// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../pointcloud.h"

namespace librealsense
{
    class pointcloud_cuda : public pointcloud
    {
    public:
        pointcloud_cuda();
    private:
        const float3 * depth_to_points(
            rs2::points output,
            const rs2_intrinsics &depth_intrinsics,
            const rs2::depth_frame& depth_frame,
            float depth_scale) override;
    };
}
