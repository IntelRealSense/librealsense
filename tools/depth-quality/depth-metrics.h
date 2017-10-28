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

        using callback_type = std::function<void(
            const std::vector<rs2::float3>& points,
            plane p, rs2::region_of_interest roi,
            float baseline_mm, float focal_length_pixels, double timestamp)>;

        inline plane plane_from_point_and_normal(const rs2::float3& point, const rs2::float3& normal)
        {
            return{ normal.x, normal.y, normal.z, -(normal.x*point.x + normal.y*point.y + normal.z*point.z) };
        }

        inline plane plane_from_points(const std::vector<rs2::float3> points)
        {
            if (points.size() < 3) throw std::runtime_error("Not enough points to calculate plane");

            rs2::float3 sum = { 0,0,0 };
            for (auto point : points) sum = sum + point;

            rs2::float3 centroid = sum / float(points.size());

            double xx = 0, xy = 0, xz = 0, yy = 0, yz = 0, zz = 0;
            for (auto point : points) {
                rs2::float3 temp = point - centroid;
                xx += temp.x * temp.x;
                xy += temp.x * temp.y;
                xz += temp.x * temp.z;
                yy += temp.y * temp.y;
                yz += temp.y * temp.z;
                zz += temp.z * temp.z;
            }

            double det_x = yy*zz - yz*yz;
            double det_y = xx*zz - xz*xz;
            double det_z = xx*yy - xy*xy;

            double det_max = std::max({ det_x, det_y, det_z });
            if (det_max <= 0) return{ 0, 0, 0, 0 };

            rs2::float3 dir{};
            if (det_max == det_x)
            {
                float a = static_cast<float>((xz*yz - xy*zz) / det_x);
                float b = static_cast<float>((xy*yz - xz*yy) / det_x);
                dir = { 1, a, b };
            }
            else if (det_max == det_y)
            {
                float a = static_cast<float>((yz*xz - xy*zz) / det_y);
                float b = static_cast<float>((xy*xz - yz*xx) / det_y);
                dir = { a, 1, b };
            }
            else
            {
                float a = static_cast<float>((yz*xy - xz*yy) / det_z);
                float b = static_cast<float>((xz*xy - yz*xx) / det_z);
                dir = { a, b, 1 };
            }

            return plane_from_point_and_normal(centroid, dir.normalize());
        }

        inline double evaluate_pixel(const plane& p, const rs2_intrinsics* intrin, int x, int y, float distance, float3& output)
        {
            float pixel[2] = { float(x), float(y) };
            rs2_deproject_pixel_to_point(&output.x, intrin, pixel, distance);
            return evaluate_plane(p, output);
        }

        inline float3 approximate_intersection(const plane& p, const rs2_intrinsics* intrin, int x, int y, float min, float max)
        {
            float3 point;
            auto far = evaluate_pixel(p, intrin, x, y, max, point);
            if (fabs(max - min) < 1e-3) return point;
            auto near = evaluate_pixel(p, intrin, x, y, min, point);
            if (far*near > 0) return{ 0, 0, 0 };

            auto avg = (max + min) / 2;
            auto mid = evaluate_pixel(p, intrin, x, y, avg, point);
            if (mid*near < 0) return approximate_intersection(p, intrin, x, y, min, avg);
            return approximate_intersection(p, intrin, x, y, avg, max);
        }

        inline float3 approximate_intersection(const plane& p, const rs2_intrinsics* intrin, int x, int y)
        {
            return approximate_intersection(p, intrin, x, y, 0.f, 1000.f);
        }

        inline snapshot_metrics analyze_depth_image(
            const rs2::video_frame& frame,
            float units, float baseline_mm,
            const rs2_intrinsics * intrin,
            rs2::region_of_interest roi,
            callback_type callback)
        {
            auto pixels = (const uint16_t*)frame.get_data();
            const auto w = frame.get_width();
            const auto h = frame.get_height();

            snapshot_metrics result{ w, h, roi, {} };

            std::mutex m;

            std::vector<rs2::float3> roi_pixels;

#pragma omp parallel for
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


                        std::lock_guard<std::mutex> lock(m);
                        roi_pixels.push_back({ point[0], point[1], point[2] });
                    }
                }

            if (roi_pixels.size() < 3) { // Not enough pixels in RoI to fit a plane
                return result;
            }

            plane p = plane_from_points(roi_pixels);

            if (p == plane{ 0, 0, 0, 0 }) { // The points in RoI don't span a plane
                return result;
            }

            callback(roi_pixels, p, roi, baseline_mm, intrin->fx, frame.get_timestamp());
            result.p = p;
            result.plane_corners[0] = approximate_intersection(p, intrin, roi.min_x, roi.min_y);
            result.plane_corners[1] = approximate_intersection(p, intrin, roi.max_x, roi.min_y);
            result.plane_corners[2] = approximate_intersection(p, intrin, roi.max_x, roi.max_y);
            result.plane_corners[3] = approximate_intersection(p, intrin, roi.min_x, roi.max_y);

            // Distance of origin (the camera) from the plane is encoded in parameter D of the plane
            result.distance = static_cast<float>(-p.d * 1000);
            // Angle can be calculated from param C
            result.angle = static_cast<float>(std::acos(std::abs(p.c)) / M_PI * 180.);

            // Calculate normal
            auto n = float3{ p.a, p.b, p.c };
            auto cam = float3{ 0.f, 0.f, -1.f };
            auto dot = n * cam;
            auto u = cam - n * dot;

            result.angle_x = u.x;
            result.angle_y = u.y;

            return result;
        }
    }
}
