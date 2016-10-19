#include <librealsense/rs.hpp>
#include "example.hpp"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "concurrency.hpp"


#include <cstdarg>
#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <map>
#include <sstream>


#pragma comment(lib, "opengl32.lib")

bool is_integer(float f)
{
    return abs(f - floor(f)) < 0.001f;
}

struct option_metadata
{
    rs::option_range range;
    bool supported = false;
    float value = 0.0f;
    std::string label = "";
    std::string id = "";

    bool is_integers() const
    {
        return is_integer(range.min) && is_integer(range.max) &&
               is_integer(range.def) && is_integer(range.step);
    }

    bool is_checkbox() const
    {
        return range.max == 1.0f &&
            range.min == 0.0f &&
            range.step == 1.0f;
    }
};

template<class T>
void push_back_if_not_exists(std::vector<T>& vec, T value)
{
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it == vec.end()) vec.push_back(value);
}

std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec)
{
    std::vector<const char*> res;
    for (auto&& s : vec) res.push_back(s.c_str());
    return res;
}

class device_model
{
public:
    explicit device_model(rs::device& dev)
    {
        for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
        {
            auto subdevice = static_cast<rs_subdevice>(j);
            auto&& endpoint = dev.get_subdevice(subdevice);

            for (auto i = 0; i < RS_OPTION_COUNT; i++)
            {
                try
                {
                    option_metadata metadata;
                    auto opt = static_cast<rs_option>(i);

                    std::stringstream ss;
                    ss << rs_subdevice_to_string(subdevice) << "/" << rs_option_to_string(opt);
                    metadata.id = ss.str();

                    metadata.label = rs_option_to_string(opt);

                    metadata.supported = endpoint.supports(opt);
                    if (metadata.supported)
                    {
                        metadata.range = endpoint.get_option_range(opt);
                        metadata.value = endpoint.get_option(opt);
                    }
                    _options_metadata[subdevice][opt] = metadata;
                }
                catch (...) {}
            }
        }

        for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
        {
            auto subdevice = static_cast<rs_subdevice>(j);
            auto&& endpoint = dev.get_subdevice(subdevice);

            for (auto&& profile : endpoint.get_stream_profiles())
            {
                std::stringstream res;
                res << profile.width << " x " << profile.height;
                _width_values[subdevice].push_back(profile.width);
                _height_values[subdevice].push_back(profile.height);
                push_back_if_not_exists(_resolutions[subdevice], res.str());
                std::stringstream fps;
                fps << profile.fps;
                _fps_values[subdevice].push_back(profile.fps);
                push_back_if_not_exists(_fpses[subdevice], fps.str());
                std::string format = rs_format_to_string(profile.format);
                push_back_if_not_exists(_formats[subdevice], format);
                _format_values[subdevice].push_back(profile.format);

                _profiles[subdevice].push_back(profile);
            }
        }
    }

    rs::stream_profile get_selected_profile(rs_subdevice sub)
    {
        auto width = _width_values[sub][_selected_res_id[sub]];
        auto height = _height_values[sub][_selected_res_id[sub]];
        auto fps = _fps_values[sub][_selected_fps_id[sub]];
        auto format = _format_values[sub][_selected_format_id[sub]];

        for (auto&& p : _profiles[sub])
        {
            if (p.width == width && p.height == height && p.fps == fps && p.format == format)
                return p;
        }
        throw std::runtime_error("Profile not supported!");
    }

