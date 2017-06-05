#include <librealsense/rs2.hpp>
#include "example.hpp"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"

#include <cstdarg>
#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <array>
#include <mutex>

#pragma comment(lib, "opengl32.lib")

#define WHITE_SPACES std::string("                                        ")

using namespace rs2;

class subdevice_model;

class option_model
{
public:
    rs2_option opt;
    option_range range;
    device endpoint;
    bool* invalidate_flag;
    bool supported = false;
    bool read_only = false;
    float value = 0.0f;
    std::string label = "";
    std::string id = "";
    subdevice_model* dev;

    void draw(std::string& error_message)
    {
        if (supported)
        {
            auto desc = endpoint.get_option_description(opt);

            if (is_checkbox())
            {
                auto bool_value = value > 0.0f;
                if (ImGui::Checkbox(label.c_str(), &bool_value))
                {
                    value = bool_value ? 1.0f : 0.0f;
                    try
                    {
                        endpoint.set_option(opt, value);
                        *invalidate_flag = true;
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }
                if (ImGui::IsItemHovered() && desc)
                {
                    ImGui::SetTooltip("%s", desc);
                }
            }
            else
            {
                std::string txt = to_string() << rs2_option_to_string(opt) << ":";
                ImGui::Text("%s", txt.c_str());

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 0.5f, 0.5f, 1.f });
                ImGui::Text("(?)");
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered() && desc)
                {
                    ImGui::SetTooltip("%s", desc);
                }

                ImGui::PushItemWidth(-1);

                try
                {
                    if (read_only)
                    {
                        ImVec2 vec{0, 14};
                        ImGui::ProgressBar(value/100.f, vec, std::to_string((int)value).c_str());
                    }
                    else if (is_enum())
                    {
                        std::vector<const char*> labels;
                        auto selected = 0, counter = 0;
                        for (auto i = range.min; i <= range.max; i += range.step, counter++)
                        {
                            if (abs(i - value) < 0.001f) selected = counter;
                            labels.push_back(endpoint.get_option_value_description(opt, i));
                        }
                        if (ImGui::Combo(id.c_str(), &selected, labels.data(),
                            static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * selected;
                            endpoint.set_option(opt, value);
                            *invalidate_flag = true;
                        }
                    }
                    else if (is_all_integers())
                    {
                        auto int_value = static_cast<int>(value);
                        if (ImGui::SliderInt(id.c_str(), &int_value,
                            static_cast<int>(range.min),
                            static_cast<int>(range.max)))
                        {
                            // TODO: Round to step?
                            value = static_cast<float>(int_value);
                            endpoint.set_option(opt, value);
                            *invalidate_flag = true;
                        }
                    }
                    else
                    {
                        if (ImGui::SliderFloat(id.c_str(), &value,
                            range.min, range.max, "%.4f"))
                        {
                            endpoint.set_option(opt, value);
                            *invalidate_flag = true;
                        }
                    }
                }
                catch (const error& e)
                {
                    error_message = error_to_string(e);
                }
                ImGui::PopItemWidth();
            }
        }
    }

    void update_supported(std::string& error_message)
    {
        try
        {
            supported =  endpoint.supports(opt);
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }

    void update_read_only(std::string& error_message)
    {
        try
        {
            read_only = endpoint.is_option_read_only(opt);
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }

    void update_all(std::string& error_message)
    {
        try
        {
            if (supported =  endpoint.supports(opt))
            {
                value = endpoint.get_option(opt);
                range = endpoint.get_option_range(opt);
                read_only = endpoint.is_option_read_only(opt);
            }
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }
private:
    bool is_all_integers() const
    {
        return is_integer(range.min) && is_integer(range.max) &&
            is_integer(range.def) && is_integer(range.step);
    }

    bool is_enum() const
    {
        if (range.step < 0.001f) return false;

        for (auto i = range.min; i <= range.max; i += range.step)
        {
            if (endpoint.get_option_value_description(opt, i) == nullptr)
                return false;
        }
        return true;
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

struct mouse_info
{
    float2 cursor;
    bool mouse_down = false;
};


class subdevice_model
{
public:
    subdevice_model(device dev, std::string& error_message)
        : dev(dev), streaming(false), queues(RS2_STREAM_COUNT),
          selected_shared_fps_id(0)
    {
        for (auto& elem : queues)
        {
            elem = std::unique_ptr<frame_queue>(new frame_queue(5));
        }

        try
        {
            if (dev.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
                auto_exposure_enabled = dev.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) > 0;
        }
        catch(...)
        {

        }

        try
        {
            if (dev.supports(RS2_OPTION_DEPTH_UNITS))
                depth_units = dev.get_option(RS2_OPTION_DEPTH_UNITS);
        }
        catch(...)
        {

        }

        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            option_model metadata;
            auto opt = static_cast<rs2_option>(i);

            std::stringstream ss;
            ss << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
                << "/" << dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME)
                << "/" << rs2_option_to_string(opt);
            metadata.id = ss.str();
            metadata.opt = opt;
            metadata.endpoint = dev;
            metadata.label = rs2_option_to_string(opt) + WHITE_SPACES + ss.str();
            metadata.invalidate_flag = &options_invalidated;
            metadata.dev = this;

            metadata.supported = dev.supports(opt);
            if (metadata.supported)
            {
                try
                {
                    metadata.range = dev.get_option_range(opt);
                    metadata.read_only = dev.is_option_read_only(opt);
                    if (!metadata.read_only)
                        metadata.value = dev.get_option(opt);
                }
                catch (const error& e)
                {
                    metadata.range = { 0, 1, 0, 0 };
                    metadata.value = 0;
                    error_message = error_to_string(e);
                }
            }
            options_metadata[opt] = metadata;
        }

        try
        {
            auto uvc_profiles = dev.get_stream_modes();
            std::reverse(std::begin(uvc_profiles), std::end(uvc_profiles));
            for (auto&& profile : uvc_profiles)
            {
                std::stringstream res;
                res << profile.width << " x " << profile.height;
                push_back_if_not_exists(res_values, std::pair<int, int>(profile.width, profile.height));
                push_back_if_not_exists(resolutions, res.str());
                std::stringstream fps;
                fps << profile.fps;
                push_back_if_not_exists(fps_values_per_stream[profile.stream], profile.fps);
                push_back_if_not_exists(shared_fps_values, profile.fps);
                push_back_if_not_exists(fpses_per_stream[profile.stream], fps.str());
                push_back_if_not_exists(shared_fpses, fps.str());

                std::string format = rs2_format_to_string(profile.format);

                push_back_if_not_exists(formats[profile.stream], format);
                push_back_if_not_exists(format_values[profile.stream], profile.format);

                auto any_stream_enabled = false;
                for (auto it : stream_enabled)
                {
                    if (it.second)
                    {
                        any_stream_enabled = true;
                        break;
                    }
                }
                if (!any_stream_enabled)
                {
                    stream_enabled[profile.stream] = true;
                }

                profiles.push_back(profile);
            }

            show_single_fps_list = is_there_common_fps();

            // set default selections
            int selection_index;

            get_default_selection_index(res_values, std::pair<int,int>(640,480), &selection_index);
            selected_res_id = selection_index;


            if (!show_single_fps_list)
            {
                for (auto fps_array : fps_values_per_stream)
                {
                    if (get_default_selection_index(fps_array.second, 30, &selection_index))
                    {
                        selected_fps_id[fps_array.first] = selection_index;
                        break;
                    }
                }
            }
            else
            {
                if (get_default_selection_index(shared_fps_values, 30, &selection_index))
                    selected_shared_fps_id = selection_index;
            }

            for (auto format_array : format_values)
            {
                for (auto format : { rs2_format::RS2_FORMAT_RGB8,
                                     rs2_format::RS2_FORMAT_Z16,
                                     rs2_format::RS2_FORMAT_Y8,
                                     rs2_format::RS2_FORMAT_MOTION_XYZ32F } )
                {
                    if (get_default_selection_index(format_array.second, format, &selection_index))
                    {
                        selected_format_id[format_array.first] = selection_index;
                        break;
                    }
                }
            }
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }


    bool is_there_common_fps()
    {
        std::vector<int> first_fps_group;
        auto group_index = 0;
        for (; group_index < fps_values_per_stream.size() ; ++group_index)
        {
            if (!fps_values_per_stream[(rs2_stream)group_index].empty())
            {
                first_fps_group = fps_values_per_stream[(rs2_stream)group_index];
                break;
            }
        }

        for (int i = group_index + 1 ; i < fps_values_per_stream.size() ; ++i)
        {
            auto fps_group = fps_values_per_stream[(rs2_stream)i];
            if (fps_group.empty())
                continue;

            for (auto& fps1 : first_fps_group)
            {
                auto it = std::find_if(std::begin(fps_group),
                                       std::end(fps_group),
                                       [&](const int& fps2)
                {
                    return fps2 == fps1;
                });
                if (it != std::end(fps_group))
                {
                    break;
                }
                return false;
            }
        }
        return true;
    }

    void draw_stream_selection()
    {
        // Draw combo-box with all resolution options for this device
        auto res_chars = get_string_pointers(resolutions);
        ImGui::PushItemWidth(-1);
        ImGui::Text("Resolution:");
        ImGui::SameLine();
        std::string label = to_string() << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
            << dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME) << " resolution";
        if (streaming)
            ImGui::Text("%s", res_chars[selected_res_id]);
        else
        {
            ImGui::Combo(label.c_str(), &selected_res_id, res_chars.data(),
                static_cast<int>(res_chars.size()));
        }

        ImGui::PopItemWidth();

        // FPS
        if (show_single_fps_list)
        {
            auto fps_chars = get_string_pointers(shared_fpses);
            ImGui::Text("FPS:       ");
            label = to_string() << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
                << dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME) << " fps";

            ImGui::SameLine();
            ImGui::PushItemWidth(-1);

            if (streaming)
            {
                ImGui::Text("%s", fps_chars[selected_shared_fps_id]);
            }
            else
            {
                ImGui::Combo(label.c_str(), &selected_shared_fps_id, fps_chars.data(),
                    static_cast<int>(fps_chars.size()));
            }

            ImGui::PopItemWidth();
        }

        // Check which streams are live in current device
        auto live_streams = 0;
        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs2_stream>(i);
            if (formats[stream].size() > 0)
                live_streams++;
        }

        // Draw combo-box with all format options for current device
        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs2_stream>(i);

            // Format
            if (formats[stream].size() == 0)
                continue;

            ImGui::PushItemWidth(-1);
            auto formats_chars = get_string_pointers(formats[stream]);
            if (!streaming || (streaming && stream_enabled[stream]))
            {
                if (live_streams > 1)
                {
                    label = to_string() << rs2_stream_to_string(stream);
                    if (!show_single_fps_list)
                        label += " stream:";

                    if (streaming)
                        ImGui::Text("%s", label.c_str());
                    else
                        ImGui::Checkbox(label.c_str(), &stream_enabled[stream]);
                }
            }

            if (stream_enabled[stream])
            {
                if (show_single_fps_list) ImGui::SameLine();

                label = to_string() << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
                    << dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME)
                    << " " << rs2_stream_to_string(stream) << " format";

                if (!show_single_fps_list)
                {
                    ImGui::Text("Format:    ");
                    ImGui::SameLine();
                }

                if (streaming)
                {
                    ImGui::Text("%s", formats_chars[selected_format_id[stream]]);
                }
                else
                {
                    ImGui::Combo(label.c_str(), &selected_format_id[stream], formats_chars.data(),
                        static_cast<int>(formats_chars.size()));
                }

                // FPS
                // Draw combo-box with all FPS options for this device
                if (!show_single_fps_list && !fpses_per_stream[stream].empty() && stream_enabled[stream])
                {
                    auto fps_chars = get_string_pointers(fpses_per_stream[stream]);
                    ImGui::Text("FPS:       ");
                    ImGui::SameLine();

                    label = to_string() << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
                        << dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME)
                        << rs2_stream_to_string(stream) << " fps";

                    if (streaming)
                    {
                        ImGui::Text("%s", fps_chars[selected_fps_id[stream]]);
                    }
                    else
                    {
                        ImGui::Combo(label.c_str(), &selected_fps_id[stream], fps_chars.data(),
                            static_cast<int>(fps_chars.size()));
                    }
                }
            }
            ImGui::PopItemWidth();
        }
    }

    bool is_selected_combination_supported()
    {
        std::vector<stream_profile> results;

        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs2_stream>(i);
            if (stream_enabled[stream])
            {
                auto width = res_values[selected_res_id].first;
                auto height = res_values[selected_res_id].second;

                auto fps = 0;
                if (show_single_fps_list)
                    fps = shared_fps_values[selected_shared_fps_id];
                else
                    fps = fps_values_per_stream[stream][selected_fps_id[stream]];

                auto format = format_values[stream][selected_format_id[stream]];

                for (auto&& p : profiles)
                {
                    if (p.width == width && p.height == height && p.fps == fps && p.format == format)
                        results.push_back(p);
                }
            }
        }
        return results.size() > 0;
    }

    std::vector<stream_profile> get_selected_profiles()
    {
        std::vector<stream_profile> results;

        std::stringstream error_message;
        error_message << "The profile ";

        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs2_stream>(i);
            if (stream_enabled[stream])
            {
                auto width = res_values[selected_res_id].first;
                auto height = res_values[selected_res_id].second;
                auto format = format_values[stream][selected_format_id[stream]];

                auto fps = 0;
                if (show_single_fps_list)
                    fps = shared_fps_values[selected_shared_fps_id];
                else
                    fps = fps_values_per_stream[stream][selected_fps_id[stream]];



                error_message << "\n{" << rs2_stream_to_string(stream) << ","
                    << width << "x" << height << " at " << fps << "Hz, "
                    << rs2_format_to_string(format) << "} ";

                for (auto&& p : profiles)
                {
                    if (p.width == width &&
                        p.height == height &&
                        p.fps == fps &&
                        p.format == format &&
                        p.stream == stream)
                        results.push_back(p);
                }
            }
        }
        if (results.size() == 0)
        {
            error_message << " is unsupported!";
            throw std::runtime_error(error_message.str());
        }
        return results;
    }

    void stop()
    {
        streaming = false;

        for (auto& elem : queues)
            elem->flush();

        dev.stop();
        dev.close();
    }

    void play(const std::vector<stream_profile>& profiles)
    {
        dev.open(profiles);
        try {
            dev.start([&](frame f){
                auto stream_type = f.get_stream_type();
                queues[(int)stream_type]->enqueue(std::move(f));
            });
        }
        catch (...)
        {
            dev.close();
            throw;
        }

        streaming = true;
    }

    void update(std::string& error_message)
    {
        if (options_invalidated)
        {
            next_option = 0;
            options_invalidated = false;
        }
        if (next_option < RS2_OPTION_COUNT)
        {
            auto& opt_md = options_metadata[static_cast<rs2_option>(next_option)];
            opt_md.update_all(error_message);

            if (next_option == RS2_OPTION_ENABLE_AUTO_EXPOSURE)
            {
                auto old_ae_enabled = auto_exposure_enabled;
                auto_exposure_enabled = opt_md.value > 0;

                if (!old_ae_enabled && auto_exposure_enabled)
                {
                    try
                    {
                        auto roi = dev.get_region_of_interest();
                        roi_rect.x = roi.min_x;
                        roi_rect.y = roi.min_y;
                        roi_rect.w = roi.max_x - roi.min_x;
                        roi_rect.h = roi.max_y - roi.min_y;
                    }
                    catch (...)
                    {
                        auto_exposure_enabled = false;
                    }
                }


            }

            if (next_option == RS2_OPTION_DEPTH_UNITS)
            {
                opt_md.dev->depth_units = opt_md.value;
            }

            next_option++;
        }
    }

    template<typename T>
    bool get_default_selection_index(const std::vector<T>& values, const T & def, int* index)
    {
        auto max_default = values.begin();
        for (auto it = values.begin(); it != values.end(); it++)
        {

            if (*it == def)
            {
                *index = (int)(it - values.begin());
                return true;
            }
            if (*max_default < *it)
            {
                max_default = it;
            }
        }
        *index = (int)(max_default - values.begin());
        return false;
    }

    device dev;

    std::map<rs2_option, option_model> options_metadata;
    std::vector<std::string> resolutions;
    std::map<rs2_stream, std::vector<std::string>> fpses_per_stream;
    std::vector<std::string> shared_fpses;
    std::map<rs2_stream, std::vector<std::string>> formats;
    std::map<rs2_stream, bool> stream_enabled;

    int selected_res_id = 0;
    std::map<rs2_stream, int> selected_fps_id;
    int selected_shared_fps_id = 0;
    std::map<rs2_stream, int> selected_format_id;

    std::vector<std::pair<int, int>> res_values;
    std::map<rs2_stream, std::vector<int>> fps_values_per_stream;
    std::vector<int> shared_fps_values;
    bool show_single_fps_list = false;
    std::map<rs2_stream, std::vector<rs2_format>> format_values;

    std::vector<stream_profile> profiles;

    std::vector<std::unique_ptr<frame_queue>> queues;
    bool options_invalidated = false;
    int next_option = RS2_OPTION_COUNT;
    bool streaming = false;

    rect roi_rect;
    bool auto_exposure_enabled = false;
    float depth_units = 1.f;
};


