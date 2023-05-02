// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "float3.h"
#include <map>
#include <vector>
#include <algorithm>
#include <include/librealsense2/h/rs_types.h>
#include <include/librealsense2/hpp/rs_frame.hpp>

#include "labeled-point-cloud-utilities.h"

namespace rs2 
{
    std::map<rs2_point_cloud_label, float3> labeled_point_cloud_utilities::get_label_to_color3f()
    {
        static const float3 DARK_PURPLE_COL = { 0.2f, 0.1f, 0.2f };
        static const float3 BROWN_COL = { 0.6f, 0.3f, 0.0f };
        static const float3 GREEN_COL = { 0.0f, 1.0f, 0.0f };
        static const float3 RED_COL = { 1.0f, 0.0f, 0.0f };
        static const float3 TURQUOISE_COL = { 0.0f, 1.0f, 1.0f };
        static const float3 BLUE_COL = { 0.0f, 0.0f, 1.0f };
        static const float3 PINK_COL = { 1.0f, 0.75f, 0.8f };
        static const float3 GREY_COL = { 0.5f, 0.5f, 0.5f };
        std::map<rs2_point_cloud_label, float3> label_to_color3f;

        label_to_color3f[RS2_POINT_CLOUD_LABEL_UNKNOWN] = DARK_PURPLE_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_UNDEFINED] = DARK_PURPLE_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_INVALID] = DARK_PURPLE_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_GROUND] = BROWN_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_NEAR_GROUND] = GREEN_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_OBSTACLE] = RED_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_OVERHEAD] = TURQUOISE_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_ABOVE_CEILING_HEIGHT] = BLUE_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_GAP] = PINK_COL;
        label_to_color3f[RS2_POINT_CLOUD_LABEL_MASKED] = GREY_COL;

        return label_to_color3f;
    }

    std::vector< std::pair<uint8_t, std::vector<rs2::vertex> > >
        labeled_point_cloud_utilities::prepare_labeled_points_data(const std::vector<rs2::vertex>& vertices_vec, 
            const std::vector<uint8_t>& labels_vec, size_t vertices_size)
    {
        std::vector< std::pair<uint8_t, std::vector<rs2::vertex> > > labels_to_vertices;

        for (int i = 0; i < vertices_size; ++i)
        {
            auto it = std::find_if(labels_to_vertices.begin(), labels_to_vertices.end(),
                [&](const std::pair<uint8_t, std::vector<rs2::vertex> >& p) {return p.first == labels_vec[i]; });
            if (it == labels_to_vertices.end())
                labels_to_vertices.push_back(std::make_pair(labels_vec[i], std::vector<rs2::vertex>()));
        }
        std::sort(labels_to_vertices.begin(), labels_to_vertices.end(),
            [&](const auto& left, const auto& right)
            {
                return left.first < right.first;
            });

        for (int i = 0; i < vertices_size; ++i)
        {
            auto it = std::find_if(labels_to_vertices.begin(), labels_to_vertices.end(),
                [&](const std::pair<uint8_t, std::vector<rs2::vertex> >& p) {return p.first == labels_vec[i]; });
            if (it == labels_to_vertices.end())
                throw std::runtime_error("Should not happen");

            it->second.push_back(vertices_vec[i]);
        }

        return labels_to_vertices;
    }
} // rs2 namespace