    bool is_stream_visible(rs_stream s)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - _steam_last_frame[s];
        auto ms = duration_cast<milliseconds>(diff).count();
        return (ms <= _frame_timeout + _min_timeout);
    }

    float get_stream_alpha(rs_stream s)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - _steam_last_frame[s];
        auto ms = duration_cast<milliseconds>(diff).count();
        if (ms > _frame_timeout + _min_timeout)
        {
            return 0.0f;
        }
        if (ms < _min_timeout)
        {
            return 1.0f;
        }
        return 1.0f - pow((ms - _min_timeout) / _frame_timeout, 2);
    }

    struct rect
    {
        int x, y;
        int w, h;

        bool operator==(const rect& other) const
        {
            return x == other.x && y == other.y && w == other.w && h == other.h;
        }
    };

    std::map<rs_stream, rect> calc_layout(int x0, int y0, int width, int height)
    {
        std::vector<rs_stream> active_streams;
        for (auto i = 0; i < RS_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs_stream>(i);
            if (is_stream_visible(stream))
            {
                active_streams.push_back(stream);
            }
        }

        auto factor = floor(sqrt(active_streams.size()));
        auto complement = ceil(active_streams.size() / factor);

        std::map<rs_stream, rect> results;
        auto i = 0;
        for (auto x = 0; x < complement; x++)
        {
            for (auto y = 0; y < factor; y++)
            {
                rect r = { x0 + x * (width / complement), y0 + y * (height / factor),
                              (width / complement), (height / factor) };
                if (i == active_streams.size()) return results;
                results[active_streams[i]] = r;
                i++;
            }
        }
        return get_interpolated_layout(results);
    }


    float _frame_timeout = 700.0f;
    float _min_timeout = 90.0f;
    std::map<rs_subdevice, std::map<rs_option, option_metadata>> _options_metadata;
    std::map<rs_subdevice, std::vector<std::string>> _resolutions;
    std::map<rs_subdevice, std::vector<std::string>> _fpses;
    std::map<rs_subdevice, std::vector<std::string>> _formats;

    std::map<rs_subdevice, int> _selected_res_id;
    std::map<rs_subdevice, int> _selected_fps_id;
    std::map<rs_subdevice, int> _selected_format_id;

    std::map<rs_subdevice, std::vector<int>> _width_values;
    std::map<rs_subdevice, std::vector<int>> _height_values;
    std::map<rs_subdevice, std::vector<int>> _fps_values;
    std::map<rs_subdevice, std::vector<rs_format>> _format_values;
    std::map<rs_subdevice, std::shared_ptr<rs::streaming_lock>> _streams;

    std::map<rs_subdevice, std::vector<rs::stream_profile>> _profiles;

    std::map<rs_subdevice, single_consumer_queue<rs::frame>> _queues;
    std::map<rs_stream, rect> _layout;
    std::map<rs_stream, rect> _old_layout;
    std::chrono::high_resolution_clock::time_point _transition_start_time;

    std::map<rs_stream, rect> get_interpolated_layout(const std::map<rs_stream, rect>& l)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        if (l != _layout)
        {
            _transition_start_time = now;
            _old_layout = _layout;
            _layout = l;
        }
        auto diff = now - _transition_start_time;
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = (ms > 100) ? 1.0f : sqrt(ms / 100.0f);

        std::map<rs_stream, rect> results;
        for (auto&& kvp : l)
        {
            auto stream = kvp.first;
            rect res{
                _layout[stream].x * t + _old_layout[stream].x * (1 - t),
                _layout[stream].y * t + _old_layout[stream].y * (1 - t),
                _layout[stream].w * t + _old_layout[stream].w * (1 - t),
                _layout[stream].h * t + _old_layout[stream].h * (1 - t),
            };
            results[stream] = res;
        }
        
        return results;
    }

    std::map<rs_stream, texture_buffer> _stream_buffers;
    std::map<rs_stream, std::pair<int, int>> _stream_size;
    std::map<rs_stream, std::chrono::high_resolution_clock::time_point> _steam_last_frame;
};