class fps_calc
{
public:
    fps_calc()
        : _counter(0),
          _delta(0),
          _last_timestamp(0),
          _num_of_frames(0)
    {}

    fps_calc(const fps_calc& other) 
    {
        std::lock_guard<std::mutex> lock(other._mtx);
        _counter = other._counter;
        _delta = other._delta;
        _num_of_frames = other._num_of_frames;
        _last_timestamp = other._last_timestamp;
    }

    void add_timestamp(double timestamp, unsigned long long frame_counter)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (++_counter >= _skip_frames)
        {
            if (_last_timestamp != 0)
            {
                _delta = timestamp - _last_timestamp;
                _num_of_frames = frame_counter - _last_frame_counter;
            }

            _last_frame_counter = frame_counter;
            _last_timestamp = timestamp;
            _counter = 0;
        }
    }

    double get_fps() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_delta == 0)
            return 0;

        return (static_cast<double>(_numerator) * _num_of_frames)/_delta;
    }

private:
    static const int _numerator = 1000;
    static const int _skip_frames = 5;
    unsigned long long _num_of_frames;
    int _counter;
    double _delta;
    double _last_timestamp;
    unsigned long long _last_frame_counter;
    mutable std::mutex _mtx;
};

typedef std::map<rs2_stream, rect> streams_layout;

