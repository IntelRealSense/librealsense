// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <cmath>
#include "../common/realsense-ui/realsense-ui-advanced-mode.h"
#include "model-views.h"

using namespace rs2;
using namespace std;
using namespace rs400;


vector<int> resolutions_by_fps_rewrite_it_as_lambda(int width,int height)
{
    return{};
}


std::string error_message{""};
struct box {
    int x;
    int y;
    int size;
    double avg_dist;
    double std;
    double non_null_pct;

    double abs_avg_dist;
    double abs_std;
    float abs_fit;

    float fit;
};


struct metrics
{
    int width;
    int height;
    double avg_dist;
    double std;
    double _percentage_of_non_null_pixels;
    std::vector<box> boxes;
};

struct plane
{
    double a;
    double b;
    double c;
    double d;
};
inline bool operator==(const plane& lhs, const plane& rhs) { return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d; }

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

    double det_max = max({ det_x, det_y, det_z });
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

metrics analyze_depth_image(const rs2::video_frame& frame, float units, const rs2_intrinsics * intrin, int box_size=10)
{
    auto pixels = (const uint16_t*)frame.get_data();
    const auto w = frame.get_width();
    const auto h = frame.get_height();
    const int n_groups = std::ceil(w / double(box_size)) * std::ceil(h / double(box_size));

    metrics result{ w, h, 0, 0, 0, std::vector<box>(n_groups, {0, 0, box_size, 0, 0, 0, 0, 0, 1, 1})};

    double sum_of_all_distances = 0;
    double sum_of_all_squared_differences = 0;
    double number_of_non_null_pixels = 0;
    long long num_processed_pixels = 0;
    
    std::mutex m;

    std::vector<std::vector<float3>> groups_of_pixels(n_groups);

#pragma omp parallel for
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            //std::cout << "Accessing index " << (y*w + x) << std::endl;
            auto depth_raw = pixels[y*w + x];


            if (depth_raw)
            {
                // units is float
                float pixel[2] = { x, y };
                float check_pixel[2] = { 0.f, 0.f };
                float point[3];
                float3 current_coordinates;
                auto distance = depth_raw * units;

                rs2_deproject_pixel_to_point(point,intrin,pixel,distance);
                current_coordinates.x = point[0];
                current_coordinates.y = point[1];
                current_coordinates.z = point[2];

                //rs2_project_point_to_pixel(check_pixel,intrin,point);
                // for sanity, assert check_pixel == pixel

                int group = int(y / double(box_size))*std::ceil(w / double(box_size)) + int(x / double(box_size));
                lock_guard<mutex> lock(m);
                groups_of_pixels[group].push_back(current_coordinates);

                ++number_of_non_null_pixels;
            }


            //std::cout << distance << std::endl;
        }

    //std::cout << number_of_non_null_pixels << std::endl;

#pragma omp parallel for
    for (int i = 0; i < n_groups; ++i) {
        result.boxes[i].x = (i % int(std::ceil(w / double(box_size)))) * box_size;
        result.boxes[i].y = (i / int(std::ceil(w / double(box_size)))) * box_size;

        if (groups_of_pixels[i].size() < 3) {
           // std::cout << "not enough pixels in group " << i << " (" << groups_of_pixels[i].size() << ")" << std::endl;
            result.boxes[i].fit = 1;
            result.boxes[i].abs_fit = 1;
        }
        else
        {
            plane p = plane_from_points(groups_of_pixels[i]);

            if (p == plane{ 0, 0, 0, 0 }) {
                result.boxes[i].fit = 1;
                result.boxes[i].abs_fit = 1;
            }
            else
            {

                double total_distance = 0, abs_total_distance = 0;
                for (auto point : groups_of_pixels[i])
                {
                    total_distance += abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d);
                    abs_total_distance += point.z;
                }
                result.boxes[i].avg_dist = total_distance / groups_of_pixels[i].size();
                result.boxes[i].abs_avg_dist = abs_total_distance / groups_of_pixels[i].size();

                double total_sq_diffs = 0, abs_total_sq_diffs = 0;
                for (auto point : groups_of_pixels[i])
                {
                    total_sq_diffs += std::pow(abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) - result.boxes[i].avg_dist, 2);
                    abs_total_sq_diffs += std::pow(abs(point.z - result.boxes[i].abs_avg_dist), 2);
                }
                result.boxes[i].std = std::sqrt(total_sq_diffs / groups_of_pixels[i].size());
                result.boxes[i].abs_std = std::sqrt(abs_total_sq_diffs / groups_of_pixels[i].size());

                result.boxes[i].non_null_pct = groups_of_pixels[i].size() / double((std::min(w, result.boxes[i].x + box_size) - result.boxes[i].x)*(std::min(h, result.boxes[i].y + box_size) - result.boxes[i].y));

                result.boxes[i].fit = result.boxes[i].std * 100;
                result.boxes[i].abs_fit = result.boxes[i].abs_std * 100;

                lock_guard<mutex> lock(m);
                sum_of_all_distances += total_distance;
                sum_of_all_squared_differences += total_sq_diffs;
                num_processed_pixels += groups_of_pixels[i].size();
            }
        }
    }

    result.avg_dist = sum_of_all_distances / num_processed_pixels;;
    //result.std = std::sqrt(sum_of_all_squared_differences/num_of_examined_pixels);
    result.std = std::sqrt(sum_of_all_squared_differences / num_processed_pixels);;
    result._percentage_of_non_null_pixels = (number_of_non_null_pixels / (w*h)) * 100;
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

