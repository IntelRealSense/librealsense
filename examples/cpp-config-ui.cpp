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

struct to_string
{
    std::ostringstream ss;
    template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

struct option_model
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

class subdevice_model
{
public:
    subdevice_model(rs_subdevice subdevice, rs::subdevice* endpoint)
        : subdevice(subdevice), endpoint(endpoint)
    {
        for (auto i = 0; i < RS_OPTION_COUNT; i++)
        {
            try
            {
                option_model metadata;
                auto opt = static_cast<rs_option>(i);

                std::stringstream ss;
                ss << rs_subdevice_to_string(subdevice) << "/" << rs_option_to_string(opt);
                metadata.id = ss.str();

                metadata.label = rs_option_to_string(opt);

                metadata.supported = endpoint->supports(opt);
                if (metadata.supported)
                {
                    metadata.range = endpoint->get_option_range(opt);
                    metadata.value = endpoint->get_option(opt);
                }
                options_metadata[opt] = metadata;
            }
            catch (...) {}
        }

        for (auto&& profile : endpoint->get_stream_profiles())
        {
            std::stringstream res;
            res << profile.width << " x " << profile.height;
            width_values.push_back(profile.width);
            height_values.push_back(profile.height);
            push_back_if_not_exists(resolutions, res.str());
            std::stringstream fps;
            fps << profile.fps;
            fps_values.push_back(profile.fps);
            push_back_if_not_exists(fpses, fps.str());
            std::string format = rs_format_to_string(profile.format);
            push_back_if_not_exists(formats, format);
            format_values.push_back(profile.format);

            profiles.push_back(profile);
        }
    }


    rs::stream_profile get_selected_profile()
    {
        auto width = width_values[selected_res_id];
        auto height = height_values[selected_res_id];
        auto fps = fps_values[selected_fps_id];
        auto format = format_values[selected_format_id];

        for (auto&& p : profiles)
        {
            if (p.width == width && p.height == height && p.fps == fps && p.format == format)
                return p;
        }
        throw std::runtime_error("Profile not supported!");
    }

    rs_subdevice subdevice;
    rs::subdevice* endpoint;

    std::map<rs_option, option_model> options_metadata;
    std::vector<std::string> resolutions;
    std::vector<std::string> fpses;
    std::vector<std::string> formats;

    int selected_res_id = 0;
    int selected_fps_id = 0;
    int selected_format_id = 0;

    std::vector<int> width_values;
    std::vector<int> height_values;
    std::vector<int> fps_values;
    std::vector<rs_format> format_values;
    std::shared_ptr<rs::streaming_lock> streams;

    std::vector<rs::stream_profile> profiles;

    single_consumer_queue<rs::frame> queues;
};

struct rect
{
    int x, y;
    int w, h;

    bool operator==(const rect& other) const
    {
        return x == other.x && y == other.y && w == other.w && h == other.h;
    }
};
typedef std::map<rs_stream, rect> streams_layout;

class device_model
{
public:
    explicit device_model(rs::device& dev)
    {
        for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
        {
            auto subdevice = static_cast<rs_subdevice>(j);
            auto&& endpoint = dev.get_subdevice(subdevice);

            auto model = std::make_shared<subdevice_model>(subdevice, &endpoint);
            subdevices.push_back(model);
        }
    }


    bool is_stream_visible(rs_stream s)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - steam_last_frame[s];
        auto ms = duration_cast<milliseconds>(diff).count();
        return (ms <= _frame_timeout + _min_timeout);
    }

    float get_stream_alpha(rs_stream s)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - steam_last_frame[s];
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

    std::vector<std::shared_ptr<subdevice_model>> subdevices;
    std::map<rs_stream, texture_buffer> stream_buffers;
    std::map<rs_stream, std::pair<int, int>> stream_size;
    std::map<rs_stream, std::chrono::high_resolution_clock::time_point> steam_last_frame;

private:
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

        if (_old_layout.size() == 0 && l.size() == 1) return l;

        auto diff = now - _transition_start_time;
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = (ms > 100) ? 1.0f : sqrt(ms / 100.0f);