struct frame_metadata
{
    std::array<std::pair<bool,rs2_metadata_t>,RS2_FRAME_METADATA_COUNT> md_attributes{};
};

class stream_model
{
public:

    rect layout;
    std::unique_ptr<texture_buffer> texture;
    float2 size;
    rs2_stream          stream;
    rs2_format format;

    std::chrono::high_resolution_clock::time_point last_frame;
    double              timestamp;
    unsigned long long  frame_number;
    rs2_timestamp_domain timestamp_domain;
    fps_calc            fps;
    rect                roi_display_rect{};
    frame_metadata      frame_md;
    bool                metadata_displayed  = false;
    bool                roi_supported       = false;    // supported by device in its current state
    bool                roi_checked         = false;    // actively selected by user
    bool                capturing_roi       = false;    // active modification of roi
    std::shared_ptr<subdevice_model> dev;

    stream_model()
        :texture ( std::unique_ptr<texture_buffer>(new texture_buffer()))
    {}

    void upload_frame(frame&& f)
    {
        last_frame = std::chrono::high_resolution_clock::now();

        auto is_motion = ((f.get_format() == RS2_FORMAT_MOTION_RAW) || (f.get_format() == RS2_FORMAT_MOTION_XYZ32F));
        auto width = (is_motion) ? 640.f : f.get_width();
        auto height = (is_motion) ? 480.f : f.get_height();

        size = { static_cast<float>(width), static_cast<float>(height)};
        stream = f.get_stream_type();
        format = f.get_format();
        frame_number = f.get_frame_number();
        timestamp_domain = f.get_frame_timestamp_domain();
        timestamp = f.get_timestamp();
        fps.add_timestamp(f.get_timestamp(), f.get_frame_number());


        // populate frame metadata attributes
        for (auto i=0; i< RS2_FRAME_METADATA_COUNT; i++)
        {
            if (f.supports_frame_metadata((rs2_frame_metadata)i))
                frame_md.md_attributes[i] = std::make_pair(true,f.get_frame_metadata((rs2_frame_metadata)i));
            else
                frame_md.md_attributes[i].first = false;
        }

        roi_supported =  (dev.get() && dev->auto_exposure_enabled);

        texture->upload(f);

    }

