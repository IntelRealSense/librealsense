// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once


namespace rs2
{
    struct float3;
    class labeled_point_cloud_utilities
    {
    public:
        static std::map<rs2_point_cloud_label, float3> get_label_to_color3f();
    };
    
}
