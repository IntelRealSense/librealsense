// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
//
// Plane Fit implementation follows http://www.ilikebigbits.com/blog/2015/3/2/plane-from-points algorithm

#pragma once
#include <vector>
#include <mutex>
#include <array>
#include <imgui.h>
#include <librealsense2/rsutil.h>
#include <librealsense2/rs.hpp>
#include "rendering.h"

namespace rs2
{
    namespace depth_quality
    {
        struct snapshot_metrics
        {
            int width;
            int height;

            rs2::region_of_interest roi;

            float distance;
            float angle;
            float angle_x;
            float angle_y;

            plane p;
            std::array<float3, 4> plane_corners;
        };

        struct single_metric_data
        {
            single_metric_data(std::string name, float val) :
                val(val), name(name) {}

            float val;
            std::string name;
        };

        using callback_type = std::function<void(
            const std::vector<rs2::float3>& points,
            const plane p,
            const rs2::region_of_interest roi,
            const float baseline_mm,
            const float focal_length_pixels,
            const int ground_thruth_mm,
            const bool plane_fit,
            const float plane_fit_to_ground_truth_mm,
            bool record,
            std::vector<single_metric_data>& samples)>;

        inline snapshot_metrics analyze_depth_image(
            const rs2::depth_frame& frame,
            float units, float baseline_mm,
            const rs2_intrinsics * intrin,
            rs2::region_of_interest roi,
            const int ground_truth_mm,
            bool plane_fit_present,
            std::vector<single_metric_data>& samples,
            bool record,
            callback_type callback)
        {
            auto pixels = (const uint16_t*)frame.get_data();
            const auto w = frame.get_width();
            const auto h = frame.get_height();

            snapshot_metrics result{ w, h, roi, {} };

            std::vector<rs2::float3> roi_pixels;

            for (int y = roi.min_y; y < roi.max_y; ++y)
                for (int x = roi.min_x; x < roi.max_x; ++x)
                {
                    auto depth_raw = pixels[y*w + x];

                    if (depth_raw)
                    {
                        // units is float
                        float pixel[2] = { float(x), float(y) };
                        float point[3];
                        auto distance = depth_raw * units;

                        rs2_deproject_pixel_to_point(point, intrin, pixel, distance);
                        roi_pixels.push_back({ point[0], point[1], point[2] });
                    }
                }

            auto p = frame.fit_plane(roi);
            if (!p) { // The points in RoI don't span a valid plane
                return result;
            }

            // Calculate intersection of the plane fit with a ray along the center of ROI
            // that by design coincides with the center of the frame
            vertex plane_fit_pivot = p(intrin->width / 2.f, intrin->height / 2.f);
            float plane_fit_to_gt_offset_mm = (ground_truth_mm > 0.f) ? (plane_fit_pivot.z * 1000 - ground_truth_mm) : 0;

            result.p = p;
            result.plane_corners[0] = to_float3(p(roi.min_x, roi.min_y));
            result.plane_corners[1] = to_float3(p(roi.max_x, roi.min_y));
            result.plane_corners[2] = to_float3(p(roi.max_x, roi.max_y));
            result.plane_corners[3] = to_float3(p(roi.min_x, roi.max_y));

            // Distance of origin (the camera) from the plane is encoded in parameter D of the plane
            // The parameter represents the euclidian distance (along plane normal) from camera to the plane
            result.distance = static_cast<float>(-p.d() * 1000);
            // Angle can be calculated from param C
            result.angle = static_cast<float>(std::acos(std::abs(p.c())) / M_PI * 180.);

            callback(roi_pixels, p, roi, baseline_mm, intrin->fx, ground_truth_mm, plane_fit_present,
                plane_fit_to_gt_offset_mm, record, samples);

            // Calculate normal
            auto n = float3{ p.a(), p.b(), p.c() };
            auto cam = float3{ 0.f, 0.f, -1.f };
            auto dot = n * cam;
            auto u = cam - n * dot;

            result.angle_x = u.x;
            result.angle_y = u.y;

            return result;
        }
    }
}