int main(int, char**) try
{
    if (!glfwInit())
        exit(1);

    auto window = glfwCreateWindow(1280, 720, "librealsense - config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(0.1, 0, 0);

    rs::context ctx;
    auto device_index = 0;
    auto list = ctx.query_devices();
    auto dev = ctx.create(list[device_index]);
    std::vector<std::string> device_names;
    for (auto&& l : list)
    {
        auto d = ctx.create(l);
        auto name = d.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME);
        auto serial = d.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER);
        std::stringstream ss;
        ss << name << " Sn#" << serial;
        device_names.push_back(ss.str());
    }

    auto model = device_model(dev);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        ImGui_ImplGlfw_NewFrame();

        auto flags = ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 300, static_cast<float>(h) });
        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("Viewer FPS: %.0f ", ImGui::GetIO().Framerate);

        if (ImGui::CollapsingHeader("Device Details", nullptr, true, true))
        {
            auto device_names_chars = get_string_pointers(device_names);
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("", &device_index, device_names_chars.data(), device_names.size()))
            {
                dev = ctx.create(list[device_index]);
                model = device_model(dev);
            }
            ImGui::PopItemWidth();

            for (auto i = 0; i < RS_CAMERA_INFO_COUNT; i++)
            {
                auto info = static_cast<rs_camera_info>(i);
                if (dev.supports(info))
                {
                    std::stringstream ss;
                    ss << rs_camera_info_to_string(info) << ":";
                    auto line = ss.str();
                    ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 0.5f });
                    ImGui::Text(line.c_str());
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    auto value = dev.get_camera_info(info);
                    ImGui::Text(value);

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip(value);
                    }
                }
            }
        }
        
        if (ImGui::CollapsingHeader("Streaming", nullptr, true, true))
        {
            for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
            {
                auto subdevice = static_cast<rs_subdevice>(j);
                auto&& endpoint = dev.get_subdevice(subdevice);

                std::stringstream label;
                label << rs_subdevice_to_string(subdevice) << " profile:";
                if (ImGui::CollapsingHeader(label.str().c_str(), nullptr, true, true))
                {
                    auto res_chars = get_string_pointers(model._resolutions[subdevice]);
                    auto fps_chars = get_string_pointers(model._fpses[subdevice]);
                    auto formats_chars = get_string_pointers(model._formats[subdevice]);

                    ImGui::Text("Resolution:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    std::stringstream label;
                    label << rs_subdevice_to_string(subdevice) << " resolution";
                    ImGui::Combo(label.str().c_str(), &model._selected_res_id[subdevice], res_chars.data(), res_chars.size());
                    ImGui::PopItemWidth();

                    ImGui::Text("FPS:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label.str("");
                    label << rs_subdevice_to_string(subdevice) << " fps";
                    ImGui::Combo(label.str().c_str() , &model._selected_fps_id[subdevice], fps_chars.data(), fps_chars.size());
                    ImGui::PopItemWidth();

                    ImGui::Text("Format:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label.str("");
                    label << rs_subdevice_to_string(subdevice) << " format";
                    ImGui::Combo(label.str().c_str(), &model._selected_format_id[subdevice], formats_chars.data(), formats_chars.size());
                    ImGui::PopItemWidth();

                    if (!model._streams[subdevice].get())
                    {
                        std::stringstream label;
                        label << "Play " << rs_subdevice_to_string(subdevice);
                        if (ImGui::Button(label.str().c_str()))
                        {
                            auto profile = model.get_selected_profile(subdevice);
                            model._streams[subdevice] = std::make_shared<rs::streaming_lock>(endpoint.open(profile));
                            model._streams[subdevice]->play(
                                [&model, subdevice](rs::frame f)
                            {
                                if (model._queues[subdevice].size() < 2)
                                    model._queues[subdevice].enqueue(std::move(f));
                            });
                        }
                    }
                    else
                    {
                        std::stringstream label;
                        label << "Stop " << rs_subdevice_to_string(subdevice);
                        if (ImGui::Button(label.str().c_str()))
                        {
                            model._queues[subdevice].clear();
                            model._streams[subdevice]->stop();
                            model._streams[subdevice].reset();
                        }
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("Controls", nullptr, true, true))
        {
            for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
            {
                auto subdevice = static_cast<rs_subdevice>(j);
                auto&& endpoint = dev.get_subdevice(subdevice);

                std::stringstream label;
                label << rs_subdevice_to_string(subdevice) << " options:";
                if (ImGui::CollapsingHeader(label.str().c_str(), nullptr, true, false))
                {
                    for (auto i = 0; i < RS_OPTION_COUNT; i++)
                    {
                        auto opt = static_cast<rs_option>(i);
                        auto&& metadata = model._options_metadata[subdevice][opt];

                        if (metadata.supported)
                        {
                            if (metadata.is_checkbox())
                            {
                                auto value = metadata.value > 0.0f;
                                if (ImGui::Checkbox(metadata.label.c_str(), &value))
                                {
                                    metadata.value = value ? 1.0f : 0.0f;
                                    endpoint.set_option(opt, metadata.value);
                                }
                            }
                            else
                            {
                                std::stringstream ss;
                                ss << metadata.label << ":";
                                ImGui::Text(ss.str().c_str());
                                ImGui::PushItemWidth(-1);

                                if (metadata.is_integers())
                                {
                                    int value = metadata.value;
                                    if (ImGui::SliderInt(metadata.id.c_str(), &value,
                                        metadata.range.min, metadata.range.max))
                                    {
                                        // TODO: Round to step?
                                        metadata.value = value;
                                        endpoint.set_option(opt, metadata.value);
                                    }
                                }
                                else
                                {
                                    if (ImGui::SliderFloat(metadata.id.c_str(), &metadata.value,
                                        metadata.range.min, metadata.range.max))
                                    {
                                        // TODO: Round to step?
                                        endpoint.set_option(opt, metadata.value);
                                    }
                                }
                                ImGui::PopItemWidth();
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip(metadata.id.c_str());
                            }
                        }
                    }
                }
            }
        }

        ImGui::End();

        // Rendering
        glViewport(0, 0, 
            static_cast<int>(ImGui::GetIO().DisplaySize.x), 
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(window, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        for (auto i = 0; i < RS_SUBDEVICE_COUNT; i++)
        {
            auto subdevice = static_cast<rs_subdevice>(i);
            rs::frame f;
            if (model._queues[subdevice].try_dequeue(&f))
            {
                model._stream_buffers[f.get_stream_type()].upload(f);
                model._steam_last_frame[f.get_stream_type()] = std::chrono::high_resolution_clock::now();
                model._stream_size[f.get_stream_type()] = std::make_pair<int, int>(f.get_width(), f.get_height());
            }
           
        }

        auto layout = model.calc_layout(300, 0, w - 300, h);
        for(auto kvp : layout)
        {
            model._stream_buffers[kvp.first].show(kvp.second.x, kvp.second.y, kvp.second.w, kvp.second.h,
                model._stream_size[kvp.first].first,
                model._stream_size[kvp.first].second,
                model.get_stream_alpha(kvp.first));
        }

        ImGui::Render();

        glfwSwapBuffers(window);
    }

    for (auto i = 0; i < RS_SUBDEVICE_COUNT; i++)
    {
        auto subdevice = static_cast<rs_subdevice>(i);
        model._queues[subdevice].clear();
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