void visualize(metrics stats, int w, int h, bool plane)
{
    float x_scale = w / float(stats.width);
    float y_scale = h / float(stats.height);
    for (auto &&area : stats.boxes) {
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, );
        //ImGui::SetNextWindowPos({ float(area.x), float(area.y) });
        //ImGui::SetNextWindowSize({ float(area.size), float(area.size) });

        ImGui::GetWindowDrawList()->AddRectFilled({ float(area.x)*x_scale, float(area.y)*y_scale }, { float(area.x + area.size)*x_scale, float(area.y + area.size)*y_scale },
            ImGui::ColorConvertFloat4ToU32(ImVec4( 0.f + ((plane)? area.fit:area.abs_fit), 1.f - ((plane)? area.fit:area.abs_fit), 0, 0.25f )), 5.f, 15.f);

        stringstream ss; ss << area.x << ", " << area.y;
        auto s = ss.str();
        /*ImGui::Begin(s.c_str(), nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);*/

        
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(s.c_str());

        //ImGui::End();
        //ImGui::PopStyleColor();
    }
}

color_map my_map({ { 255, 255, 255 }, { 0, 0, 0 } });

int main(int argc, char * argv[])
{
    bool use_rect_fitting = true;
    int box_size = 50;

    context ctx;
    
    util::device_hub hub(ctx);

    auto finished = false;
    GLFWwindow* win;

    map<pair<int,int>,vector<int>> supported_fps_by_resolution;
    std::vector<std::string> restarting_device_info;

    advanced_mode_control amc{};
    bool get_curr_advanced_controls = true;

    const auto margin = 15.f;

    while (!finished)
    {
        try
        {
            std::set<pair<int,int>> resolutions;
            std::vector<std::string> resolutions_strs;
            std::vector<const char*> resolutions_chars;

            int default_width,default_height;

            auto dev = hub.wait_for_device();
            auto dpt = dev.first<depth_sensor>();



            auto modes = dpt.get_stream_modes();
            for (auto&& profile : dpt.get_stream_modes())
            {
                if (profile.stream == RS2_STREAM_DEPTH &&
                    profile.format == RS2_FORMAT_Z16)
                {
                    resolutions.insert(make_pair(profile.width,profile.height));
                    supported_fps_by_resolution[make_pair(profile.width,profile.height)].push_back(profile.fps);
                }
            }


            std::vector<const char*> presets_labels;
            std::vector<float> presets_numbers;
            /**
            * read option value from the device
            * \param[in] option   option id to be queried
            * \return float value of the option
            */
            //auto p = dpt.get_option(RS2_OPTION_ADVANCED_MODE_PRESET);
            //.set_option(RS2_OPTION_ADVANCED_MODE_PRESET, p);
            //auto text = dpt.get_option_value_description(RS2_OPTION_ADVANCED_MODE_PRESET, 1.f);
            auto range = dpt.get_option_range(RS2_OPTION_ADVANCED_MODE_PRESET);
            auto index_of_selected_preset = 0, counter = 0;
            auto units = dpt.get_depth_scale();

            for (auto i = range.min; i <= range.max; i += range.step, counter++)
            {
                 //if (range.step < 0.001f) return false;
                // what if (endpoint.get_option_value_description(RS2_OPTION_ADVANCED_MODE_PRESET, i) == nullptr)?

                /**
                   * get option value description (in case specific option value hold special meaning)
                   * \param[in] option     option id to be checked
                   * \param[in] value      value of the option
                   * \return human-readable (const char*) description of a specific value of an option or null if no special meaning
                   */
                presets_labels.push_back(dpt.get_option_value_description(RS2_OPTION_ADVANCED_MODE_PRESET, i));
                presets_numbers.push_back(i);
            }


            std::vector<pair<int,int>> resolutions_vec(resolutions.begin(), resolutions.end());



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
            config.enable_stream(RS2_STREAM_DEPTH, default_width, default_height, 30, RS2_FORMAT_Z16);
            //config.enable_stream(RS2_STREAM_DEPTH, 640, 360, 30, RS2_FORMAT_Z16);
            auto stream = config.open(dev);
            rs2_intrinsics current_frame_intrinsics = stream.get_intrinsics(RS2_STREAM_DEPTH);

            frame_queue calc_queue(1);
            frame_queue display_queue(1);

            stream.start([&](rs2::frame f)
            {
                calc_queue.enqueue(f);
                display_queue.enqueue(f);
            });

            // black.start(syncer);
            

            texture_buffer buffers[RS2_STREAM_COUNT];
            buffers[RS2_STREAM_DEPTH].equalize = false;
            buffers[RS2_STREAM_DEPTH].cm = &my_map;
            buffers[RS2_STREAM_DEPTH].min_depth = 0.2 / dpt.get_depth_scale();
            buffers[RS2_STREAM_DEPTH].max_depth = 1.5 / dpt.get_depth_scale();

            // Open a GLFW window
            glfwInit();
            ostringstream ss;
            ss << "Depth Sanity (" << dev.get_info(RS2_CAMERA_INFO_NAME) << ")";

            //dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)

            bool options_invalidated = false;
            std::string error_message;
            subdevice_model sub_mod_depth(dev, dpt, error_message);

            option_model metadata;

            for (auto i = 0; i < RS2_OPTION_COUNT; i++)
            {

                auto opt = static_cast<rs2_option>(i);
                if (opt == rs2_option::RS2_OPTION_ENABLE_AUTO_EXPOSURE )
                {
                    std::stringstream ss;
                    ss << dev.get_info(RS2_CAMERA_INFO_NAME)
                        << "/" << dpt.get_info(RS2_CAMERA_INFO_NAME)
                        << "/" << rs2_option_to_string(opt);
                    metadata.id = ss.str();
                    metadata.opt = opt;
                    //metadata.endpoint = s;
                    metadata.endpoint=dpt;
                    metadata.label = rs2_option_to_string(opt) + std::string("##") + ss.str();
                    metadata.invalidate_flag = &options_invalidated;
                    //metadata.dev = this;
                    metadata.dev =&sub_mod_depth;
                    metadata.supported = dpt.supports(opt);
                    if (metadata.supported)
                    {
                        try
                        {
                            metadata.range = dpt.get_option_range(opt);
                            metadata.read_only = dpt.is_option_read_only(opt);
                            if (!metadata.read_only)
                                metadata.value = dpt.get_option(opt);
                        }
                        catch (const error& e)
                        {
                            metadata.range = { 0, 1, 0, 0 };
                            metadata.value = 0;
                            error_message = error_to_string(e);
                        }
                    }
                    break;
                }
            }


            win = glfwCreateWindow(1280, 720, ss.str().c_str(), nullptr, nullptr);
            glfwMakeContextCurrent(win);


            ImGui_ImplGlfw_Init(win, true);

            metrics latest_stat{};
            latest_stat.width = 1280;
            latest_stat.height = 720;
            latest_stat.avg_dist = 0.0;
            latest_stat.std = 0.0;
            latest_stat._percentage_of_non_null_pixels = 0.0;
            std::mutex m;

            std::thread t([&m, &finished, &calc_queue, &units, &current_frame_intrinsics, &latest_stat, &box_size]() {
                while (!finished)
                {
                    auto f = calc_queue.wait_for_frame();

                    auto stream_type = f.get_stream_type();

                    if (stream_type == RS2_STREAM_DEPTH)
                    {
                        auto stats = analyze_depth_image(f, units, &current_frame_intrinsics, box_size);
                        
                        lock_guard<mutex> lock(m);
                        latest_stat = stats;
                        //cout << "Average distance is : " << latest_stat.avg_dist << endl;
                        //cout << "Standard_deviation is : " << latest_stat.std << endl;
                        //cout << "Percentage_of_non_null_pixels is : " << latest_stat._percentage_of_non_null_pixels << endl;
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

                auto stream_type = f.get_stream_type();

                if (stream_type == RS2_STREAM_DEPTH)
                {
                    buffers[stream_type].upload(f);
                }

                buffers[RS2_STREAM_DEPTH].show({ 0, 0, (float)w, (float)h }, 1);

                metrics stats_copy;
                {
                    lock_guard<mutex> lock(m);
                    stats_copy = latest_stat;
                }

                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0,0,0,0 });
                ImGui::SetNextWindowPos({ 0, 0 });
                ImGui::SetNextWindowSize({ (float)w, (float)h });
                ImGui::Begin("global", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);

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
                                         selected_resolution.second, fps, RS2_FORMAT_Z16);
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
                        dpt.set_option(RS2_OPTION_ADVANCED_MODE_PRESET, selected_preset);
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

                metadata.draw(error_message);

                ImGui::Checkbox("Use Plane-Fitting", &use_rect_fitting);
                ImGui::SliderInt("Windows Size", &box_size, 10, 175);

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

               ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.8f });

               ImGui::SetNextWindowPos({ w - 200 - margin, h - 180 - margin });
               ImGui::SetNextWindowSize({ 200, 180 });

//               ImGui::SetNextWindowPos({ 0.85*w, 0.90*h });
//               ImGui::SetNextWindowSize({ (.1*w), (.1*h)});

//               ImGui::SetNextWindowPos({ 1530, 930 });
//               ImGui::SetNextWindowSize({ 300.f, 370.f });

               ImGui::Begin("latest_stat", nullptr,
                                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
               //ImGui::SetWindowFontScale(w/1000);


               const int graph_size = 100;
               static float avgs[graph_size];
               static float stds[graph_size];
               static float fill[graph_size];
               static int avg_idx = 0;
               static int std_idx = 0;
               static int fill_idx = 0;

               avgs[avg_idx] = stats_copy.avg_dist * 100;
               avg_idx = (avg_idx + 1) % graph_size;

               stds[std_idx] = stats_copy.std * 100;
               std_idx = (std_idx + 1) % graph_size;

               fill[fill_idx] = stats_copy._percentage_of_non_null_pixels;
               fill_idx = (fill_idx + 1) % graph_size;

               stringstream ss_avg;
               ss_avg << "AVG = " << stats_copy.avg_dist * 100 << "(cm)";
               auto s_avg = ss_avg.str();
               ImGui::PlotLines("##AVG", avgs, graph_size, avg_idx, s_avg.c_str(), 0.f, 1.f, { 180, 50 });

               stringstream ss_std;
               ss_std << "STD = " << stats_copy.std * 100 << "(cm)";
               auto s_std = ss_std.str();
               ImGui::PlotLines("##STD", stds, graph_size, std_idx, s_std.c_str(), 0.f, 1.f, { 180, 50 });

               stringstream ss_fill;
               ss_fill << "FILL = " << stats_copy._percentage_of_non_null_pixels << "%";
               auto s_fill = ss_fill.str();
               ImGui::PlotLines("##STD", fill, graph_size, fill_idx, s_fill.c_str(), 0.f, 100.f, { 180, 50 });

               /*ImGui::Text("STD: %.3f(m)", stats_copy.std);*/
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
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
        catch (const exception & e)
        {
            cerr << e.what() << endl;
        }

        ImGui_ImplGlfw_Shutdown();
        glfwDestroyWindow(win);
        glfwTerminate();
    }
    return EXIT_SUCCESS;
}

