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

        static std::vector< std::pair<uint8_t, std::vector<rs2::vertex> > >
            prepare_labeled_points_data(const std::vector<rs2::vertex>& vertices_vec,
                const std::vector<uint8_t>& labels_vec, size_t vertices_size);
    };
    
}