        std::map<rs_stream, rect> results;
        for (auto&& kvp : l)
        {
            auto stream = kvp.first;
            if (_old_layout.find(stream) == _old_layout.end())
            {
                _old_layout[stream] = rect{
                    _layout[stream].x + _layout[stream].w / 2,
                    _layout[stream].y + _layout[stream].h / 2,
                    0, 0
                };
            }
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

    float _frame_timeout = 700.0f;
    float _min_timeout = 90.0f;

    streams_layout _layout;
    streams_layout _old_layout;
    std::chrono::high_resolution_clock::time_point _transition_start_time;
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
    std::string label;

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
            for (auto&& sub : model.subdevices)
            {
                label = to_string() << rs_subdevice_to_string(sub->subdevice);
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, true))
                {
                    auto res_chars = get_string_pointers(sub->resolutions);
                    auto fps_chars = get_string_pointers(sub->fpses);
                    auto formats_chars = get_string_pointers(sub->formats);

                    ImGui::Text("Resolution:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << rs_subdevice_to_string(sub->subdevice) << " resolution";
                    ImGui::Combo(label.c_str(), &sub->selected_res_id, res_chars.data(), res_chars.size());
                    ImGui::PopItemWidth();

                    ImGui::Text("FPS:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << rs_subdevice_to_string(sub->subdevice) << " fps";
                    ImGui::Combo(label.c_str() , &sub->selected_fps_id, fps_chars.data(), fps_chars.size());
                    ImGui::PopItemWidth();

                    ImGui::Text("Format:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << rs_subdevice_to_string(sub->subdevice) << " format";
                    ImGui::Combo(label.c_str(), &sub->selected_format_id, formats_chars.data(), formats_chars.size());
                    ImGui::PopItemWidth();

                    if (!sub->streams.get())
                    {
                        label = to_string() << "Play " << rs_subdevice_to_string(sub->subdevice);
                        if (ImGui::Button(label.c_str()))
                        {
                            auto profile = sub->get_selected_profile();
                            sub->streams = std::make_shared<rs::streaming_lock>(sub->endpoint->open(profile));
                            sub->streams->play(
                                [sub](rs::frame f)
                            {
                                if (sub->queues.size() < 2)
                                    sub->queues.enqueue(std::move(f));
                            });
                        }
                    }
                    else
                    {
                        label = to_string() << "Stop " << rs_subdevice_to_string(sub->subdevice);
                        if (ImGui::Button(label.c_str()))
                        {
                            sub->queues.clear();
                            sub->streams->stop();
                            sub->streams.reset();
                        }
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("Controls", nullptr, true, true))
        {
            for (auto&& sub : model.subdevices)
            {
                label = to_string() << rs_subdevice_to_string(sub->subdevice) << " options:";
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, false))
                {
                    for (auto i = 0; i < RS_OPTION_COUNT; i++)
                    {
                        auto opt = static_cast<rs_option>(i);
                        auto&& metadata = sub->options_metadata[opt];

                        if (metadata.supported)
                        {
                            if (metadata.is_checkbox())
                            {
                                auto value = metadata.value > 0.0f;
                                if (ImGui::Checkbox(metadata.label.c_str(), &value))
                                {
                                    metadata.value = value ? 1.0f : 0.0f;
                                    sub->endpoint->set_option(opt, metadata.value);
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
                                        sub->endpoint->set_option(opt, metadata.value);
                                    }
                                }
                                else
                                {
                                    if (ImGui::SliderFloat(metadata.id.c_str(), &metadata.value,
                                        metadata.range.min, metadata.range.max))
                                    {
                                        // TODO: Round to step?
                                        sub->endpoint->set_option(opt, metadata.value);
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

        for (auto&& sub : model.subdevices)
        {
            rs::frame f;
            if (sub->queues.try_dequeue(&f))
            {
                model.stream_buffers[f.get_stream_type()].upload(f);
                model.steam_last_frame[f.get_stream_type()] = std::chrono::high_resolution_clock::now();
                model.stream_size[f.get_stream_type()] = std::make_pair<int, int>(f.get_width(), f.get_height());
            }
           
        }

        auto layout = model.calc_layout(300, 0, w - 300, h);
        for(auto kvp : layout)
        {
            model.stream_buffers[kvp.first].show(kvp.second.x, kvp.second.y, kvp.second.w, kvp.second.h,
                model.stream_size[kvp.first].first,
                model.stream_size[kvp.first].second,
                model.get_stream_alpha(kvp.first));
        }

        ImGui::Render();

        glfwSwapBuffers(window);
    }

    for (auto&& sub : model.subdevices)
    {
        sub->queues.clear();
        sub->streams.reset();
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
