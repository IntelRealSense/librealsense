#pragma once
#include <vector>
#include <imgui.h>
#include <rendering.h>

// FW Declaration
namespace rs2
{
    struct region_of_interest;
    struct float3;
}

namespace rs2_depth_quality
{
    struct metrics
    {
        double avg_dist;
        double std_dev;
        double fit;
        double distance;
        double angle;
        std::vector<rs2::float3> outliers;
        double outlier_pct;
    };

    struct img_metrics
    {
        int width;
        int height;

        rs2::region_of_interest roi;

        metrics plane;
        metrics depth;

        double non_null_pct;
    };

    struct plane
    {
        double a;
        double b;
        double c;
        double d;
    };
    inline bool operator==(const plane& lhs, const plane& rhs) { return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d; }

    plane plane_from_point_and_normal(const rs2::float3& point, const rs2::float3& normal)
    {
        return{ normal.x, normal.y, normal.z, -(normal.x*point.x + normal.y*point.y + normal.z*point.z) };
    }

    plane plane_from_points(const std::vector<rs2::float3> points)
    {
        if (points.size() < 3) throw std::runtime_error("Not enough points to calculate plane");

        rs2::float3 sum = { 0,0,0 };
        for (auto point : points) sum = sum + point;

        rs2::float3 centroid = sum / points.size();

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

    metrics calculate_plane_metrics(const std::vector<rs2::float3>& points, plane p, float outlier_crop = 2.5, float std_devs = 2)
    {
        metrics result;

        std::vector<double> distances;
        distances.reserve(points.size());
        for (auto point : points)
        {
            distances.push_back(std::abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) * 1000);
        }

        std::sort(distances.begin(), distances.end());
        int n_outliers = distances.size() * (outlier_crop / 100);
        auto begin = distances.begin() + n_outliers, end = distances.end() - n_outliers;

        double total_distance = 0;
        for (auto itr = begin; itr < end; ++itr) total_distance += *itr;
        result.avg_dist = total_distance / (distances.size() - 2 * n_outliers);

        double total_sq_diffs = 0;
        for (auto itr = begin; itr < end; ++itr)
        {
            total_sq_diffs += std::pow(*itr - result.avg_dist, 2);
        }
        result.std_dev = std::sqrt(total_sq_diffs / points.size());

        result.fit = result.std_dev / 10;

        result.distance = -p.d;

        result.angle = std::acos(std::abs(p.c)) / M_PI * 180;

        for (auto point : points) if (std::abs(std::abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) * 1000 - result.avg_dist) > result.std_dev * std_devs) result.outliers.push_back(point);

        result.outlier_pct = result.outliers.size() / float(distances.size()) * 100;

        return result;
    }

    metrics calculate_depth_metrics(const std::vector<rs2::float3>& points)
    {
        metrics result;

        double total_distance = 0;
        for (auto point : points)
        {
            total_distance += point.z * 1000;
        }
        result.avg_dist = total_distance / points.size();

        double total_sq_diffs = 0;
        for (auto point : points)
        {
            total_sq_diffs += std::pow(abs(point.z * 1000 - result.avg_dist), 2);
        }
        result.std_dev = std::sqrt(total_sq_diffs / points.size());

        result.fit = result.std_dev / 10;

        result.distance = points[0].z;

        result.angle = 0;

        return result;
    }


    class PlotMetric
    {
    private:
        /*int size;*/
        int idx;
        float vals[100];
        float min, max;
        std::string id, label, tail;
        ImVec2 size;

    public:
        PlotMetric(const std::string& name, float min, float max, ImVec2 size, const std::string& tail) : idx(0), vals(), min(min), max(max), id("##" + name), label(name + " = "), tail(tail), size(size) {}
        ~PlotMetric() {}

        void add_value(float val)
        {
            vals[idx] = val;
            idx = (idx + 1) % 100;
        }
        void plot()
        {
            std::stringstream ss;
            ss << label << vals[(100 + idx - 1) % 100] << tail;
            ImGui::PlotLines(id.c_str(), vals, 100, idx, ss.str().c_str(), min, max, size);
        }
    };


    img_metrics analyze_depth_image(const rs2::video_frame& frame, float units, const rs2_intrinsics * intrin, rs2::region_of_interest roi)
    {
        auto pixels = (const uint16_t*)frame.get_data();
        const auto w = frame.get_width();
        const auto h = frame.get_height();

        img_metrics result{ w, h, roi,{ 0, 0, 0 },{ 0, 0, 0 }, 0 };

        std::mutex m;

        std::vector<rs2::float3> roi_pixels;

#pragma omp parallel for
        for (int y = roi.min_y; y < roi.max_y; ++y)
            for (int x = roi.min_x; x < roi.max_x; ++x)
            {
                //std::cout << "Accessing index " << (y*w + x) << std::endl;
                auto depth_raw = pixels[y*w + x];

                if (depth_raw)
                {
                    // units is float
                    float pixel[2] = { x, y };
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

        if (roi_pixels.size() < 3) {
            // std::cout << "Not enough pixels in RoI to fit a plane." << std::endl;
            return result;
        }

        plane p = plane_from_points(roi_pixels);

        if (p == plane{ 0, 0, 0, 0 }) {
            // std::cout << "The points in RoI don't span a plane." << std::endl;
            return result;
        }

        result.plane = calculate_plane_metrics(roi_pixels, p);
        result.depth = calculate_depth_metrics(roi_pixels);

        result.non_null_pct = roi_pixels.size() / double((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * 100;

        return result;
    }

    void visualize(img_metrics stats, int w, int h, bool plane)
    {
        float x_scale = w / float(stats.width);
        float y_scale = h / float(stats.height);
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, );
        //ImGui::SetNextWindowPos({ float(area.x), float(area.y) });
        //ImGui::SetNextWindowSize({ float(area.size), float(area.size) });

        ImGui::GetWindowDrawList()->AddRectFilled({ float(stats.roi.min_x)*x_scale, float(stats.roi.min_y)*y_scale }, { float(stats.roi.max_x)*x_scale, float(stats.roi.max_y)*y_scale },
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.f + ((plane) ? stats.plane.fit : stats.depth.fit), 1.f - ((plane) ? stats.plane.fit : stats.depth.fit), 0, 0.25f)), 5.f, 15.f);

        //std::stringstream ss; ss << stats.roi.min_x << ", " << stats.roi.min_y;
        //auto s = ss.str();
        /*ImGui::Begin(s.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);*/


        /*if (ImGui::IsItemHovered())
        ImGui::SetTooltip(s.c_str());*/

        //ImGui::End();
        //ImGui::PopStyleColor();
    }

    class depth_metrics_model
    {
        // Data display plots
        /*PlotMetric avg_plot("AVG", 0, 10, { 180, 50 }, " (mm)");
        PlotMetric std_plot("STD", 0, 10, { 180, 50 }, " (mm)");
        PlotMetric fill_plot("FILL", 0, 100, { 180, 50 }, "%");
        PlotMetric dist_plot("DIST", 0, 5, { 180, 50 }, " (m)");
        PlotMetric angle_plot("ANGLE", 0, 180, { 180, 50 }, " (deg)");
        PlotMetric out_plot("OUTLIERS", 0, 100, { 180, 50 }, "%");
*/
    public:
        void render() const
        {

        }
    };

}