    void outline_rect(const rect& r)
    {
        glPushAttrib(GL_ENABLE_BIT);

        glLineWidth(1);
        glLineStipple(1, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);

        glBegin(GL_LINE_STRIP);
        glVertex2i(r.x, r.y);
        glVertex2i(r.x, r.y + r.h);
        glVertex2i(r.x + r.w, r.y + r.h);
        glVertex2i(r.x + r.w, r.y);
        glVertex2i(r.x, r.y);
        glEnd();

        glPopAttrib();
    }

    float get_stream_alpha()
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - last_frame;
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = smoothstep(static_cast<float>(ms),
            _min_timeout, _min_timeout + _frame_timeout);
        return 1.0f - t;
    }

    bool is_stream_visible()
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - last_frame;
        auto ms = duration_cast<milliseconds>(diff).count();
        return ms <= _frame_timeout + _min_timeout;
    }

    void update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message)
    {
        if (roi_checked)
        {
            // Case 1: Starting Dragging of the ROI rect
            // Pre-condition: not capturing already + mouse is down + we are inside stream rect
            if (!capturing_roi && mouse.mouse_down && stream_rect.contains(mouse.cursor))
            {
                // Initialize roi_display_rect with drag-start position
                roi_display_rect.x = mouse.cursor.x;
                roi_display_rect.y = mouse.cursor.y;
                roi_display_rect.w = 0; // Still unknown, will be update later
                roi_display_rect.h = 0;
                capturing_roi = true; // Mark that we are in process of capturing the ROI rect
            }
            // Case 2: We are in the middle of dragging (capturing) ROI rect and we did not leave the stream boundaries
            if (capturing_roi && stream_rect.contains(mouse.cursor))
            {
                // x,y remain the same, only update the width,height with new mouse position relative to starting mouse position
                roi_display_rect.w = mouse.cursor.x - roi_display_rect.x;
                roi_display_rect.h = mouse.cursor.y - roi_display_rect.y;
            }
            // Case 3: We are in middle of dragging (capturing) and mouse was released
            if (!mouse.mouse_down && capturing_roi && stream_rect.contains(mouse.cursor))
            {
                // Update width,height one last time
                roi_display_rect.w = mouse.cursor.x - roi_display_rect.x;
                roi_display_rect.h = mouse.cursor.y - roi_display_rect.y;
                capturing_roi = false; // Mark that we are no longer dragging

                if (roi_display_rect) // If the rect is not empty?
                {
                    // Convert from local (pixel) coordinate system to device coordinate system
                    auto r = roi_display_rect;
                    r.x = ((r.x - stream_rect.x) / stream_rect.w) * size.x;
                    r.y = ((r.y - stream_rect.y) / stream_rect.h) * size.y;
                    r.w = (r.w / stream_rect.w) * size.x;
                    r.h = (r.h / stream_rect.h) * size.y;
                    dev->roi_rect = r; // Store new rect in device coordinates into the subdevice object

                    // Send it to firmware:
                    // Step 1: get rid of negative width / height
                    region_of_interest roi;
                    roi.min_x = std::min(r.x, r.x + r.w);
                    roi.max_x = std::max(r.x, r.x + r.w);
                    roi.min_y = std::min(r.y, r.y + r.h);
                    roi.max_y = std::max(r.y, r.y + r.h);

                    try
                    {
                        // Step 2: send it to firmware
                        dev->dev.set_region_of_interest(roi);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }
                else // If the rect is empty
                {
                    try
                    {
                        // To reset ROI, just set ROI to the entire frame
                        auto x_margin = (int)size.x / 8;
                        auto y_margin = (int)size.y / 8;

                        // Default ROI behaviour is center 3/4 of the screen:
                        dev->dev.set_region_of_interest({ x_margin, y_margin,
                                                          (int)size.x - x_margin - 1,
                                                          (int)size.y - y_margin - 1 });

                        roi_display_rect = { 0, 0, 0, 0 };
                        dev->roi_rect = { 0, 0, 0, 0 };
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }
            }
            // If we left stream bounds while capturing, stop capturing
            if (capturing_roi && !stream_rect.contains(mouse.cursor))
            {
                capturing_roi = false;
            }

            // When not capturing, just refresh the ROI rect in case the stream box moved
            if (!capturing_roi)
            {
                auto r = dev->roi_rect; // Take the current from device, convert to local coordinates
                r.x = ((r.x / size.x) * stream_rect.w) + stream_rect.x;
                r.y = ((r.y / size.y) * stream_rect.h) + stream_rect.y;
                r.w = (r.w / size.x) * stream_rect.w;
                r.h = (r.h / size.y) * stream_rect.h;
                roi_display_rect = r;
            }

            // Display ROI rect
            glColor3f(1.0f, 1.0f, 1.0f);
            outline_rect(roi_display_rect);
        }
    }

    void show_frame(const rect& stream_rect, const mouse_info& g, std::string& error_message)
    {
        texture->show(stream_rect, get_stream_alpha());

        update_ae_roi_rect(stream_rect, g, error_message);
    }

    void show_metadata(const mouse_info& g)
    {

        auto lines = RS2_FRAME_METADATA_COUNT;
        auto flags = ImGuiWindowFlags_ShowBorders;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.3f, 0.3f, 0.5});
        ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.f, 0.25f, 0.3f, 1 });
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.f, 0.3f, 0.8f, 1 });
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 });

        std::string label = to_string() << rs2_stream_to_string(stream) << " Stream Metadata #";
        ImGui::Begin(label.c_str(), nullptr, flags);

        // Print all available frame metadata attributes
        for (size_t i=0; i < RS2_FRAME_METADATA_COUNT; i++ )
        {
            if (frame_md.md_attributes[i].first)
            {
                label = to_string() << rs2_frame_metadata_to_string((rs2_frame_metadata)i) << " = " << frame_md.md_attributes[i].second;
                ImGui::Text("%s", label.c_str());
            }
        }

        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

    }

    float _frame_timeout = 700.0f;
    float _min_timeout = 167.0f;
};

