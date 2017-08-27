// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.hpp>
#include "rendering.h"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <cmath>
#include <thread>
#include "../common/realsense-ui-advanced-mode.h"
#include "model-views.h"

using namespace rs2;
using namespace rs400;

std::string error_message{""};
color_map my_map({ { 255, 255, 255 },{ 0, 0, 0 } });

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
    PlotMetric(const std::string& name, float min, float max, ImVec2 size, const std::string& tail) : idx(0), vals(),  min(min), max(max), id("##" + name), label(name + " = "), tail(tail), size(size) {}
    ~PlotMetric() {}

    void add_value(float val)
    {
        vals[idx] = val;
        idx = (idx + 1) % 100;
    }
    void plot()
    {
        std::stringstream ss;
        ss << label << vals[(100 + idx - 1)%100] << tail;
        ImGui::PlotLines(id.c_str(), vals, 100, idx, ss.str().c_str(), min, max, size);
    }
};

struct metrics
{
    double avg_dist;
    double std_dev;
    double fit;
    double distance;
    double angle;
    std::vector<float3> outliers;
    double outlier_pct;
};

struct img_metrics
{
    int width;
    int height;

    region_of_interest roi;

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

struct option_data
{
    bool supported;
    union { bool b; float f; } value;
    option_range range;

    option_data(depth_sensor s, rs2_option o, bool isBool = false) : supported(s.supports(o)), range(supported ? s.get_option_range(o) : option_range{0, 1, 0, 1}) {
        if (isBool) value.b = (supported ? s.get_option(o) : 0);
        else value.f = (supported ? s.get_option(o) : 0);
    }
};

plane plane_from_point_and_normal(const float3& point, const float3& normal)
{
    return{ normal.x, normal.y, normal.z, -(normal.x*point.x + normal.y*point.y + normal.z*point.z) };
}

plane plane_from_points(const std::vector<float3> points)
{
    if (points.size() < 3) throw std::runtime_error("Not enough points to calculate plane");

    float3 sum = { 0,0,0 };
    for (auto point : points) sum = sum + point;

    float3 centroid = sum / points.size();

    double xx = 0, xy = 0, xz = 0, yy = 0, yz = 0, zz = 0;
    for (auto point : points) {
        float3 temp = point - centroid;
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

    float3 dir;
    if (det_max == det_x)
    {
        float a = (xz*yz - xy*zz) / det_x;
        float b = (xy*yz - xz*yy) / det_x;
        dir = { 1, a, b };
    }
    else if (det_max == det_y)
    {
        float a = (yz*xz - xy*zz) / det_y;
        float b = (xy*xz - yz*xx) / det_y;
        dir = { a, 1, b };
    }
    else
    {
        float a = (yz*xy - xz*yy) / det_z;
        float b = (xz*xy - yz*xx) / det_z;
        dir = { a, b, 1 };
    }

    return plane_from_point_and_normal(centroid, dir.normalize());
}

metrics calculate_plane_metrics(const std::vector<float3>& points, plane p, float outlier_crop = 2.5, float std_devs = 2)
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

    result.angle = std::acos(std::abs(p.c))/ M_PI * 180;

    for (auto point : points) if (std::abs(std::abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) * 1000 - result.avg_dist) > result.std_dev * std_devs) result.outliers.push_back(point);

    result.outlier_pct = result.outliers.size() / float(distances.size()) * 100;

    return result;
}

metrics calculate_depth_metrics(const std::vector<float3>& points)
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
        total_sq_diffs += std::pow(abs(point.z*1000 - result.avg_dist), 2);
    }
    result.std_dev = std::sqrt(total_sq_diffs / points.size());

    result.fit = result.std_dev / 10;

    result.distance = points[0].z;

    result.angle = 0;

    return result;
}

