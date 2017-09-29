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
        struct metrics
        {
            float avg_dist;
            float std_dev;
            float fit;
            float distance;
            float angle;
            float outlier_pct;
        };

        struct snapshot_metrics
        {
            int width;
            int height;

            rs2::region_of_interest roi;

            metrics planar;
            metrics depth;
            plane p;
            std::array<float3, 4> plane_corners;

            float non_null_pct;
        };

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

        inline metrics calculate_plane_metrics(const std::vector<rs2::float3>& points, plane p, float outlier_crop = 2.5, float std_devs = 2)
        {
            metrics result;

            std::vector<double> distances;
            distances.reserve(points.size());
            for (auto point : points)
            {
                distances.push_back(std::abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) * 1000);
            }

            std::sort(distances.begin(), distances.end());
            int n_outliers = int(distances.size() * (outlier_crop / 100));
            auto begin = distances.begin() + n_outliers, end = distances.end() - n_outliers;

            double total_distance = 0;
            for (auto itr = begin; itr < end; ++itr) total_distance += *itr;
            result.avg_dist = static_cast<float>(total_distance / (distances.size() - 2 * n_outliers));

            double total_sq_diffs = 0;
            for (auto itr = begin; itr < end; ++itr)
            {
                total_sq_diffs += std::pow(*itr - result.avg_dist, 2);
            }
            result.std_dev = static_cast<float>(std::sqrt(total_sq_diffs / points.size()));

            result.fit = result.std_dev / 10;

            result.distance = float(-p.d);

            result.angle = static_cast<float>(std::acos(std::abs(p.c)) / M_PI * 180.);

            std::vector<float3> outliers;
            for (auto point : points) if (std::abs(std::abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) * 1000 - result.avg_dist) > result.std_dev * std_devs) outliers.push_back(point);

            result.outlier_pct = outliers.size() / float(distances.size()) * 100;

            return result;
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

        inline metrics calculate_depth_metrics(const std::vector<rs2::float3>& points)
        {
            metrics result;

            double total_distance = 0;
            for (auto point : points)
            {
                total_distance += point.z * 1000;
            }
            result.avg_dist = float(total_distance / points.size());

            double total_sq_diffs = 0;
            for (auto point : points)
            {
                total_sq_diffs += std::pow(abs(point.z * 1000 - result.avg_dist), 2);
            }
            result.std_dev = static_cast<float>(std::sqrt(total_sq_diffs / points.size()));

            result.fit = result.std_dev / 10;

            result.distance = points[0].z;

            result.angle = 0;

            return result;
        }

        inline snapshot_metrics analyze_depth_image(const rs2::video_frame& frame, float units, const rs2_intrinsics * intrin, rs2::region_of_interest roi)
        {
            auto pixels = (const uint16_t*)frame.get_data();
            const auto w = frame.get_width();
            const auto h = frame.get_height();

            snapshot_metrics result{ w, h, roi,{ 0, 0, 0 },{ 0, 0, 0 }, 0 };

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

                        // float check_pixel[2] = { 0.f, 0.f };
                        // rs2_project_point_to_pixel(check_pixel, intrin, point);
                        // for sanity, assert check_pixel == pixel

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

            result.planar = calculate_plane_metrics(roi_pixels, p);
            result.depth = calculate_depth_metrics(roi_pixels);
            result.p = p;
            result.plane_corners[0] = approximate_intersection(p, intrin, roi.min_x, roi.min_y);
            result.plane_corners[1] = approximate_intersection(p, intrin, roi.max_x, roi.min_y);
            result.plane_corners[2] = approximate_intersection(p, intrin, roi.max_x, roi.max_y);
            result.plane_corners[3] = approximate_intersection(p, intrin, roi.min_x, roi.max_y);

            result.non_null_pct = roi_pixels.size() / float((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * 100;

            return result;
        }
    }
}