class device_model
{
public:
    device_model(){}
    void reset()
    {
        subdevices.resize(0);
        streams.clear();
    }

    explicit device_model(device& dev, std::string& error_message)
    {
        for (auto&& sub : dev.get_adjacent_devices())
        {
            auto model = std::make_shared<subdevice_model>(sub, error_message);
            subdevices.push_back(model);
        }
    }

    std::map<rs2_stream, rect> calc_layout(float x0, float y0, float width, float height)
    {
        std::vector<rs2_stream> active_streams;
        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs2_stream>(i);
            if (streams[stream].is_stream_visible())
            {
                active_streams.push_back(stream);
            }
        }

        if (fullscreen)
        {
            auto it = std::find(begin(active_streams), end(active_streams), selected_stream);
            if (it == end(active_streams)) fullscreen = false;
        }

        std::map<rs2_stream, rect> results;

        if (fullscreen)
        {
            results[selected_stream] = { static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(width), static_cast<float>(height) };
        }
        else
        {
            auto factor = ceil(sqrt(active_streams.size()));
            auto complement = ceil(active_streams.size() / factor);

            auto cell_width = static_cast<float>(width / factor);
            auto cell_height = static_cast<float>(height / complement);

            auto i = 0;
            for (auto x = 0; x < factor; x++)
            {
                for (auto y = 0; y < complement; y++)
                {
                    if (i == active_streams.size()) break;

                    rect r = { x0 + x * cell_width, y0 + y * cell_height,
                        cell_width, cell_height };
                    results[active_streams[i]] = r;
                    i++;
                }
            }
        }

        return get_interpolated_layout(results);
    }

    void upload_frame(frame&& f)
    {
        auto stream_type = f.get_stream_type();
        streams[stream_type].upload_frame(std::move(f));
    }

    std::vector<std::shared_ptr<subdevice_model>> subdevices;
    std::map<rs2_stream, stream_model> streams;
    bool fullscreen = false;
    bool metadata_supported = false;
    rs2_stream selected_stream = RS2_STREAM_ANY;

private:
    std::map<rs2_stream, rect> get_interpolated_layout(const std::map<rs2_stream, rect>& l)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        if (l != _layout) // detect layout change
        {
            _transition_start_time = now;
            _old_layout = _layout;
            _layout = l;
        }

        //if (_old_layout.size() == 0 && l.size() == 1) return l;

        auto diff = now - _transition_start_time;
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = smoothstep(static_cast<float>(ms), 0, 100);

        std::map<rs2_stream, rect> results;
        for (auto&& kvp : l)
        {
            auto stream = kvp.first;
            if (_old_layout.find(stream) == _old_layout.end())
            {
                _old_layout[stream] = _layout[stream].center();
            }
            results[stream] = _old_layout[stream].lerp(t, _layout[stream]);
        }

        return results;
    }

    streams_layout _layout;
    streams_layout _old_layout;
    std::chrono::high_resolution_clock::time_point _transition_start_time;
};

struct user_data
{
    GLFWwindow* curr_window = nullptr;
    mouse_info* mouse = nullptr;
};

struct notification_data
{
    notification_data(  std::string description,
                        double timestamp,
                        rs2_log_severity severity,
                        rs2_notification_category category)
        :   _description(description),
          _timestamp(timestamp),
          _severity(severity),
          _category(category){}


    rs2_notification_category get_category() const
    {
        return _category;
    }

    std::string get_description() const
    {
        return _description;
    }


    double get_timestamp() const
    {
        return _timestamp;
    }


    rs2_log_severity get_severity() const
    {
        return _severity;
    }

    std::string _description;
    double _timestamp;
    rs2_log_severity _severity;
    rs2_notification_category _category;
};

struct notification_model
{
    static const int MAX_LIFETIME_MS = 10000;

    notification_model()
    {
        message = "";
    }

    notification_model(const notification_data& n)
    {
        message = n.get_description();
        timestamp  = n.get_timestamp();
        severity = n.get_severity();
        created_time = std::chrono::high_resolution_clock::now();

    }

    int height = 60;

    int index;
    std::string message;
    double timestamp;
    rs2_log_severity severity;
    std::chrono::high_resolution_clock::time_point created_time;
    // TODO: Add more info

    double get_age_in_ms()
    {
        auto age = std::chrono::high_resolution_clock::now() - created_time;
        return std::chrono::duration_cast<std::chrono::milliseconds>(age).count();
    }