img_metrics analyze_depth_image(const rs2::video_frame& frame, float units, const rs2_intrinsics * intrin, region_of_interest roi)
{
    auto pixels = (const uint16_t*)frame.get_data();
    const auto w = frame.get_width();
    const auto h = frame.get_height();

    img_metrics result{ w, h, roi, {0, 0, 0}, {0, 0, 0}, 0 };

    std::mutex m;

    std::vector<float3> roi_pixels;

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

std::vector<std::string> get_device_info(const device& dev, bool include_location = true)
{
    std::vector<std::string> res;
    for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
    {
        auto info = static_cast<rs2_camera_info>(i);

        // When camera is being reset, either because of "hardware reset"
        // or because of switch into advanced mode,
        // we don't want to capture the info that is about to change
        if ((info == RS2_CAMERA_INFO_LOCATION ||
            info == RS2_CAMERA_INFO_ADVANCED_MODE)
            && !include_location) continue;

        if (dev.supports(info))
        {
            auto value = dev.get_info(info);
            res.push_back(value);
        }
    }
    return res;
}

void visualize(img_metrics stats, int w, int h, bool plane)
{
    float x_scale = w / float(stats.width);
    float y_scale = h / float(stats.height);
    //ImGui::PushStyleColor(ImGuiCol_WindowBg, );
    //ImGui::SetNextWindowPos({ float(area.x), float(area.y) });
    //ImGui::SetNextWindowSize({ float(area.size), float(area.size) });

    ImGui::GetWindowDrawList()->AddRectFilled({ float(stats.roi.min_x)*x_scale, float(stats.roi.min_y)*y_scale }, { float(stats.roi.max_x)*x_scale, float(stats.roi.max_y)*y_scale },
        ImGui::ColorConvertFloat4ToU32(ImVec4( 0.f + ((plane)? stats.plane.fit:stats.depth.fit), 1.f - ((plane)? stats.plane.fit:stats.depth.fit), 0, 0.25f )), 5.f, 15.f);

    //std::stringstream ss; ss << stats.roi.min_x << ", " << stats.roi.min_y;
    //auto s = ss.str();
    /*ImGui::Begin(s.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);*/


    /*if (ImGui::IsItemHovered())
        ImGui::SetTooltip(s.c_str());*/

    //ImGui::End();
    //ImGui::PopStyleColor();
}

int main(int argc, char * argv[])
{

    // UI option variables
    bool use_rect_fitting = true;

    // Data display plots
    PlotMetric avg_plot("AVG", 0, 10, { 180, 50 }, " (mm)");
    PlotMetric std_plot("STD", 0, 10, { 180, 50 }, " (mm)");
    PlotMetric fill_plot("FILL", 0, 100, { 180, 50 }, "%");
    PlotMetric dist_plot("DIST", 0, 5, { 180, 50 }, " (m)");
    PlotMetric angle_plot("ANGLE", 0, 180, { 180, 50 }, " (deg)");
    PlotMetric out_plot("OUTLIERS", 0, 100, { 180, 50 }, "%");

    context ctx;

    util::device_hub hub(ctx);

    auto finished = false;
    GLFWwindow* win;

    std::map<std::pair<int, int>, std::vector<int>> supported_fps_by_resolution;
    std::vector<std::string> restarting_device_info;

    advanced_mode_control amc{};
    bool get_curr_advanced_controls = true;

    const auto margin = 15.f;


    // Open a GLFW window
    glfwInit();

    bool options_invalidated = false;
    std::string error_message;

    win = glfwCreateWindow(1280, 720, "Depth Sanity", nullptr, nullptr);
    glfwMakeContextCurrent(win);

    ImGui_ImplGlfw_Init(win, true);


    while (!finished)
    {
        try
        {
            std::set<std::pair<int, int>> resolutions;
            std::vector<std::string> resolutions_strs;
            std::vector<const char*> resolutions_chars;

            int default_width, default_height;

            auto dev = hub.wait_for_device();
            auto dpt = dev.first<depth_sensor>();

            option_data ae(dpt, RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);
            option_data exposure(dpt, RS2_OPTION_EXPOSURE);
            option_data emitter(dpt, RS2_OPTION_EMITTER_ENABLED, true);
            option_data laser(dpt, RS2_OPTION_LASER_POWER);


            auto modes = dpt.get_stream_profiles();
            for (auto&& profile : modes)
            {
                if (profile.stream_type() == RS2_STREAM_DEPTH &&
                    profile.format() == RS2_FORMAT_Z16)
                {
                    auto video_profile = profile.as<video_stream_profile>();
                    resolutions.insert(std::make_pair(video_profile.width(), video_profile.height()));
                    supported_fps_by_resolution[std::make_pair(video_profile.width(), video_profile.height())].push_back(profile.fps());
                }
            }


            std::vector<const char*> presets_labels;
            std::vector<float> presets_numbers;

            auto range = dpt.get_option_range(RS2_OPTION_VISUAL_PRESET);
            auto index_of_selected_preset = 0, counter = 0;
            auto units = dpt.get_depth_scale();

            for (auto i = range.min; i <= range.max; i += range.step, counter++)
            {
                presets_labels.push_back(dpt.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i));
                presets_numbers.push_back(i);
            }


            std::vector<std::pair<int, int>> resolutions_vec(resolutions.begin(), resolutions.end());



            default_width = resolutions_vec.at((resolutions_vec.size()-1)).first;
            default_height = resolutions_vec.at((resolutions_vec.size()-1)).second;
            int index_of_selected_resolution = (resolutions.size()-1);



            for (auto&& resolution : resolutions_vec)
            {
                resolutions_strs.push_back(rs2::to_string() << resolution.first << "x" << resolution.second);
            }

            for (auto&& resolution_as_string : resolutions_strs)
            {
                resolutions_chars.push_back(resolution_as_string.c_str());
            }



            // Configure depth stream to run at 30 frames per second
            util::config config;
            config.enable_stream(RS2_STREAM_DEPTH, default_width, default_height, RS2_FORMAT_Z16, 30);

            auto stream = config.open(dev);
            rs2_intrinsics current_frame_intrinsics = stream.get_intrinsics(RS2_STREAM_DEPTH);

            frame_queue calc_queue(1);
            frame_queue display_queue(1);

            stream.start([&](rs2::frame f)
            {
                calc_queue.enqueue(f);
                display_queue.enqueue(f);
            });

            texture_buffer buffers[RS2_STREAM_COUNT];

            float roi_x_begin = default_width * (1.f / 3.f), roi_y_begin = default_height * (1.f / 3.f), roi_x_end = default_width * (2.f / 3.f), roi_y_end = default_height * (2.f / 3.f);
            region_of_interest roi{ roi_x_begin, roi_y_begin, roi_x_end, roi_y_end };
            img_metrics latest_stat{ 1280, 720, roi, { 0, 0, 0 }, { 0, 0, 0 }, 0 };
            std::mutex m;

            std::thread t([&m, &finished, &calc_queue, &units, &current_frame_intrinsics, &latest_stat, &roi]() {
                while (!finished)
                {
                    auto f = calc_queue.wait_for_frame();

                    auto stream_type = f.get_profile().stream_type();

                    if (stream_type == RS2_STREAM_DEPTH)
                    {
                        auto stats = analyze_depth_image(f, units, &current_frame_intrinsics, roi);

                        std::lock_guard<std::mutex> lock(m);
                        latest_stat = stats;
                        //cout << "Average distance is : " << latest_stat.avg_dist << endl;
                        //cout << "Standard_deviation is : " << latest_stat.std << endl;
                        //cout << "Percentage_of_non_null_pixels is : " << latest_stat.non_null_pct << endl;
                    }
                }
            });

            while (hub.is_connected(dev) && !glfwWindowShouldClose(win))
            {

                int w, h;
                glfwGetFramebufferSize(win, &w, &h);

                auto index = 0;


                // Wait for new images
                glfwPollEvents();
                ImGui_ImplGlfw_NewFrame();

                // Clear the framebuffer
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);

                auto f = display_queue.wait_for_frame();

                auto stream_type = f.get_profile().stream_type();

                if (stream_type == RS2_STREAM_DEPTH)
                {
                    buffers[stream_type].upload(f);
                }

                buffers[RS2_STREAM_DEPTH].show({ 0, 0, (float)w, (float)h }, 1);

                img_metrics stats_copy;
                {
                    std::lock_guard<std::mutex> lock(m);
                    stats_copy = latest_stat;
                }

                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0,0,0,0 });
                ImGui::SetNextWindowPos({ 0, 0 });
                ImGui::SetNextWindowSize({ (float)w, (float)h });
                ImGui::Begin("global", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);

                ImGuiIO& io = ImGui::GetIO();

                if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(0))
                {
                    auto pos = ImGui::GetIO().MousePos;
                    roi_x_begin = roi_x_end = pos.x;
                    roi_y_begin = roi_y_end = pos.y;
                }

                if (ImGui::IsWindowFocused() && ImGui::IsMouseDragging())
                {
                    roi_x_end += ImGui::GetIO().MouseDelta.x;
                    roi_y_end += ImGui::GetIO().MouseDelta.y;
                }

                roi = { int(std::max(std::min(roi_x_begin, roi_x_end), 0.f)), int(std::max(std::min(roi_y_begin, roi_y_end), 0.f)), int(std::min(std::max(roi_x_begin, roi_x_end), float(stats_copy.width))), int(std::min(std::max(roi_y_begin, roi_y_end), float(stats_copy.height))) };

                visualize(stats_copy, w, h, use_rect_fitting);

                ImGui::End();
                ImGui::PopStyleColor();

                // Draw GUI:
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.8f });

                //ImGui::SetNextWindowPos({ 10, 10 });
                ImGui::SetNextWindowPos({ margin, margin });
                ImGui::SetNextWindowSize({ 300, 200 });

//                ImGui::SetNextWindowPos({ 410, 360 });
//                ImGui::SetNextWindowSize({ 400.f, 350.f });
                ImGui::Begin("Stream Selector", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);


                rs2_error* e = nullptr;

                //ImFont* fnt;
                // Base font scale, multiplied by the per-window font scale which you can adjust with SetFontScale()
                //fnt->Scale=3.f;
                //ImGui::PushFont(fnt);

                ImGui::Text("SDK version: %s", api_version_to_string(rs2_get_api_version(&e)).c_str());
                //rs2_camera_info_to_string(rs2_camera_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)));
                ImGui::Text("Firmware: %s", dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION));
                ImGui::Text("Resolution:"); ImGui::SameLine();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("Resolution", &index_of_selected_resolution, resolutions_chars.data(), resolutions_chars.size()))
                {
                    auto selected_resolution = resolutions_vec[index_of_selected_resolution];

                    stream.stop();
                    stream.close();
                    util::config config;

                    auto fps = supported_fps_by_resolution[selected_resolution].front();
                    // TODO: if you can find 30, use it
                    // else use whatever
                    config.enable_stream(RS2_STREAM_DEPTH, selected_resolution.first,
                                         selected_resolution.second, RS2_FORMAT_Z16, fps);
                    stream = config.open(dev);
                    stream.start([&](rs2::frame f)
                    {
                        calc_queue.enqueue(f);
                        display_queue.enqueue(f);
                    });

                }
                ImGui::Text("Preset:"); ImGui::SameLine();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("Preset", &index_of_selected_preset, presets_labels.data(), presets_labels.size()))
                {
                    /*
                     * another way to get the number of preset :
                     *  if (ImGui::Combo(id.c_str(), &index_of_selected_preset, labels.data(),
                        static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * index_of_selected_preset;
                            endpoint.set_option(opt, value);
                     */
                    auto selected_preset = presets_numbers[index_of_selected_preset];
                    /*
                     * the second parameter < float selected_preset> is an preset that should be sat
                     */

                    try
                    {
                        dpt.set_option(RS2_OPTION_VISUAL_PRESET, selected_preset);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }

                if (ae.supported && ImGui::Checkbox("Enable Auto Exposure", &ae.value.b))
                {
                    try
                    {
                        dpt.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, ae.value.b);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }

                if (exposure.supported && ImGui::SliderFloat("Exposure", &exposure.value.f, exposure.range.min, exposure.range.max, "%.3f"))
                {
                    try
                    {
                        if (ae.supported) dpt.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, false);
                        dpt.set_option(RS2_OPTION_EXPOSURE, exposure.value.f);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }

                if (emitter.supported && ImGui::Checkbox("Emitter Powered", &emitter.value.b))
                {
                    try
                    {
                        dpt.set_option(RS2_OPTION_EMITTER_ENABLED, emitter.value.b);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }

                if (laser.supported && ImGui::SliderFloat("Laser Power", &laser.value.f, laser.range.min, laser.range.max, "%.3f"))
                {
                    try
                    {
                        if (emitter.value.b)
                            dpt.set_option(RS2_OPTION_LASER_POWER, laser.value.f);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }

                ImGui::Checkbox("Use Plane-Fitting", &use_rect_fitting);

                ImGui::PopItemWidth();


                if (error_message != "")
                {
                    ImGui::OpenPopup("Oops, something went wrong!");
                }
                if (ImGui::BeginPopupModal("Oops, something went wrong!", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("RealSense error calling:");
                    ImGui::InputTextMultiline("error", const_cast<char*>(error_message.c_str()),
                        error_message.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::Separator();

                    if (ImGui::Button("OK", ImVec2(120, 0)))
                    {
                        error_message = "";
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

               ImGui::End();
               ImGui::PopStyleColor();

               ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.9f });

               ImGui::SetNextWindowPos({ w - 196 - margin, h - 336 - margin });
               ImGui::SetNextWindowSize({ 196, 336 });

//               ImGui::SetNextWindowPos({ 0.85*w, 0.90*h });
//               ImGui::SetNextWindowSize({ (.1*w), (.1*h)});

//               ImGui::SetNextWindowPos({ 1530, 930 });
//               ImGui::SetNextWindowSize({ 300.f, 370.f });

               ImGui::Begin("latest_stat", nullptr,
                                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
               //ImGui::SetWindowFontScale(w/1000);

               metrics data = use_rect_fitting? stats_copy.plane:stats_copy.depth;

               avg_plot.add_value(data.avg_dist);
               std_plot.add_value(data.std_dev);
               fill_plot.add_value(stats_copy.non_null_pct);
               dist_plot.add_value(data.distance);
               angle_plot.add_value(data.angle);
               out_plot.add_value(data.outlier_pct);

               avg_plot.plot();
               std_plot.plot();
               fill_plot.plot();
               dist_plot.plot();
               angle_plot.plot();
               out_plot.plot();

               ImGui::End();
               ImGui::PopStyleColor();

               ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.7f });

               ImGui::SetNextWindowPos({ float((w - 345) / 2), margin });
               ImGui::SetNextWindowSize({ 345, 28 });

               ImGui::Begin("##help", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

               ImGui::Text("Click and drag to select the Region of Interest");

               ImGui::End();
               ImGui::PopStyleColor();

               ImGui::Render();
               glPopMatrix();
               glfwSwapBuffers(win);
            }

            if (glfwWindowShouldClose(win))
                finished = true;

            t.join();

        }
        catch (const error & e)
        {
            std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
        }
        catch (const std::exception & e)
        {
            std::cerr << e.what() << std::endl;
        }

        ImGui_ImplGlfw_Shutdown();
        glfwDestroyWindow(win);
        glfwTerminate();
    }
    return EXIT_SUCCESS;
}