    void draw(int w, int y, notification_model& selected)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        auto ms = get_age_in_ms() / MAX_LIFETIME_MS;
        auto t = smoothstep(static_cast<float>(ms), 0.7f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0, 0, 1 - t });
        ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.5f, 0.2f, 0.2f, 1 - t });
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.6f, 0.2f, 0.2f, 1 - t });
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 - t });



        auto lines = std::count(message.begin(), message.end(), '\n')+1;
        ImGui::SetNextWindowPos({ float(w - 430), float(y) });
        ImGui::SetNextWindowSize({ float(500), float(lines*50) });
        height = lines*50 +10;
        std::string label = to_string() << "Hardware Notification #" << index;
        ImGui::Begin(label.c_str(), nullptr, flags);



        ImGui::Text("%s", message.c_str());

        if(lines == 1)
            ImGui::SameLine();

        ImGui::Text("(...)");

        if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered())
        {
            selected = *this;
        }

        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }
};



struct notifications_model
{
    std::vector<notification_model> pending_notifications;
    int index = 1;
    const int MAX_SIZE = 6;
    std::mutex m;

    void add_notification(const notification_data& n)
    {
        std::lock_guard<std::mutex> lock(m); // need to protect the pending_notifications queue because the insertion of notifications
                                             // done from the notifications callback and proccesing and removing of old notifications done from the main thread

        notification_model m(n);
        m.index = index++;
        pending_notifications.push_back(m);

        if (pending_notifications.size() > MAX_SIZE)
            pending_notifications.erase(pending_notifications.begin());
    }

    void draw(int w, int h, notification_model& selected)
    {
        std::lock_guard<std::mutex> lock(m);
        if (pending_notifications.size() > 0)
        {
            // loop over all notifications, remove "old" ones
            pending_notifications.erase(std::remove_if(std::begin(pending_notifications),
                                                       std::end(pending_notifications),
                                                       [&](notification_model& n)
            {
                return (n.get_age_in_ms() > notification_model::MAX_LIFETIME_MS);
            }), end(pending_notifications));

            int idx = 0;
            auto height = 30;
            for (auto& noti : pending_notifications)
            {
                noti.draw(w, height, selected);
                height += noti.height;
                idx++;
            }
        }


        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
        ImGui::Begin("Notification parent window", nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.1f, 0, 0, 1 });
        ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.3f, 0.1f, 0.1f, 1 });
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.5f, 0.1f, 0.1f, 1 });
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 });

        if (selected.message != "")
            ImGui::OpenPopup("Notification from Hardware");
        if (ImGui::BeginPopupModal("Notification from Hardware", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Received the following notification:");
            std::stringstream s;
            s<<"Timestamp: "<<std::fixed<<selected.timestamp<<"\n"<<"Severity: "<<selected.severity<<"\nDescription: "<<const_cast<char*>(selected.message.c_str());
            ImGui::InputTextMultiline("notification", const_cast<char*>(s.str().c_str()),
                selected.message.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll);
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                selected.message = "";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::End();

        ImGui::PopStyleColor();
    }
};

std::vector<std::string> get_device_info(const device& dev)
{
    std::vector<std::string> res;
    for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
    {
        auto info = static_cast<rs2_camera_info>(i);
        if (dev.supports(info))
        {
            auto value = dev.get_camera_info(info);
            res.push_back(value);

        }
    }
    return res;
}



std::string get_device_name(device& dev)
{
    std::string name = dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME);              // retrieve device name

    std::string serial = dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER);   // retrieve device serial number
    std::string module = dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);   // retrieve device serial number

    std::stringstream s;
    s<< std::setw(25)<<std::left<< name <<  " " <<std::setw(10)<<std::left<< module<<" Sn#" << serial;
    return s.str();        // push name and sn to list
}

std::vector<std::string> get_devices_names(const device_list& list)
{
    std::vector<std::string> device_names;

    for (uint32_t i = 0; i < list.size(); i++)
    {
        try
        {
            auto dev = list[i];
            device_names.push_back(get_device_name(dev));        // push name and sn to list
        }
        catch (...)
        {
            device_names.push_back(to_string() << "Unknown Device #" << i);
        }
    }
    return device_names;
}


int find_device_index(const device_list& list ,std::vector<std::string> device_info)
{
    std::vector<std::vector<std::string>> devices_info;

    for (auto l:list)
    {
        devices_info.push_back(get_device_info(l));
    }

    auto it = std::find(devices_info.begin(),devices_info.end(), device_info);
    return std::distance(devices_info.begin(), it);
}

int main(int, char**) try
{
    // activate logging to console
    log_to_console(RS2_LOG_SEVERITY_WARN);

    // Init GUI
    if (!glfwInit())
        exit(1);

    // Create GUI Windows
    auto window = glfwCreateWindow(1280, 720, "librealsense - config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(10, 0, 0);

    // Create RealSense Context
    context ctx;
    auto refresh_device_list = true;

    auto device_index = 0;

    std::vector<std::string> device_names;

    std::vector<const char*>  device_names_chars;

    notifications_model not_model;
    std::string error_message = "";
    notification_model selected_notification;
    // Initialize list with each device name and serial number
    std::string label;

    mouse_info mouse;

    user_data data;
    data.curr_window = window;
    data.mouse = &mouse;


    glfwSetWindowUserPointer(window, &data);

    glfwSetCursorPosCallback(window, [](GLFWwindow * w, double cx, double cy)
    {
        reinterpret_cast<user_data *>(glfwGetWindowUserPointer(w))->mouse->cursor = { (float)cx, (float)cy };
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow * w, int button, int action, int mods)
    {
        auto data = reinterpret_cast<user_data *>(glfwGetWindowUserPointer(w));
        data->mouse->mouse_down = action != GLFW_RELEASE;
    });

    auto last_time_point = std::chrono::high_resolution_clock::now();

    device dev;
    device_model model;
    device_list list;
    device_list new_list;
    std::vector<std::string> active_device_info;
    auto dev_exist = false;
    auto hw_reset_enable = true;

    std::vector<device> devs;
    std::mutex m;

    auto timestamp =  std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

    ctx.set_devices_changed_callback([&](event_information& info)
    {
        timestamp =  std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::lock_guard<std::mutex> lock(m);

        for(auto dev:devs)
        {
            if(info.was_removed(dev))
            {

                not_model.add_notification({get_device_name(dev) + "\ndisconnected",
                                            timestamp,
                                            RS2_LOG_SEVERITY_INFO,
                                            RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR});
            }
        }


        if(info.was_removed(dev))
        {
            dev_exist = false;
        }

        for(auto dev:info.get_new_devices())
        {
            not_model.add_notification({get_device_name(dev) + "\nconnected",
                                        timestamp,
                                        RS2_LOG_SEVERITY_INFO,
                                        RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR});
        }

        refresh_device_list = true;

    });

    // Closing the window
    while (!glfwWindowShouldClose(window))
    {
        {
            std::lock_guard<std::mutex> lock(m);

            if(refresh_device_list)
            {

                refresh_device_list = false;

                try
                {
                    list = ctx.query_devices();
                    
                    device_names = get_devices_names(list);
                    device_names_chars = get_string_pointers(device_names);

                    for(auto dev: devs)
                    {
                        dev = nullptr;
                    }
                    devs.clear();

                    if( !dev_exist)
                    {
                        device_index = 0;
                        dev = nullptr;
                        model.reset();
                       
                        if(list.size()>0)
                        {
                            dev = list[device_index];                  // Access first device
                            model = device_model(dev, error_message);  // Initialize device model
                            active_device_info = get_device_info(dev);
                            dev_exist = true;
                        }
                    }
                    else
                    {
                        device_index = find_device_index(list, active_device_info);
                    }

                    for (auto&& sub : list)
                    {
                        devs.push_back(sub);
                        sub.set_notifications_callback([&](const notification& n)
                        {
                            not_model.add_notification({n.get_description(), n.get_timestamp(), n.get_severity(), n.get_category()});
                        });
                    }
                    
                }
                catch (const error& e)
                {
                    error_message = error_to_string(e);
                }
                catch (const std::exception& e)
                {
                    error_message = e.what();
                }

                hw_reset_enable = true;
            }
        }
        bool update_read_only_options = false;
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(now - last_time_point).count();
        if (duration >= 5000)
        {
            update_read_only_options = true;
            last_time_point = now;
        }

        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        ImGui_ImplGlfw_NewFrame();

        // Flags for pop-up window - no window resize, move or collaps
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        const float panel_size = 320;
        // Set window position and size
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ panel_size, static_cast<float>(h) });

        // *********************
        // Creating window menus
        // *********************
        ImGui::Begin("Control Panel", nullptr, flags);

        rs2_error* e = nullptr;
        label = to_string() << "VERSION: " << api_version_to_string(rs2_get_api_version(&e));
        ImGui::Text("%s", label.c_str());

        if (list.size()==0 )
        {
            ImGui::Text("No device detected.");
        }
        // Device Details Menu - Elaborate details on connected devices
        if (list.size()>0 && ImGui::CollapsingHeader("Device Details", nullptr, true, true))
        {
            // Draw a combo-box with the list of connected devices

            ImGui::PushItemWidth(-1);
            auto new_index = device_index;
            if (ImGui::Combo("", &new_index, device_names_chars.data(),
                static_cast<int>(device_names.size())))
            {

                for (auto&& sub : model.subdevices)
                {
                    if (sub->streaming)
                        sub->stop();
                }

                try
                {
                    dev = list[new_index];
                    device_index = new_index;
                    model = device_model(dev, error_message);
                    active_device_info = get_device_info(dev);
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
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(device_names_chars[new_index]);
            }
            ImGui::PopItemWidth();


            // Show all device details - name, module name, serial number, FW version and location
            for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
            {
                auto info = static_cast<rs2_camera_info>(i);
                if (dev.supports(info))
                {
                    // retrieve info property
                    std::stringstream ss;
                    ss << rs2_camera_info_to_string(info) << ":";
                    auto line = ss.str();
                    ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 0.5f });
                    ImGui::Text("%s", line.c_str());
                    ImGui::PopStyleColor();

                    // retrieve property value
                    ImGui::SameLine();
                    auto value = dev.get_camera_info(info);
                    ImGui::Text("%s", value);

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", value);
                    }
                }
            }
        }

        // Streaming Menu - Allow user to play different streams
        if (list.size()>0 && ImGui::CollapsingHeader("Streaming", nullptr, true, true))
        {
            if (model.subdevices.size() > 1)
            {
                try
                {
                    const float stream_all_button_width = 290;
                    const float stream_all_button_height = 0;

                    auto anything_stream = false;
                    for (auto&& sub : model.subdevices)
                    {
                        if (sub->streaming) anything_stream = true;
                    }
                    if (!anything_stream)
                    {
                        label = to_string() << "Start All";

                        if (ImGui::Button(label.c_str(), { stream_all_button_width, stream_all_button_height }))
                        {
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->is_selected_combination_supported())
                                {
                                    auto profiles = sub->get_selected_profiles();
                                    sub->play(profiles);

                                    for (auto&& profile : profiles)
                                    {
                                        model.streams[profile.stream].dev = sub;
                                    }
                                }
                            }

                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Start streaming from all subdevices");
                        }
                    }
                    else
                    {
                        label = to_string() << "Stop All";

                        if (ImGui::Button(label.c_str(), { stream_all_button_width, stream_all_button_height }))
                        {
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->streaming) sub->stop();
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Stop streaming from all subdevices");
                        }
                    }
                }
                catch(const error& e)
                {
                    error_message = error_to_string(e);
                }
                catch(const std::exception& e)
                {
                    error_message = e.what();
                }
            }

            // Draw menu foreach subdevice with its properties
            for (auto&& sub : model.subdevices)
            {

                label = to_string() << sub->dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, true))
                {
                    sub->draw_stream_selection();

                    try
                    {
                        if (!sub->streaming)
                        {
                            label = to_string() << "Start " << sub->dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);

                            if (sub->is_selected_combination_supported())
                            {
                                if (ImGui::Button(label.c_str()))
                                {
                                    auto profiles = sub->get_selected_profiles();
                                    sub->play(profiles);

                                    for (auto&& profile : profiles)
                                    {
                                        model.streams[profile.stream].dev = sub;
                                    }
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Start streaming data from selected sub-device");
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("%s", label.c_str());
                            }
                        }
                        else
                        {
                            label = to_string() << "Stop " << sub->dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);
                            if (ImGui::Button(label.c_str()))
                            {
                                sub->stop();
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Stop streaming data from selected sub-device");
                            }
                        }
                    }
                    catch(const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch(const std::exception& e)
                    {
                        error_message = e.what();
                    }

                    for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                    {
                        auto opt = static_cast<rs2_option>(i);
                        auto&& metadata = sub->options_metadata[opt];
                        if (update_read_only_options)
                        {
                            metadata.update_supported(error_message);
                            if (metadata.supported && sub->streaming)
                            {
                                metadata.update_read_only(error_message);
                                if (metadata.read_only)
                                {
                                    metadata.update_all(error_message);
                                }
                            }
                        }
                        metadata.draw(error_message);
                    }
                }
                ImGui::Text("\n");
            }
        }

        if (list.size()>0 && ImGui::CollapsingHeader("Hardware Commands", nullptr, true))
        {
            label = to_string() << "Hardware Reset";

            const float hardware_reset_button_width = 290;
            const float hardware_reset_button_height = 0;

            if (ImGui::ButtonEx(label.c_str(), { hardware_reset_button_width, hardware_reset_button_height }, hw_reset_enable?0:ImGuiButtonFlags_Disabled))
            {
                try
                {
                    dev.hardware_reset();
                }
                catch(const error& e)
                {
                    error_message = error_to_string(e);
                }
                catch(const std::exception& e)
                {
                    error_message = e.what();
                }
            }
        }

        ImGui::Text("\n\n\n\n\n\n\n");

        for (auto&& sub : model.subdevices)
        {
            sub->update(error_message);
        }

        if (error_message != "")
            ImGui::OpenPopup("Oops, something went wrong!");
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

        not_model.draw(w, h, selected_notification);

        // Fetch frames from queue
        for (auto&& sub : model.subdevices)
        {
            for (auto& queue : sub->queues)
            {
                try
                {
                    frame f;
                    if (queue->poll_for_frame(&f))
                    {
                        model.upload_frame(std::move(f));
                    }
                }
                catch(const error& e)
                {
                    error_message = error_to_string(e);
                     sub->stop();
                }
                catch(const std::exception& e)
                {
                    error_message = e.what();
                    sub->stop();
                }
            }

        }

        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(window, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        auto layout = model.calc_layout(panel_size, 0.f, w - panel_size, (float)h);

        for (auto &&kvp : layout)
        {
            auto&& view_rect = kvp.second;
            auto stream = kvp.first;
            auto&& stream_size = model.streams[stream].size;
            auto stream_rect = view_rect.adjust_ratio(stream_size);

            model.streams[stream].show_frame(stream_rect, mouse, error_message);

            flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y });
            ImGui::SetNextWindowSize({ stream_rect.w, stream_rect.h });
            label = to_string() << "Stream of " << rs2_stream_to_string(stream);
            ImGui::Begin(label.c_str(), nullptr, flags);

            label = to_string() << rs2_stream_to_string(stream) << " "
                << stream_size.x << "x" << stream_size.y << ", "
                << rs2_format_to_string(model.streams[stream].format) << ", "
                << "Frame# " << model.streams[stream].frame_number << ", "
                << "FPS:";

            ImGui::Text("%s", label.c_str());
            ImGui::SameLine();

            label = to_string() << std::setprecision(2) << std::fixed << model.streams[stream].fps.get_fps();
            ImGui::Text("%s", label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("FPS is calculated based on timestamps and not viewer time");
            }

            ImGui::SameLine((int)ImGui::GetWindowWidth() - 30);

            if (!layout.empty() && !model.fullscreen)
            {
                if (ImGui::Button("[+]", { 26, 20 }))
                {
                    model.fullscreen = true;
                    model.selected_stream = stream;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Maximize stream to full-screen");
                }
            }
            else if (model.fullscreen)
            {
                if (ImGui::Button("[-]", { 26, 20 }))
                {
                    model.fullscreen = false;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Minimize stream to tile-view");
                }
            }

            if (!layout.empty())
            {
                if (model.streams[stream].roi_supported )
                {
                    ImGui::SameLine((int)ImGui::GetWindowWidth() - 160);
                    ImGui::Checkbox("[ROI]", &model.streams[stream].roi_checked);

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Auto Exposure Region-of-Interest Selection");
                }
                else
                    model.streams[stream].roi_checked = false;
            }

            // Control metadata overlay widget
            ImGui::SameLine((int)ImGui::GetWindowWidth() - 90); // metadata GUI hint
            if (!layout.empty())
            {
                ImGui::Checkbox("[MD]", &model.streams[stream].metadata_displayed);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Show per-frame metadata");
            }

            label = to_string() << "Timestamp: " << std::fixed << std::setprecision(3) << model.streams[stream].timestamp
                                << ", Domain:";
            ImGui::Text("%s", label.c_str());

            ImGui::SameLine();
            auto domain = model.streams[stream].timestamp_domain;
            label = to_string() << rs2_timestamp_domain_to_string(domain);

            if (domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hardware Timestamp unavailable! This is often an indication of inproperly applied Kernel patch.\nPlease refer to installation.md for mode information");
                }
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Specifies the clock-domain for the timestamp (hardware-clock / system-time)");
                }
            }

            ImGui::End();
            ImGui::PopStyleColor();

            if (stream_rect.contains(mouse.cursor))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
                ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + stream_rect.h - 25 });
                ImGui::SetNextWindowSize({ stream_rect.w, 25 });
                label = to_string() << "Footer for stream of " << rs2_stream_to_string(stream);
                ImGui::Begin(label.c_str(), nullptr, flags);

                std::stringstream ss;
                auto x = ((mouse.cursor.x - stream_rect.x) / stream_rect.w) * stream_size.x;
                auto y = ((mouse.cursor.y - stream_rect.y) / stream_rect.h) * stream_size.y;
                ss << std::fixed << std::setprecision(0) << x << ", " << y;

                float val;
                if (model.streams[stream].texture->try_pick(x, y, &val))
                {
                    ss << ", *p: 0x" << std::hex << val;
                    if (stream == RS2_STREAM_DEPTH && val > 0)
                    {
                        auto meters = (val * model.streams[stream].dev->depth_units);
                        ss << std::dec << ", ~"
                           << std::setprecision(2) << meters << " meters";
                    }
                }

                label = ss.str();
                ImGui::Text("%s", label.c_str());

                ImGui::End();
                ImGui::PopStyleColor();
            }
        }

        // Metadata overlay windows shall be drawn after textures to preserve z-buffer functionality
        for (auto &&kvp : layout)
        {
            if (model.streams[kvp.first].metadata_displayed)
                model.streams[kvp.first].show_metadata(mouse);
        }

        ImGui::Render();


        glfwSwapBuffers(window);
    }

    // Stop all subdevices
    for (auto&& sub : model.subdevices)
    {
        if (sub->streaming)
            sub->stop();
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return EXIT_SUCCESS;
}
catch (const error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
