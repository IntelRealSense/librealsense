// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <regex>
#include <thread>
#include <algorithm>

#include <librealsense2/rs_advanced_mode.hpp>
#include <librealsense2/rsutil.h>

#include "model-views.h"

#include <imgui_internal.h>

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#include <noc_file_dialog.h>

#define ARCBALL_CAMERA_IMPLEMENTATION
#include <arcball_camera.h>

using namespace rs400;

ImVec4 flip(const ImVec4& c)
{
    return{ c.y, c.x, c.z, c.w };
}

ImVec4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a, bool consistent_color)
{
    auto res = ImVec4(r / (float)255, g / (float)255, b / (float)255, a / (float)255);
#ifdef FLIP_COLOR_SCHEME
    if (!consistent_color) return flip(res);
#endif
    return res;
}

ImVec4 operator+(const ImVec4& c, float v)
{
    return ImVec4(
        std::max(0.f, std::min(1.f, c.x + v)),
        std::max(0.f, std::min(1.f, c.y + v)),
        std::max(0.f, std::min(1.f, c.z + v)),
        std::max(0.f, std::min(1.f, c.w))
    );
}


namespace rs2
{
    void imgui_easy_theming(ImFont*& font_14, ImFont*& font_18)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        ImGuiIO& io = ImGui::GetIO();

        const auto OVERSAMPLE = 1;

        static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.

                                                                     // Load 14px size fonts
        {
            ImFontConfig config_words;
            config_words.OversampleV = OVERSAMPLE;
            config_words.OversampleH = OVERSAMPLE;
            font_14 = io.Fonts->AddFontFromMemoryCompressedTTF(karla_regular_compressed_data, karla_regular_compressed_size, 16.f);

            ImFontConfig config_glyphs;
            config_glyphs.MergeMode = true;
            config_glyphs.OversampleV = OVERSAMPLE;
            config_glyphs.OversampleH = OVERSAMPLE;
            font_14 = io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data,
                font_awesome_compressed_size, 14.f, &config_glyphs, icons_ranges);
        }

        // Load 18px size fonts
        {
            ImFontConfig config_words;
            config_words.OversampleV = OVERSAMPLE;
            config_words.OversampleH = OVERSAMPLE;
            font_18 = io.Fonts->AddFontFromMemoryCompressedTTF(karla_regular_compressed_data, karla_regular_compressed_size, 21.f, &config_words);

            ImFontConfig config_glyphs;
            config_glyphs.MergeMode = true;
            config_glyphs.OversampleV = OVERSAMPLE;
            config_glyphs.OversampleH = OVERSAMPLE;
            font_18 = io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data,
                font_awesome_compressed_size, 20.f, &config_glyphs, icons_ranges);
        }

        style.WindowRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;

        style.Colors[ImGuiCol_WindowBg] = dark_window_background;
        style.Colors[ImGuiCol_Border] = black;
        style.Colors[ImGuiCol_BorderShadow] = transparent;
        style.Colors[ImGuiCol_FrameBg] = dark_window_background;
        style.Colors[ImGuiCol_ScrollbarBg] = scrollbar_bg;
        style.Colors[ImGuiCol_ScrollbarGrab] = scrollbar_grab;
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = scrollbar_grab + 0.1f;
        style.Colors[ImGuiCol_ScrollbarGrabActive] = scrollbar_grab + (-0.1f);
        style.Colors[ImGuiCol_ComboBg] = dark_window_background;
        style.Colors[ImGuiCol_CheckMark] = regular_blue;
        style.Colors[ImGuiCol_SliderGrab] = regular_blue;
        style.Colors[ImGuiCol_SliderGrabActive] = regular_blue;
        style.Colors[ImGuiCol_Button] = button_color;
        style.Colors[ImGuiCol_ButtonHovered] = button_color + 0.1f;
        style.Colors[ImGuiCol_ButtonActive] = button_color + (-0.1f);
        style.Colors[ImGuiCol_Header] = header_color;
        style.Colors[ImGuiCol_HeaderActive] = header_color + (-0.1f);
        style.Colors[ImGuiCol_HeaderHovered] = header_color + 0.1f;
        style.Colors[ImGuiCol_TitleBg] = title_color;
        style.Colors[ImGuiCol_TitleBgCollapsed] = title_color;
        style.Colors[ImGuiCol_TitleBgActive] = header_color;
    }

    // Helper function to get window rect from GLFW
    rect get_window_rect(GLFWwindow* window)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        int xpos, ypos;
        glfwGetWindowPos(window, &xpos, &ypos);

        return{ (float)xpos, (float)ypos,
            (float)width, (float)height };
    }

    // Helper function to get monitor rect from GLFW
    rect get_monitor_rect(GLFWmonitor* monitor)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        int xpos, ypos;
        glfwGetMonitorPos(monitor, &xpos, &ypos);

        return{ (float)xpos, (float)ypos,
            (float)mode->width, (float)mode->height };
    }

    // Select appropriate scale factor based on the display
    // that most of the application is presented on
    int pick_scale_factor(GLFWwindow* window)
    {
        auto window_rect = get_window_rect(window);
        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        if (count == 0) return 1; // Not sure if possible, but better be safe

                                    // Find the monitor that covers most of the application pixels:
        GLFWmonitor* best = monitors[0];
        float best_area = 0.f;
        for (int i = 0; i < count; i++)
        {
            auto int_area = window_rect.intersection(
                get_monitor_rect(monitors[i])).area();
            if (int_area >= best_area)
            {
                best_area = int_area;
                best = monitors[i];
            }
        }

        int widthMM = 0;
        int heightMM = 0;
        glfwGetMonitorPhysicalSize(best, &widthMM, &heightMM);

        // This indicates that the monitor dimentions are unknown
        if (widthMM * heightMM == 0) return 1;

        // The actual calculation is somewhat arbitrary, but we are going for
        // about 1cm buttons, regardless of resultion
        // We discourage fractional scale factors
        float how_many_pixels_in_mm =
            get_monitor_rect(best).area() / (widthMM * heightMM);
        float scale = sqrt(how_many_pixels_in_mm) / 5.f;
        if (scale < 1.f) return 1;
        return (int)(floor(scale));
    }

    std::tuple<uint8_t, uint8_t, uint8_t> get_texcolor(video_frame texture, texture_coordinate texcoords)
    {
        const int w = texture.get_width(), h = texture.get_height();
        int x = std::min(std::max(int(texcoords.u*w + .5f), 0), w - 1);
        int y = std::min(std::max(int(texcoords.v*h + .5f), 0), h - 1);
        int idx = x*texture.get_bytes_per_pixel() + y*texture.get_stride_in_bytes();
        const auto texture_data = reinterpret_cast<const uint8_t*>(texture.get_data());
        return std::tuple<uint8_t, uint8_t, uint8_t>(
            texture_data[idx], texture_data[idx + 1], texture_data[idx + 2]);
    }

    void export_to_ply(const std::string& fname, notifications_model& ns, frameset frames, video_frame texture)
    {
        std::thread([&ns, frames, texture, fname]() mutable {

            points p;

            for (auto&& f : frames)
            {
                if (p = f.as<points>())
                {
                    break;
                }
            }

            if (p)
            {
                p.export_to_ply(fname, texture);
                ns.add_notification({ to_string() << "Finished saving 3D view " << (texture ? "to " : "without texture to ") << fname,
                    std::chrono::duration_cast<std::chrono::duration<double,std::micro>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
                    RS2_LOG_SEVERITY_INFO,
                    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
        }).detach();
    }

    const char* file_dialog_open(file_dialog_mode flags, const char* filters, const char* default_path, const char* default_name)
    {
        return noc_file_dialog_open(flags, filters, default_path, default_name);
    }

    int save_to_png(const char* filename,
        size_t pixel_width, size_t pixels_height, size_t bytes_per_pixel,
        const void* raster_data, size_t stride_bytes)
    {
        return stbi_write_png(filename, (int)pixel_width, (int)pixels_height, bytes_per_pixel, raster_data, stride_bytes);
    }

    std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec)
    {
        std::vector<const char*> res;
        for (auto&& s : vec) res.push_back(s.c_str());
        return res;
    }

    bool option_model::draw(std::string& error_message, notifications_model& model)
    {
        auto res = false;
        if (supported)
        {
            auto desc = endpoint->get_option_description(opt);

            if (is_checkbox())
            {
                auto bool_value = value > 0.0f;
                if (ImGui::Checkbox(label.c_str(), &bool_value))
                {
                    res = true;
                    value = bool_value ? 1.0f : 0.0f;
                    try
                    {
                        model.add_log(to_string() << "Setting " << opt << " to " 
                            << value << " (" << (bool_value? "ON" : "OFF") << ")");
                        endpoint->set_option(opt, value);
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
                if (!is_enum())
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
                            ImVec2 vec{ 0, 14 };
                            std::string text = (value == (int)value) ? std::to_string((int)value) : std::to_string(value);
                            if (range.min != range.max)
                            {
                                ImGui::ProgressBar((value / (range.max - range.min)), vec, text.c_str());
                            }
                            else //constant value options
                            {
                                auto c = ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_FrameBg));
                                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, c);
                                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c);
                                float dummy = (int)value;
                                ImGui::DragFloat(id.c_str(), &dummy, 1, 0, 0, text.c_str());
                                ImGui::PopStyleColor(2);
                            }
                        }
                        else if (is_all_integers())
                        {
                            auto int_value = static_cast<int>(value);
                            if (ImGui::SliderIntWithSteps(id.c_str(), &int_value,
                                static_cast<int>(range.min),
                                static_cast<int>(range.max),
                                static_cast<int>(range.step)))
                            {
                                // TODO: Round to step?
                                value = static_cast<float>(int_value);
                                model.add_log(to_string() << "Setting " << opt << " to " << value);
                                endpoint->set_option(opt, value);
                                *invalidate_flag = true;
                                res = true;
                            }
                        }
                        else
                        {
                            if (ImGui::SliderFloat(id.c_str(), &value,
                                range.min, range.max, "%.4f"))
                            {
                                model.add_log(to_string() << "Setting " << opt << " to " << value);
                                endpoint->set_option(opt, value);
                                *invalidate_flag = true;
                                res = true;
                            }
                        }
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }
                else
                {
                    std::string txt = to_string() << rs2_option_to_string(opt) << ":";
                    auto col_id = id + "columns";
                    //ImGui::Columns(2, col_id.c_str(), false);
                    //ImGui::SetColumnOffset(1, 120);



                    ImGui::Text("%s", txt.c_str());
                    if (ImGui::IsItemHovered() && desc)
                    {
                        ImGui::SetTooltip("%s", desc);
                    }

                    ImGui::SameLine(); ImGui::SetCursorPosX(135);

                    ImGui::PushItemWidth(-1);

                    std::vector<const char*> labels;
                    auto selected = 0, counter = 0;
                    for (auto i = range.min; i <= range.max; i += range.step, counter++)
                    {
                        if (std::fabs(i - value) < 0.001f) selected = counter;
                        labels.push_back(endpoint->get_option_value_description(opt, i));
                    }
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });

                    try
                    {
                        if (ImGui::Combo(id.c_str(), &selected, labels.data(),
                            static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * selected;
                            model.add_log(to_string() << "Setting " << opt << " to " 
                                << value << " (" << labels[selected] << ")");
                            endpoint->set_option(opt, value);
                            *invalidate_flag = true;
                            res = true;
                        }
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }

                    ImGui::PopStyleColor();

                    ImGui::PopItemWidth();

                    ImGui::NextColumn();
                    ImGui::Columns(1);
                }


            }

            if (!read_only && opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE && dev->auto_exposure_enabled && dev->streaming)
            {
                ImGui::SameLine(0, 10);
                std::string button_label = label;
                auto index = label.find_last_of('#');
                if (index != std::string::npos)
                {
                    button_label = label.substr(index + 1);
                }

                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1.f,1.f,1.f,1.f });
                if (!dev->roi_checked)
                {
                    std::string caption = to_string() << "Set ROI##" << button_label;
                    if (ImGui::Button(caption.c_str(), { 55, 0 }))
                    {
                        dev->roi_checked = true;
                    }
                }
                else
                {
                    std::string caption = to_string() << "Cancel##" << button_label;
                    if (ImGui::Button(caption.c_str(), { 55, 0 }))
                    {
                        dev->roi_checked = false;
                    }
                }
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Select custom region of interest for the auto-exposure algorithm\nClick the button, then draw a rect on the frame");
            }
        }

        return res;
    }

    void option_model::update_supported(std::string& error_message)
    {
        try
        {
            supported = endpoint->supports(opt);
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }

    void option_model::update_read_only_status(std::string& error_message)
    {
        try
        {
            read_only = endpoint->is_option_read_only(opt);
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }

    void option_model::update_all_fields(std::string& error_message, notifications_model& model)
    {
        try
        {
            if (supported = endpoint->supports(opt))
            {
                value = endpoint->get_option(opt);
                range = endpoint->get_option_range(opt);
                read_only = endpoint->is_option_read_only(opt);
            }
        }
        catch (const error& e)
        {
            if (read_only) {
                auto timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
                model.add_notification({ to_string() << "Could not refresh read-only option " << rs2_option_to_string(opt) << ": " << e.what(),
                    timestamp,
                    RS2_LOG_SEVERITY_WARN,
                    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
            else
                error_message = error_to_string(e);
        }
    }

    bool option_model::is_all_integers() const
    {
        return is_integer(range.min) && is_integer(range.max) &&
            is_integer(range.def) && is_integer(range.step);
    }

    bool option_model::is_enum() const
    {
        if (range.step < 0.001f) return false;

        for (auto i = range.min; i <= range.max; i += range.step)
        {
            if (endpoint->get_option_value_description(opt, i) == nullptr)
                return false;
        }
        return true;
    }

    bool option_model::is_checkbox() const
    {
        return range.max == 1.0f &&
            range.min == 0.0f &&
            range.step == 1.0f;
    }

    void subdevice_model::populate_options(std::map<int, option_model>& opt_container,
        const device& dev,
        const sensor& s,
        bool* options_invalidated,
        subdevice_model* model,
        std::shared_ptr<options> options,
        std::string& error_message)
    {
        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            option_model metadata;
            auto opt = static_cast<rs2_option>(i);

            std::stringstream ss;
            ss << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
                << "/" << s.get_info(RS2_CAMERA_INFO_NAME)
                << "/" << rs2_option_to_string(opt);
            metadata.id = ss.str();
            metadata.opt = opt;
            metadata.endpoint = options;
            metadata.label = rs2_option_to_string(opt) + std::string("##") + ss.str();
            metadata.invalidate_flag = options_invalidated;
            metadata.dev = model;

            metadata.supported = options->supports(opt);
            if (metadata.supported)
            {
                try
                {
                    metadata.range = options->get_option_range(opt);
                    metadata.read_only = options->is_option_read_only(opt);
                    if (!metadata.read_only)
                        metadata.value = options->get_option(opt);
                }
                catch (const error& e)
                {
                    metadata.range = { 0, 1, 0, 0 };
                    metadata.value = 0;
                    error_message = error_to_string(e);
                }
            }
            opt_container[opt] = metadata;
        }
    }


    processing_block_model::processing_block_model(subdevice_model* owner,
        const std::string& name,
        std::shared_ptr<options> block,
        std::function<rs2::frame(rs2::frame)> invoker,
        std::string& error_message)
        : _name(name), _block(block), _invoker(invoker)
    {
        subdevice_model::populate_options(options_metadata,
            owner->dev, *owner->s, &owner->options_invalidated, owner, block, error_message);
    }

    subdevice_model::subdevice_model(device& dev,
        std::shared_ptr<sensor> s, std::string& error_message)
        : s(s), dev(dev), ui(), last_valid_ui(),
        streaming(false), _pause(false),
        depth_colorizer(std::make_shared<rs2::colorizer>()),
        decimation_filter(),
        spatial_filter(),
        temporal_filter(),
        depth_to_disparity(),
        disparity_to_depth()
    {
        try
        {
            if (s->supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
                auto_exposure_enabled = s->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) > 0;
        }
        catch (...)
        {

        }

        try
        {
            if (s->supports(RS2_OPTION_DEPTH_UNITS))
                depth_units = s->get_option(RS2_OPTION_DEPTH_UNITS);
        }
        catch (...){}

        try
        {
            if (s->supports(RS2_OPTION_STEREO_BASELINE))
                stereo_baseline = s->get_option(RS2_OPTION_STEREO_BASELINE);
        }
        catch (...) {}

        if (s->is<depth_sensor>())
        {
            auto colorizer = std::make_shared<processing_block_model>(
                this, "Depth Visualization", depth_colorizer,
                [=](rs2::frame f) { return depth_colorizer->colorize(f); }, error_message);
            const_effects.push_back(colorizer);

            auto decimate = std::make_shared<rs2::decimation_filter>();
            decimation_filter = std::make_shared<processing_block_model>(
                this, "Decimation Filter", decimate,
                [=](rs2::frame f) { return decimate->proccess(f); },
                error_message);
            decimation_filter->enabled = true;
            post_processing.push_back(decimation_filter);

            auto depth_2_disparity = std::make_shared<rs2::disparity_transform>();
            depth_to_disparity = std::make_shared<processing_block_model>(
                this, "Depth->Disparity", depth_2_disparity,
                [=](rs2::frame f) { return depth_2_disparity->proccess(f); }, error_message);
            if (s->is<depth_stereo_sensor>())
            {
                depth_to_disparity->enabled = true;
                post_processing.push_back(depth_to_disparity);
            }

            auto spatial = std::make_shared<rs2::spatial_filter>();
            spatial_filter = std::make_shared<processing_block_model>(
                this, "Spatial Filter", spatial,
                [=](rs2::frame f) { return spatial->proccess(f); },
                error_message);
            spatial_filter->enabled = true;
            post_processing.push_back(spatial_filter);

            auto temporal = std::make_shared<rs2::temporal_filter>();
            temporal_filter = std::make_shared<processing_block_model>(
                this, "Temporal Filter", temporal,
                [=](rs2::frame f) { return temporal->proccess(f); }, error_message);
            temporal_filter->enabled = true;
            post_processing.push_back(temporal_filter);

            auto disparity_2_depth = std::make_shared<rs2::disparity_transform>(false);
            disparity_to_depth = std::make_shared<processing_block_model>(
                this, "Disparity->Depth", disparity_2_depth,
                [=](rs2::frame f) { return disparity_2_depth->proccess(f); }, error_message);
            disparity_to_depth->enabled = s->is<depth_stereo_sensor>();
            if (s->is<depth_stereo_sensor>())
            {
                disparity_to_depth->enabled = true;
                // the block will be internally available, but removed from UI
                //post_processing.push_back(disparity_to_depth);
            }
        }

        populate_options(options_metadata, dev, *s, &options_invalidated, this, s, error_message);

        try
        {
            auto uvc_profiles = s->get_stream_profiles();
            reverse(begin(uvc_profiles), end(uvc_profiles));
            for (auto&& profile : uvc_profiles)
            {
                std::stringstream res;
                if (auto vid_prof = profile.as<video_stream_profile>())
                {
                    res << vid_prof.width() << " x " << vid_prof.height();
                    push_back_if_not_exists(res_values, std::pair<int, int>(vid_prof.width(), vid_prof.height()));
                    push_back_if_not_exists(resolutions, res.str());
                }

                std::stringstream fps;
                fps << profile.fps();
                push_back_if_not_exists(fps_values_per_stream[profile.unique_id()], profile.fps());
                push_back_if_not_exists(shared_fps_values, profile.fps());
                push_back_if_not_exists(fpses_per_stream[profile.unique_id()], fps.str());
                push_back_if_not_exists(shared_fpses, fps.str());
                stream_display_names[profile.unique_id()] = profile.stream_name();

                std::string format = rs2_format_to_string(profile.format());

                push_back_if_not_exists(formats[profile.unique_id()], format);
                push_back_if_not_exists(format_values[profile.unique_id()], profile.format());

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
                    stream_enabled[profile.unique_id()] = true;
                }

                profiles.push_back(profile);
            }

            for (auto&& fps_list : fps_values_per_stream)
            {
                sort_together(fps_list.second, fpses_per_stream[fps_list.first]);
            }
            sort_together(shared_fps_values, shared_fpses);
            sort_together(res_values, resolutions);

            show_single_fps_list = is_there_common_fps();

            // set default selections
            int selection_index;

            if (!show_single_fps_list)
            {
                for (auto fps_array : fps_values_per_stream)
                {
                    if (get_default_selection_index(fps_array.second, 30, &selection_index))
                    {
                        ui.selected_fps_id[fps_array.first] = selection_index;
                        break;
                    }
                }
            }
            else
            {
                if (get_default_selection_index(shared_fps_values, 30, &selection_index))
                    ui.selected_shared_fps_id = selection_index;
            }

            for (auto format_array : format_values)
            {
                for (auto format : { RS2_FORMAT_RGB8,
                                     RS2_FORMAT_Z16,
                                     RS2_FORMAT_Y8,
                                     RS2_FORMAT_MOTION_XYZ32F })
                {
                    if (get_default_selection_index(format_array.second, format, &selection_index))
                    {
                        ui.selected_format_id[format_array.first] = selection_index;
                        break;
                    }
                }
            }

            // For Realtec sensors
            auto rgb_rotation_btn = (val_in_range(std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID)),
            { std::string("0AD3") ,std::string("0B07") }) &&
                val_in_range(std::string(s->get_info(RS2_CAMERA_INFO_NAME)), { std::string("RGB Camera") }));
            // Limit Realtec sensor default
            auto constrain = (rgb_rotation_btn) ? std::make_pair(640, 480) : std::make_pair(0, 0);
            get_default_selection_index(res_values, constrain, &selection_index);
            ui.selected_res_id = selection_index;

            while (ui.selected_res_id >= 0 && !is_selected_combination_supported()) ui.selected_res_id--;
            last_valid_ui = ui;
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }


    bool subdevice_model::is_there_common_fps()
    {
        std::vector<int> first_fps_group;
        auto group_index = 0;
        for (; group_index < fps_values_per_stream.size(); ++group_index)
        {
            if (!fps_values_per_stream[(rs2_stream)group_index].empty())
            {
                first_fps_group = fps_values_per_stream[(rs2_stream)group_index];
                break;
            }
        }

        for (int i = group_index + 1; i < fps_values_per_stream.size(); ++i)
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

    bool subdevice_model::draw_stream_selection()
    {
        bool res = false;

        std::string label = to_string() << "Stream Selection Columns##" << dev.get_info(RS2_CAMERA_INFO_NAME)
            << s->get_info(RS2_CAMERA_INFO_NAME);

        auto streaming_tooltip = [&]() {
            if (streaming && ImGui::IsItemHovered())
                ImGui::SetTooltip("Can't modify while streaming");
        };

        //ImGui::Columns(2, label.c_str(), false);
        //ImGui::SetColumnOffset(1, 135);
        auto col0 = ImGui::GetCursorPosX();
        auto col1 = 145.f;

        // Draw combo-box with all resolution options for this device
        auto res_chars = get_string_pointers(resolutions);
        ImGui::Text("Resolution:");
        streaming_tooltip();
        ImGui::SameLine(); ImGui::SetCursorPosX(col1);

        label = to_string() << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
            << s->get_info(RS2_CAMERA_INFO_NAME) << " resolution";
        if (streaming)
        {
            ImGui::Text("%s", res_chars[ui.selected_res_id]);
            streaming_tooltip();
        }
        else
        {
            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
            if (ImGui::Combo(label.c_str(), &ui.selected_res_id, res_chars.data(),
                static_cast<int>(res_chars.size())))
            {
                res = true;
            }
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();
        }
        ImGui::SetCursorPosX(col0);

        if (draw_fps_selector)
        {
            // FPS
            if (show_single_fps_list)
            {
                auto fps_chars = get_string_pointers(shared_fpses);
                ImGui::Text("Frame Rate (FPS):");
                streaming_tooltip();
                ImGui::SameLine(); ImGui::SetCursorPosX(col1);

                label = to_string() << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
                    << s->get_info(RS2_CAMERA_INFO_NAME) << " fps";

                if (streaming)
                {
                    ImGui::Text("%s", fps_chars[ui.selected_shared_fps_id]);
                    streaming_tooltip();
                }
                else
                {
                    ImGui::PushItemWidth(-1);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
                    if (ImGui::Combo(label.c_str(), &ui.selected_shared_fps_id, fps_chars.data(),
                        static_cast<int>(fps_chars.size())))
                    {
                        res = true;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopItemWidth();
                }

                ImGui::SetCursorPosX(col0);
            }
        }

        if (draw_streams_selector)
        {
            if (!streaming)
            {
                ImGui::Text("Available Streams:");
            }

            // Draw combo-box with all format options for current device
            for (auto&& f : formats)
            {
                // Format
                if (f.second.size() == 0)
                    continue;

                auto formats_chars = get_string_pointers(f.second);
                if (!streaming || (streaming && stream_enabled[f.first]))
                {
                    if (streaming)
                    {
                        label = to_string() << stream_display_names[f.first] << (show_single_fps_list ? "" : " stream:");
                        ImGui::Text("%s", label.c_str());
                        streaming_tooltip();
                    }
                    else
                    {
                        label = to_string() << stream_display_names[f.first] << "##" << f.first;
                        ImGui::Checkbox(label.c_str(), &stream_enabled[f.first]);
                    }
                }

                if (stream_enabled[f.first])
                {
                    ImGui::SameLine(); ImGui::SetCursorPosX(col1);

                    //if (show_single_fps_list) ImGui::SameLine();

                    label = to_string() << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
                        << s->get_info(RS2_CAMERA_INFO_NAME)
                        << " " << f.first << " format";

                    if (!show_single_fps_list)
                    {
                        ImGui::Text("Format:");
                        streaming_tooltip();
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);
                    }

                    if (streaming)
                    {
                        ImGui::Text("%s", formats_chars[ui.selected_format_id[f.first]]);
                        streaming_tooltip();
                    }
                    else
                    {
                        ImGui::PushItemWidth(-1);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
                        ImGui::Combo(label.c_str(), &ui.selected_format_id[f.first], formats_chars.data(),
                            static_cast<int>(formats_chars.size()));
                        ImGui::PopStyleColor();
                        ImGui::PopItemWidth();
                    }
                    ImGui::SetCursorPosX(col0);
                    // FPS
                    // Draw combo-box with all FPS options for this device
                    if (!show_single_fps_list && !fpses_per_stream[f.first].empty() && stream_enabled[f.first])
                    {
                        auto fps_chars = get_string_pointers(fpses_per_stream[f.first]);
                        ImGui::Text("Frame Rate (FPS):");
                        streaming_tooltip();
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);

                        label = to_string() << s->get_info(RS2_CAMERA_INFO_NAME)
                            << s->get_info(RS2_CAMERA_INFO_NAME)
                            << f.first << " fps";

                        if (streaming)
                        {
                            ImGui::Text("%s", fps_chars[ui.selected_fps_id[f.first]]);
                            streaming_tooltip();
                        }
                        else
                        {
                            ImGui::PushItemWidth(-1);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
                            ImGui::Combo(label.c_str(), &ui.selected_fps_id[f.first], fps_chars.data(),
                                static_cast<int>(fps_chars.size()));
                            ImGui::PopStyleColor();
                            ImGui::PopItemWidth();
                        }
                        ImGui::SetCursorPosX(col0);
                    }
                }
                else
                {
                    //ImGui::NextColumn();
                }

                //if (streaming && rgb_rotation_btn && ImGui::Button("Flip Stream Orientation", ImVec2(160, 20)))
                //{
                //    rotate_rgb_image(dev, res_values[selected_res_id].first);
                //    if (ImGui::IsItemHovered())
                //        ImGui::SetTooltip("Rotate Sensor 180 deg");
                //}
            }
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        return res;
    }

    bool subdevice_model::is_selected_combination_supported()
    {
        std::vector<stream_profile> results;

        for (auto&& f : formats)
        {
            auto stream = f.first;
            if (stream_enabled[stream] && res_values.size() > 0)
            {
                auto width = res_values[ui.selected_res_id].first;
                auto height = res_values[ui.selected_res_id].second;

                auto fps = 0;
                if (show_single_fps_list)
                    fps = shared_fps_values[ui.selected_shared_fps_id];
                else
                    fps = fps_values_per_stream[stream][ui.selected_fps_id[stream]];

                auto format = format_values[stream][ui.selected_format_id[stream]];

                for (auto&& p : profiles)
                {
                    if (auto vid_prof = p.as<video_stream_profile>())
                    {
                        if (vid_prof.width() == width &&
                            vid_prof.height() == height &&
                            p.unique_id() == stream &&
                            p.fps() == fps &&
                            p.format() == format)
                            results.push_back(p);
                    }
                    else
                    {
                        if (p.fps() == fps &&
                            p.unique_id() == stream &&
                            p.format() == format)
                            results.push_back(p);
                    }
                }
            }
        }

        return results.size() > 0;
    }

    std::vector<stream_profile> subdevice_model::get_selected_profiles()
    {
        std::vector<stream_profile> results;

        std::stringstream error_message;
        error_message << "The profile ";

        for (auto&& f : formats)
        {
            auto stream = f.first;
            if (stream_enabled[stream])
            {
                auto width = res_values[ui.selected_res_id].first;
                auto height = res_values[ui.selected_res_id].second;
                auto format = format_values[stream][ui.selected_format_id[stream]];

                auto fps = 0;
                if (show_single_fps_list)
                    fps = shared_fps_values[ui.selected_shared_fps_id];
                else
                    fps = fps_values_per_stream[stream][ui.selected_fps_id[stream]];

                error_message << "\n{" << stream_display_names[stream] << ","
                    << width << "x" << height << " at " << fps << "Hz, "
                    << rs2_format_to_string(format) << "} ";

                for (auto&& p : profiles)
                {
                    if (auto vid_prof = p.as<video_stream_profile>())
                    {
                        if (vid_prof.width() == width &&
                            vid_prof.height() == height &&
                            p.unique_id() == stream &&
                            p.fps() == fps &&
                            p.format() == format)
                            results.push_back(p);
                    }
                    else
                    {
                        if (p.fps() == fps &&
                            p.unique_id() == stream &&
                            p.format() == format)
                            results.push_back(p);
                    }
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

    void subdevice_model::stop(viewer_model& viewer)
    {
        viewer.not_model.add_log("Stopping streaming");

        streaming = false;
        _pause = false;

        s->stop();

        queues.foreach([&](frame_queue& q)
        {
            frame f;
            while (q.poll_for_frame(&f));
        });

        s->close();
    }

    bool subdevice_model::is_paused() const
    {
        return _pause.load();
    }

    void subdevice_model::pause()
    {
        _pause = true;
    }

    void subdevice_model::resume()
    {
        _pause = false;
    }

    void subdevice_model::play(const std::vector<stream_profile>& profiles, viewer_model& viewer)
    {
        std::stringstream ss;
        ss << "Starting streaming of ";
        for (int i = 0; i < profiles.size(); i++)
        {
            ss << profiles[i].stream_type();
            if (i < profiles.size() - 1) ss << ", ";
        }
        ss << "...";
        viewer.not_model.add_log(ss.str());

        s->open(profiles);

        try {
            s->start([&](frame f)
            {
                if (viewer.synchronization_enable && (!viewer.is_3d_view || viewer.is_3d_depth_source(f) || viewer.is_3d_texture_source(f)))
                {
                    viewer.s(f);
                }
                else
                {
                    auto id = f.get_profile().unique_id();
                    viewer.ppf.frames_queue[id].enqueue(f);
                }
            });
            }

        catch (...)
        {
            s->close();
            throw;
        }

        streaming = true;
    }

    void subdevice_model::update(std::string& error_message, notifications_model& notifications)
    {
        if (options_invalidated)
        {
            next_option = 0;
            options_invalidated = false;
        }
        if (next_option < RS2_OPTION_COUNT)
        {
            auto& opt_md = options_metadata[static_cast<rs2_option>(next_option)];
            opt_md.update_all_fields(error_message, notifications);

            if (next_option == RS2_OPTION_ENABLE_AUTO_EXPOSURE)
            {
                auto old_ae_enabled = auto_exposure_enabled;
                auto_exposure_enabled = opt_md.value > 0;

                if (!old_ae_enabled && auto_exposure_enabled)
                {
                    try
                    {
                        if (s->is<roi_sensor>())
                        {
                            auto r = s->as<roi_sensor>().get_region_of_interest();
                            roi_rect.x = static_cast<float>(r.min_x);
                            roi_rect.y = static_cast<float>(r.min_y);
                            roi_rect.w = static_cast<float>(r.max_x - r.min_x);
                            roi_rect.h = static_cast<float>(r.max_y - r.min_y);
                        }
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

            if (next_option == RS2_OPTION_STEREO_BASELINE)
                opt_md.dev->stereo_baseline = opt_md.value;

            next_option++;
        }
    }

    void subdevice_model::draw_options(const std::vector<rs2_option>& drawing_order,
        bool update_read_only_options, std::string& error_message,
        notifications_model& notifications)
    {
        for (auto& opt : drawing_order)
        {
            draw_option(opt, update_read_only_options, error_message, notifications);
        }

        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            auto opt = static_cast<rs2_option>(i);
            if (opt == RS2_OPTION_FRAMES_QUEUE_SIZE) continue;
            if (std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
            {
                draw_option(opt, update_read_only_options, error_message, notifications);
            }
        }
    }

    bool option_model::draw_option(bool update_read_only_options,
        bool is_streaming,
        std::string& error_message, notifications_model& model)
    {
        if (update_read_only_options)
        {
            update_supported(error_message);
            if (supported && is_streaming)
            {
                update_read_only_status(error_message);
                if (read_only)
                {
                    update_all_fields(error_message, model);
                }
            }
        }
        return draw(error_message, model);
    }

    stream_model::stream_model()
        : texture(std::unique_ptr<texture_buffer>(new texture_buffer())),
        _stream_not_alive(std::chrono::milliseconds(1500))
    {}

    texture_buffer* stream_model::upload_frame(frame&& f)
    {
        if (dev && dev->is_paused()) return nullptr;

        last_frame = std::chrono::high_resolution_clock::now();

        auto image = f.as<video_frame>();
        auto width = (image) ? image.get_width() : 640.f;
        auto height = (image) ? image.get_height() : 480.f;

        size = { static_cast<float>(width), static_cast<float>(height) };
        profile = f.get_profile();
        frame_number = f.get_frame_number();
        timestamp_domain = f.get_frame_timestamp_domain();
        timestamp = f.get_timestamp();
        fps.add_timestamp(f.get_timestamp(), f.get_frame_number());


        // populate frame metadata attributes
        for (auto i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
        {
            if (f.supports_frame_metadata((rs2_frame_metadata_value)i))
                frame_md.md_attributes[i] = std::make_pair(true, f.get_frame_metadata((rs2_frame_metadata_value)i));
            else
                frame_md.md_attributes[i].first = false;
        }

        texture->upload(f);
        return texture.get();
    }

    void outline_rect(const rect& r)
    {
        glPushAttrib(GL_ENABLE_BIT);

        glLineWidth(1);
        glLineStipple(1, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);

        glBegin(GL_LINE_STRIP);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x, r.y + r.h);
        glVertex2f(r.x + r.w, r.y + r.h);
        glVertex2f(r.x + r.w, r.y);
        glVertex2f(r.x, r.y);
        glEnd();

        glPopAttrib();
    }

    void draw_rect(const rect& r, int line_width)
    {
        glPushAttrib(GL_ENABLE_BIT);

        glLineWidth(line_width);

        glBegin(GL_LINE_STRIP);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x, r.y + r.h);
        glVertex2f(r.x + r.w, r.y + r.h);
        glVertex2f(r.x + r.w, r.y);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x, r.y + r.h);
        glVertex2f(r.x + r.w, r.y + r.h);
        glVertex2f(r.x + r.w, r.y);
        glVertex2f(r.x, r.y);
        glEnd();

        glPopAttrib();
    }

    bool stream_model::is_stream_visible()
    {
        if (dev &&
            (dev->is_paused() ||
            (dev->streaming && dev->dev.is<playback>()) ||
                (dev->streaming /*&& texture->get_last_frame()*/)))
        {
            return true;
        }
        return false;
    }

    bool stream_model::is_stream_alive()
    {
        if (dev &&
            (dev->is_paused() ||
            (dev->streaming && dev->dev.is<playback>())))
        {
            last_frame = std::chrono::high_resolution_clock::now();
            return true;
        }

        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - last_frame;
        auto ms = duration_cast<milliseconds>(diff).count();
        _stream_not_alive.add_value(ms > _frame_timeout + _min_timeout);
        return !_stream_not_alive.eval();
    }

    void stream_model::begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p)
    {
        dev = d;
        original_profile = p;
        profile = p;
        texture->colorize = d->depth_colorizer;

        if (auto vd = p.as<video_stream_profile>())
        {
            size = {
                static_cast<float>(vd.width()),
                static_cast<float>(vd.height()) };
        };
        _stream_not_alive.reset();

    }

    void stream_model::update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message)
    {
        if (dev->roi_checked)
        {
            auto&& sensor = dev->s;
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
                    r = r.normalize(stream_rect).unnormalize(_normalized_zoom.unnormalize(get_stream_bounds()));
                    dev->roi_rect = r; // Store new rect in device coordinates into the subdevice object

                    // Send it to firmware:
                    // Step 1: get rid of negative width / height
                    region_of_interest roi{};
                    roi.min_x = std::min(r.x, r.x + r.w);
                    roi.max_x = std::max(r.x, r.x + r.w);
                    roi.min_y = std::min(r.y, r.y + r.h);
                    roi.max_y = std::max(r.y, r.y + r.h);

                    try
                    {
                        // Step 2: send it to firmware
                        if (sensor->is<roi_sensor>())
                        {
                            sensor->as<roi_sensor>().set_region_of_interest(roi);
                        }
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
                        if (sensor->is<roi_sensor>())
                        {
                            sensor->as<roi_sensor>().set_region_of_interest({ x_margin, y_margin,
                                                                             (int)size.x - x_margin - 1,
                                                                             (int)size.y - y_margin - 1 });
                        }

                        roi_display_rect = { 0, 0, 0, 0 };
                        dev->roi_rect = { 0, 0, 0, 0 };
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }

                dev->roi_checked = false;
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
                r = r.normalize(_normalized_zoom.unnormalize(get_stream_bounds())).unnormalize(stream_rect).cut_by(stream_rect);
                roi_display_rect = r;
            }

            // Display ROI rect
            glColor3f(1.0f, 1.0f, 1.0f);
            outline_rect(roi_display_rect);
        }
    }

    std::string get_file_name(const std::string& path)
    {
        std::string file_name;
        for (auto rit = path.rbegin(); rit != path.rend(); ++rit)
        {
            if (*rit == '\\' || *rit == '/')
                break;
            file_name += *rit;
        }
        std::reverse(file_name.begin(), file_name.end());
        return file_name;
    }

    bool draw_combo_box(const std::string& id, const std::vector<std::string>& device_names, int& new_index)
    {
        std::vector<const char*>  device_names_chars = get_string_pointers(device_names);
        return ImGui::Combo(id.c_str(), &new_index, device_names_chars.data(), static_cast<int>(device_names.size()));
    }

    void viewer_model::show_3dviewer_header(ImFont* font, rs2::rect stream_rect, bool& paused)
    {
        //frame texture_map;
        //static auto last_frame_number = 0;
        //for (auto&& s : streams)
        //{
        //    if (s.second.profile.stream_type() == RS2_STREAM_DEPTH && s.second.texture->last)
        //    {
        //        auto frame_number = s.second.texture->last.get_frame_number();

        //        if (last_frame_number == frame_number) break;
        //        last_frame_number = frame_number;

        //        for (auto&& s : streams)
        //        {
        //            if (s.second.profile.stream_type() != RS2_STREAM_DEPTH && s.second.texture->last)
        //            {
        //                rendered_tex_id = s.second.texture->get_gl_handle();
        //                texture_map = s.second.texture->last; // also save it for later
        //                pc.map_to(texture_map);
        //                break;
        //            }
        //        }

        //        if (s.second.dev && !s.second.dev->is_paused())
        //            depth_frames_to_render.enqueue(s.second.texture->last);
        //        break;
        //    }
        //}


        const auto top_bar_height = 32.f;
        const auto num_of_buttons = 4;

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushFont(font);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, header_window_bg);
        ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y });
        ImGui::SetNextWindowSize({ stream_rect.w, top_bar_height });
        std::string label = to_string() << "header of 3dviewer";
        ImGui::Begin(label.c_str(), nullptr, flags);

        int selected_depth_source = -1;
        std::vector<std::string> depth_sources_str;
        std::vector<int> depth_sources;
        int i = 0;
        for (auto&& s : streams)
        {
            if (s.second.is_stream_visible() &&
                s.second.texture->get_last_frame() &&
                s.second.profile.stream_type() == RS2_STREAM_DEPTH)
            {
                if (selected_depth_source_uid == -1)
                {
                    selected_depth_source_uid = streams_origin[s.second.profile.unique_id()];
                }
                if (streams_origin[s.second.profile.unique_id()] == selected_depth_source_uid)
                {
                    selected_depth_source = i;
                }

                depth_sources.push_back(s.second.profile.unique_id());

                auto dev_name = s.second.dev ? s.second.dev->dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
                auto stream_name = rs2_stream_to_string(s.second.profile.stream_type());

                depth_sources_str.push_back(to_string() << dev_name << " " << stream_name);

                i++;
            }
        }

        if (depth_sources_str.size() > 0 && allow_3d_source_change)
        {
            ImGui::SetCursorPos({ 7, 7 });
            ImGui::Text("Depth Source:"); ImGui::SameLine();

            ImGui::SetCursorPosY(7);
            ImGui::PushItemWidth(190);
            draw_combo_box("##Depth Source", depth_sources_str, selected_depth_source);
            i = 0;
            for (auto&& s : streams)
            {
                if (s.second.is_stream_visible() &&
                    s.second.texture->get_last_frame() &&
                    s.second.profile.stream_type() == RS2_STREAM_DEPTH)
                {
                    if (i == selected_depth_source)
                    {
                        selected_depth_source_uid =  streams_origin[s.second.profile.unique_id()];
                    }
                    i++;
                }
            }

            ImGui::PopItemWidth();
            ImGui::SameLine();
        }

        int selected_tex_source = 0;
        std::vector<std::string> tex_sources_str;
        std::vector<int> tex_sources;
        i = 0;
        for (auto&& s : streams)
        {
            if (s.second.is_stream_visible() &&
                s.second.texture->get_last_frame())
            {
                if (selected_tex_source_uid == -1)
                {
                    selected_tex_source_uid = streams_origin[s.second.profile.unique_id()];
                }
                if (streams_origin[s.second.profile.unique_id()] == selected_tex_source_uid)
                {
                    selected_tex_source = i;
                }

                tex_sources.push_back(s.second.profile.unique_id());

                auto dev_name = s.second.dev ? s.second.dev->dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
                std::string stream_name = rs2_stream_to_string(s.second.profile.stream_type());
                if (s.second.profile.stream_index())
                    stream_name += "_" + std::to_string(s.second.profile.stream_index());
                tex_sources_str.push_back(to_string() << dev_name << " " << stream_name);

                i++;
            }
        }

        if (!allow_3d_source_change) ImGui::SetCursorPos({ 7, 7 });
        // Only allow to change texture if we have something to put it on:
        if (tex_sources_str.size() > 0 && depth_sources_str.size() > 0)
        {
            ImGui::SetCursorPosY(7);
            ImGui::Text("Texture Source:"); ImGui::SameLine();

            ImGui::SetCursorPosY(7);
            ImGui::PushItemWidth(200);
            draw_combo_box("##Tex Source", tex_sources_str, selected_tex_source);

            i = 0;
            for (auto&& s : streams)
            {
                if (s.second.is_stream_visible() &&
                    s.second.texture->get_last_frame())
                {
                    if (i == selected_tex_source)
                    {
                        selected_tex_source_uid = streams_origin[s.second.profile.unique_id()];
                        texture.colorize = s.second.texture->colorize;
                    }
                    i++;
                }
            }
            ImGui::PopItemWidth();
        }


        //ImGui::SetCursorPosY(9);
        //ImGui::Text("Viewport:"); ImGui::SameLine();
        //ImGui::SetCursorPosY(7);

        //ImGui::PushItemWidth(70);
        //std::vector<std::string> viewports{ "Top", "Front", "Side" };
        //auto selected_view = -1;
        //if (draw_combo_box("viewport_combo", viewports, selected_view))
        //{
        //    if (selected_view == 0)
        //    {
        //        reset_camera({ 0.0f, 1.5f, 0.0f });
        //    }
        //    else if (selected_view == 1)
        //    {
        //        reset_camera({ 0.0f, 0.0f, -1.5f });
        //    }
        //    else if (selected_view == 2)
        //    {
        //        reset_camera({ -1.5f, 0.0f, -0.0f });
        //    }
        //}
        //ImGui::PopItemWidth();


        ImGui::SetCursorPos({ stream_rect.w - 32 * num_of_buttons - 5, 0 });

        if (paused)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            label = to_string() << u8"\uf04b" << "##Resume 3d";
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                paused = false;
                for (auto&& s : streams)
                {
                    if (s.second.dev) s.second.dev->resume();
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Resume All");
            }
            ImGui::PopStyleColor(2);
        }
        else
        {
            label = to_string() << u8"\uf04c" << "##Pause 3d";
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                paused = true;
                for (auto&& s : streams)
                {
                    if (s.second.dev) s.second.dev->pause();
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Pause All");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(u8"\uf0c7", { 24, top_bar_height }))
        {
            if (auto ret = file_dialog_open(save_file, "Polygon File Format (PLY)\0*.ply\0", NULL, NULL))
            {
                auto model = ppf.get_points();

                frame tex;
                if (selected_tex_source_uid >= 0)
                {
                    tex = streams[selected_tex_source_uid].texture->get_last_frame(true);
                    if (tex) ppf.update_texture(tex);
                }

                std::string fname(ret);
                if (!ends_with(to_lower(fname), ".ply")) fname += ".ply";
                export_to_ply(fname.c_str(), not_model, model, tex);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Export 3D model to PLY format");

        ImGui::SameLine();

        if (ImGui::Button(u8"\uf021", { 24, top_bar_height }))
        {
            reset_camera();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset View");

        ImGui::SameLine();

        if (support_non_syncronized_mode)
        {
            if (synchronization_enable)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                if (ImGui::Button(u8"\uf023", { 24, top_bar_height }))
                {
                    synchronization_enable = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Disable syncronization between the pointcloud and the texture");
                ImGui::PopStyleColor(2);
            }
            else
            {
                if (ImGui::Button(u8"\uf09c", { 24, top_bar_height }))
                {
                    synchronization_enable = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Keep the pointcloud and the texture sycronized");
            }
        }


        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar();

        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, dark_sensor_bg);
        ImGui::SetNextWindowPos({ stream_rect.x + stream_rect.w - 265, stream_rect.y + top_bar_height + 5 });
        ImGui::SetNextWindowSize({ 260, 65 });
        ImGui::Begin("3D Info box", nullptr, flags);

        ImGui::Columns(2, 0, false);
        ImGui::SetColumnOffset(1, 90);

        ImGui::Text("Rotate Camera:");
        ImGui::NextColumn();
        ImGui::Text("Left Mouse Button");
        ImGui::NextColumn();

        ImGui::Text("Pan:");
        ImGui::NextColumn();
        ImGui::Text("Middle Mouse Button");
        ImGui::NextColumn();

        ImGui::Text("Zoom In/Out:");
        ImGui::NextColumn();
        ImGui::Text("Mouse Wheel");
        ImGui::NextColumn();

        ImGui::Columns();

        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);


        ImGui::PopFont();
    }

    void viewer_model::gc_streams()
    {
        std::lock_guard<std::mutex> lock(streams_mutex);
        std::vector<int> streams_to_remove;
        for (auto&& kvp : streams)
        {
            if (!kvp.second.is_stream_visible() &&
                (!kvp.second.dev || (!kvp.second.dev->is_paused() && !kvp.second.dev->streaming)))
            {
                if (kvp.first == selected_depth_source_uid)
                {
                    ppf.depth_stream_active = false;
                }
                streams_to_remove.push_back(kvp.first);
            }
        }
        for (auto&& i : streams_to_remove) {
            if(selected_depth_source_uid == i)
                selected_depth_source_uid = -1;
            if(selected_tex_source_uid == i)
                selected_tex_source_uid = -1;
            streams.erase(i);

            if(ppf.frames_queue.find(i) != ppf.frames_queue.end())
            {
                ppf.frames_queue.erase(i);
            }
        }
    }

    void stream_model::show_stream_header(ImFont* font, rs2::rect stream_rect, viewer_model& viewer)
    {
        const auto top_bar_height = 32.f;
        auto num_of_buttons = 4;
        if (!viewer.allow_stream_close) num_of_buttons--;
        if (viewer.streams.size() > 1) num_of_buttons++;

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushFont(font);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, header_window_bg);
        ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y - top_bar_height });
        ImGui::SetNextWindowSize({ stream_rect.w, top_bar_height });
        std::string label = to_string() << "Stream of " << profile.unique_id();
        ImGui::Begin(label.c_str(), nullptr, flags);

        ImGui::SetCursorPos({ 9, 7 });

        std::string tooltip;
        if (dev && dev->dev.supports(RS2_CAMERA_INFO_NAME) &&
            dev->dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER) &&
            dev->s->supports(RS2_CAMERA_INFO_NAME))
        {
            std::string dev_name = dev->dev.get_info(RS2_CAMERA_INFO_NAME);
            std::string dev_serial = dev->dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
            std::string sensor_name = dev->s->get_info(RS2_CAMERA_INFO_NAME);
            std::string stream_name = rs2_stream_to_string(profile.stream_type());

            tooltip = to_string() << dev_name << " S/N:" << dev_serial << " | " << sensor_name << ", " << stream_name << " stream";
            const auto approx_char_width = 12;
            if (stream_rect.w - 32 * num_of_buttons >= (dev_name.size() + dev_serial.size() + sensor_name.size() + stream_name.size()) * approx_char_width)
                label = tooltip;
            else if (stream_rect.w - 32 * num_of_buttons >= (dev_name.size() + sensor_name.size() + stream_name.size()) * approx_char_width)
                label = to_string() << dev_name << " | " << sensor_name << " " << stream_name << " stream";
            else if (stream_rect.w - 32 * num_of_buttons >= (dev_name.size() + stream_name.size()) * approx_char_width)
                label = to_string() << dev_name << " " << stream_name << " stream";
            else if (stream_rect.w - 32 * num_of_buttons >= stream_name.size() * approx_char_width * 2)
                label = to_string() << stream_name << " stream";
            else
                label = "";
        }
        else
        {
            label = to_string() << "Unknown " << rs2_stream_to_string(profile.stream_type()) << " stream";
            tooltip = label;
        }

        ImGui::PushTextWrapPos(stream_rect.w - 32 * num_of_buttons - 5);
        ImGui::Text("%s", label.c_str());
        if (tooltip != label && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip.c_str());
        ImGui::PopTextWrapPos();

        ImGui::SetCursorPos({ stream_rect.w - 32 * num_of_buttons, 0 });

        auto p = dev->dev.as<playback>();
        if (dev->is_paused() || (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            label = to_string() << u8"\uf04b" << "##Resume " << profile.unique_id();
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                if (p)
                {
                    p.resume();
                }
                dev->resume();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Resume sensor");
            }
            ImGui::PopStyleColor(2);
        }
        else
        {
            label = to_string() << u8"\uf04c" << "##Pause " << profile.unique_id();
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                if (p)
                {
                    p.pause();
                }
                dev->pause();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Pause sensor");
            }
        }
        ImGui::SameLine();

        label = to_string() << u8"\uf030" << "##Snapshot " << profile.unique_id();
        if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
        {
            auto ret = file_dialog_open(save_file, "Portable Network Graphics (PNG)\0*.png\0", NULL, NULL);

            if (ret)
            {
                std::string filename = ret;
                if (!ends_with(to_lower(filename), ".png")) filename += ".png";

                auto frame = texture->get_last_frame(true).as<video_frame>();
                if (frame)
                {
                    save_to_png(filename.data(), frame.get_width(), frame.get_height(), frame.get_bytes_per_pixel(), frame.get_data(), frame.get_width() * frame.get_bytes_per_pixel());

                    viewer.not_model.add_notification({ to_string() << "Snapshot was saved to " << filename,
                        0, RS2_LOG_SEVERITY_INFO,
                        RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                }
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save snapshot");
        }
        ImGui::SameLine();

        label = to_string() << u8"\uf05a" << "##Info " << profile.unique_id();
        if (show_stream_details)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);

            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                show_stream_details = false;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hide stream info overlay");
            }

            ImGui::PopStyleColor(2);
        }
        else
        {
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                show_stream_details = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Show stream info overlay");
            }
        }
        ImGui::SameLine();

        if (viewer.streams.size() > 1)
        {
            if (!viewer.fullscreen)
            {
                label = to_string() << u8"\uf2d0" << "##Maximize " << profile.unique_id();

                if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
                {
                    viewer.fullscreen = true;
                    viewer.selected_stream = this;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Maximize stream to full-screen");
                }

                ImGui::SameLine();
            }
            else if (viewer.fullscreen)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);

                label = to_string() << u8"\uf2d2" << "##Restore " << profile.unique_id();

                if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
                {
                    viewer.fullscreen = false;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Restore tile view");
                }

                ImGui::PopStyleColor(2);
                ImGui::SameLine();
            }
        }
        else
        {
            viewer.fullscreen = false;
        }

        if (viewer.allow_stream_close)
        {
            label = to_string() << u8"\uf00d" << "##Stop " << profile.unique_id();
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                dev->stop(viewer);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Stop this sensor");
            }

        }

        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar();

        if (show_stream_details)
        {
            auto flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

            ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(9, 11, 13, 100));
            ImGui::SetNextWindowPos({ stream_rect.x + stream_rect.w - 275, stream_rect.y + 5 });
            ImGui::SetNextWindowSize({ 270, 45 });
            std::string label = to_string() << "Stream Info of " << profile.unique_id();
            ImGui::Begin(label.c_str(), nullptr, flags);

            label = to_string() << size.x << "x" << size.y << ", "
                << rs2_format_to_string(profile.format()) << ", "
                << "FPS:";
            ImGui::Text("%s", label.c_str());
            ImGui::SameLine();

            label = to_string() << std::setprecision(2) << std::fixed << fps.get_fps();
            ImGui::Text("%s", label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("FPS is calculated based on timestamps and not viewer time");
            }
            ImGui::SameLine();

            label = to_string() << "Frame#" << frame_number;
            ImGui::Text("%s", label.c_str());

            ImGui::Columns(2, 0, false);
            ImGui::SetColumnOffset(1, 160);
            label = to_string() << "Timestamp: " << std::fixed << std::setprecision(3) << timestamp;
            ImGui::Text("%s", label.c_str());
            ImGui::NextColumn();

            label = to_string() << rs2_timestamp_domain_to_string(timestamp_domain);

            if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hardware Timestamp unavailable! This is often an indication of improperly applied Kernel patch.\nPlease refer to installation.md for mode information");
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

            ImGui::Columns(1);

            ImGui::End();
            ImGui::PopStyleColor(6);
            ImGui::PopStyleVar(2);
        }
    }

    void stream_model::show_stream_footer(const rect &stream_rect, const  mouse_info& mouse)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + stream_rect.h - 30 });
        ImGui::SetNextWindowSize({ stream_rect.w, 30 });
        std::string label = to_string() << "Footer for stream of " << profile.unique_id();
        ImGui::Begin(label.c_str(), nullptr, flags);

        if (stream_rect.contains(mouse.cursor))
        {
            std::stringstream ss;
            rect cursor_rect{ mouse.cursor.x, mouse.cursor.y };
            auto ts = cursor_rect.normalize(stream_rect);
            auto pixels = ts.unnormalize(_normalized_zoom.unnormalize(get_stream_bounds()));
            auto x = (int)pixels.x;
            auto y = (int)pixels.y;

            ss << std::fixed << std::setprecision(0) << x << ", " << y;

            float val{};
            if (texture->try_pick(x, y, &val))
            {
                ss << ", *p: 0x" << std::hex << val;
            }

            if (texture->get_last_frame().is<depth_frame>())
            {
                auto meters = texture->get_last_frame().as<depth_frame>().get_distance(x, y);
                if (meters > 0)
                {
                    ss << std::dec << ", "
                        << std::setprecision(2) << meters << " meters";
                }
            }

            label = ss.str();
            ImGui::Text("%s", label.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    rect stream_model::get_normalized_zoom(const rect& stream_rect, const mouse_info& g, bool is_middle_clicked, float zoom_val)
    {
        rect zoomed_rect = dev->normalized_zoom.unnormalize(stream_rect);
        if (stream_rect.contains(g.cursor))
        {
            if (!is_middle_clicked)
            {
                if (zoom_val < 1.f)
                {
                    zoomed_rect = zoomed_rect.center_at(g.cursor)
                        .zoom(zoom_val)
                        .fit({ 0, 0, 40, 40 })
                        .enclose_in(zoomed_rect)
                        .enclose_in(stream_rect);
                }
                else if (zoom_val > 1.f)
                {
                    zoomed_rect = zoomed_rect.zoom(zoom_val).enclose_in(stream_rect);
                }
            }
            else
            {
                auto dir = g.cursor - _middle_pos;

                if (dir.length() > 0)
                {
                    zoomed_rect = zoomed_rect.pan(1.1f * dir).enclose_in(stream_rect);
                }
            }
            dev->normalized_zoom = zoomed_rect.normalize(stream_rect);
        }
        return dev->normalized_zoom;
    }

    void stream_model::show_frame(const rect& stream_rect, const mouse_info& g, std::string& error_message)
    {
        auto zoom_val = 1.f;
        if (stream_rect.contains(g.cursor))
        {
            static const auto wheel_step = 0.1f;
            auto mouse_wheel_value = -g.mouse_wheel * 0.1f;
            if (mouse_wheel_value > 0)
                zoom_val += wheel_step;
            else if (mouse_wheel_value < 0)
                zoom_val -= wheel_step;
        }

        auto is_middle_clicked = ImGui::GetIO().MouseDown[0] ||
            ImGui::GetIO().MouseDown[2];

        if (!_mid_click && is_middle_clicked)
            _middle_pos = g.cursor;

        _mid_click = is_middle_clicked;

        _normalized_zoom = get_normalized_zoom(stream_rect,
            g, is_middle_clicked,
            zoom_val);
        texture->show(stream_rect, 1.f, _normalized_zoom);

        if (dev && dev->show_algo_roi)
        {
            rect r{ float(dev->algo_roi.min_x), float(dev->algo_roi.min_y),
                    float(dev->algo_roi.max_x - dev->algo_roi.min_x),
                    float(dev->algo_roi.max_y - dev->algo_roi.min_y) };

            r = r.normalize(_normalized_zoom.unnormalize(get_stream_bounds())).unnormalize(stream_rect).cut_by(stream_rect);

            glColor3f(yellow.x, yellow.y, yellow.z);
            draw_rect(r, 2);

            std::string message = "Metrics Region of Interest";
            auto msg_width = stb_easy_font_width((char*)message.c_str());
            if (msg_width < r.w)
                draw_text(r.x + r.w / 2 - msg_width / 2, r.y + 10, message.c_str());

            glColor3f(1.f, 1.f, 1.f);
        }

        update_ae_roi_rect(stream_rect, g, error_message);
        texture->show_preview(stream_rect, _normalized_zoom);

        if (is_middle_clicked)
            _middle_pos = g.cursor;
    }

    void stream_model::show_metadata(const mouse_info& g)
    {
        auto flags = ImGuiWindowFlags_ShowBorders;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.3f, 0.3f, 0.5 });
        ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.f, 0.25f, 0.3f, 1 });
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.f, 0.3f, 0.8f, 1 });
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 });

        std::string label = to_string() << profile.stream_name() << " Stream Metadata";
        ImGui::Begin(label.c_str(), nullptr, flags);

        // Print all available frame metadata attributes
        for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
        {
            if (frame_md.md_attributes[i].first)
            {
                label = to_string() << rs2_frame_metadata_to_string((rs2_frame_metadata_value)i) << " = " << frame_md.md_attributes[i].second;
                ImGui::Text("%s", label.c_str());
            }
        }

        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    void device_model::reset()
    {
        subdevices.resize(0);
        _recorder.reset();
    }

    std::pair<std::string, std::string> get_device_name(const device& dev)
    {
        // retrieve device name
        std::string name = (dev.supports(RS2_CAMERA_INFO_NAME)) ? dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";

        // retrieve device serial number
        std::string serial = (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) ? dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "Unknown";

        std::stringstream s;

        if (dev.is<playback>())
        {
            auto playback_dev = dev.as<playback>();

            s << "Playback device: ";
            name += (to_string() << " (File: " << playback_dev.file_name() << ")");
        }
        s << std::setw(25) << std::left << name;
        return std::make_pair(s.str(), serial);        // push name and sn to list
    }

    device_model::device_model(device& dev, std::string& error_message, viewer_model& viewer)
        : dev(dev)
    {
        for (auto&& sub : dev.query_sensors())
        {
            auto model = std::make_shared<subdevice_model>(dev, std::make_shared<sensor>(sub), error_message);
            subdevices.push_back(model);
        }

        auto name = get_device_name(dev);
        id = to_string() << name.first << ", " << name.second;

        // Initialize static camera info:
        for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
        {
            auto info = static_cast<rs2_camera_info>(i);

            try
            {
                if (dev.supports(info))
                {
                    auto value = dev.get_info(info);
                    infos.push_back({ std::string(rs2_camera_info_to_string(info)),
                                      std::string(value) });
                }
            }
            catch (...)
            {
                infos.push_back({ std::string(rs2_camera_info_to_string(info)),
                                  std::string("???") });
            }
        }

        if (dev.is<playback>())
        {
            for (auto&& sub : subdevices)
            {
                for (auto&& p : sub->profiles)
                {
                    sub->stream_enabled[p.unique_id()] = true;
                }
            }
            play_defaults(viewer);
        }
    }
    void device_model::play_defaults(viewer_model& viewer)
    {
        for (auto&& sub : subdevices)
        {
            if (!sub->streaming)
            {
                std::vector<rs2::stream_profile> profiles;
                try
                {
                    profiles = sub->get_selected_profiles();
                }
                catch (...)
                {
                    continue;
                }

                if (profiles.empty())
                    continue;

                sub->play(profiles, viewer);

                for (auto&& profile : profiles)
                {
                    viewer.begin_stream(sub, profile);

                }
            }
        }
    }

    void viewer_model::show_event_log(ImFont* font_14, float x, float y, float w, float h)
    {
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar;

        ImGui::PushFont(font_14);
        ImGui::SetNextWindowPos({ x, y });
        ImGui::SetNextWindowSize({ w, h });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        is_output_collapsed = ImGui::Begin("Output", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ShowBorders);

        int i = 0;
        not_model.foreach_log([&](const std::string& line) {
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);

            auto rc = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ rc.x + 10, rc.y + 4 });

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::Text(u8"\uf068"); ImGui::SameLine();
            ImGui::PopStyleColor();

            rc = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ rc.x, rc.y - 4 });

            std::string label = to_string() << "##log_entry" << i++;
            ImGui::InputText(label.c_str(),
                        (char*)line.data(),
                        line.size() + 1,
                        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor(2);

            rc = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ rc.x, rc.y - 6 });
        });

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

    void viewer_model::popup_if_error(ImFont* font_14, std::string& error_message)
    {
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar;

        ImGui::PushFont(font_14);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        // The list of errors the user asked not to show again:
        static std::set<std::string> errors_not_to_show;
        static bool dont_show_this_error = false;
        auto simplify_error_message = [](const std::string& s) {
            std::regex e("\\b(0x)([^ ,]*)");
            return std::regex_replace(s, e, "address");
        };

        std::string name = u8"\uf071 Oops, something went wrong!";

        if (error_message != "")
        {
            if (errors_not_to_show.count(simplify_error_message(error_message)))
            {
                not_model.add_notification({ error_message,
                    std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count(),
                    RS2_LOG_SEVERITY_ERROR,
                    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                error_message = "";
            }
            else
            {
                ImGui::OpenPopup(name.c_str());
            }
        }
        if (ImGui::BeginPopupModal(name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("RealSense error calling:");
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            ImGui::InputTextMultiline("error", const_cast<char*>(error_message.c_str()),
                error_message.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                if (dont_show_this_error)
                {
                    errors_not_to_show.insert(simplify_error_message(error_message));
                }
                error_message = "";
                ImGui::CloseCurrentPopup();
                dont_show_this_error = false;
            }

            ImGui::SameLine();
            ImGui::Checkbox("Don't show this error again", &dont_show_this_error);

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }
    void viewer_model::show_icon(ImFont* font_18, const char* label_str, const char* text, int x, int y, int id,
        const ImVec4& text_color, const std::string& tooltip)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushFont(font_18);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::SetNextWindowPos({ (float)x, (float)y });
        ImGui::SetNextWindowSize({ 320.f, 32.f });
        std::string label = to_string() << label_str << id;
        ImGui::Begin(label.c_str(), nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        ImGui::Text("%s", text);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered() && tooltip != "")
            ImGui::SetTooltip("%s", tooltip.c_str());

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    void viewer_model::show_paused_icon(ImFont* font_18, int x, int y, int id)
    {
        show_icon(font_18, "paused_icon", u8"\uf04c", x, y, id, white);
    }
    void viewer_model::show_recording_icon(ImFont* font_18, int x, int y, int id, float alpha_delta)
    {
        show_icon(font_18, "recording_icon", u8"\uf111", x, y, id, from_rgba(255, 46, 54, alpha_delta * 255));
    }

    rs2::frame post_processing_filters::apply_filters(rs2::frame f)
    {

        if (f.get_profile().stream_type() == RS2_STREAM_DEPTH)
        {
            for (auto&& s : viewer.streams)
            {
                if(!s.second.dev) continue;
                auto dev = s.second.dev;

                if(s.second.original_profile.unique_id() == f.get_profile().unique_id())
                {
                    if (dev->post_processing_enabled)
                    {
                        auto dec_filter = s.second.dev->decimation_filter;
                        auto depth_2_disparity = s.second.dev->depth_to_disparity;
                        auto spatial_filter = s.second.dev->spatial_filter;
                        auto temp_filter = s.second.dev->temporal_filter;
                        auto disparity_2_depth = s.second.dev->disparity_to_depth;

                        if (dec_filter->enabled)
                            f = dec_filter->invoke(f);

                        if (depth_2_disparity->enabled)
                            f = depth_2_disparity->invoke(f);

                        if (spatial_filter->enabled)
                            f = spatial_filter->invoke(f);

                        if (temp_filter->enabled)
                            f = temp_filter->invoke(f);

                        if (disparity_2_depth->enabled)
                            f = disparity_2_depth->invoke(f);

                        return f;
                    }
                }
            }
        }
        return f;
    }

    std::vector<rs2::frame> post_processing_filters::handle_frame(rs2::frame f)
    {
        std::vector<rs2::frame> res;
        auto filtered = apply_filters(f);
        res.push_back(filtered);

        auto uid = f.get_profile().unique_id();
        auto new_uid = filtered.get_profile().unique_id();
        viewer.streams_origin[new_uid] = uid;

        if(viewer.is_3d_view)
        {
            if(viewer.is_3d_depth_source(f))
            {
                res.push_back(pc.calculate(filtered));
            }
            if(viewer.is_3d_texture_source(f))
            {
                update_texture(filtered);
            }
        }

        return res;
    }

    void post_processing_filters::proccess(rs2::frame f, const rs2::frame_source& source)
    {
        points p;
        std::vector<frame> results;
        frame res;

        if (auto composite = f.as<rs2::frameset>())
        {
            for (auto&& f : composite)
            {
                auto res = handle_frame(f);
                results.insert(results.end(), res.begin(), res.end());
            }
        }
        else
        {
             auto res = handle_frame(f);
             results.insert(results.end(), res.begin(), res.end());
        }

        res = source.allocate_composite_frame(results);

        if(res)
            source.frame_ready(std::move(res));
    }


    void post_processing_filters::render_loop()
    {
        while (keep_calculating)
        {

            try
            {
                frame frm;
                if(viewer.synchronization_enable)
                {
                    auto index = 0;
                    while (syncer_queue.poll_for_frame(&frm) && ++index <= syncer_queue.capacity())
                    {
                        processing_block.invoke(frm);
                    }
                }
                else
                {
                    std::map<int, rs2::frame_queue> frames_queue_local;
                    {
                        std::lock_guard<std::mutex> lock(viewer.streams_mutex);
                        frames_queue_local = frames_queue;
                    }
                    for(auto&& q :  frames_queue_local)
                    {

                        frame frm;
                        if (q.second.poll_for_frame(&frm))
                        {
                            processing_block.invoke(frm);
                        }
                    }
                }
            }
            catch (...) {}

        }
    }

    void viewer_model::show_no_stream_overlay(ImFont* font_18, int min_x, int min_y, int max_x, int max_y)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushFont(font_18);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::SetNextWindowPos({ (min_x + max_x) / 2.f - 150, (min_y + max_y) / 2.f - 20 });
        ImGui::SetNextWindowSize({ float(max_x - min_x), 50.f });
        ImGui::Begin("nostreaming_popup", nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_Text, sensor_header_light_blue);
        ImGui::Text(u8"Nothing is streaming! Toggle \uf204 to start");
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void viewer_model::show_no_device_overlay(ImFont* font_18, int x, int y)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushFont(font_18);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::SetNextWindowPos({ float(x), float(y) });
        ImGui::SetNextWindowSize({ 250.f, 50.f });
        ImGui::Begin("nostreaming_popup", nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0x70, 0x8f, 0xa8, 0xff));
        ImGui::Text("Connect a RealSense Camera\nor Add Source");
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    // Generate streams layout, creates a grid-like layout with factor amount of columns
    std::map<int, rect> generate_layout(const rect& r,
        int top_bar_height, int factor,
        const std::set<stream_model*>& active_streams,
        std::map<stream_model*, int>& stream_index
    )
    {
        std::map<int, rect> results;
        if (factor == 0) return results;

        // Calc the number of rows
        auto complement = ceil((float)active_streams.size() / factor);

        auto cell_width = static_cast<float>(r.w / factor);
        auto cell_height = static_cast<float>(r.h / complement);

        auto it = active_streams.begin();
        for (auto x = 0; x < factor; x++)
        {
            for (auto y = 0; y < complement; y++)
            {
                // There might be spare boxes at the end (3 streams in 2x2 array for example)
                if (it == active_streams.end()) break;

                rect rxy = { r.x + x * cell_width, r.y + y * cell_height + top_bar_height,
                    cell_width, cell_height - top_bar_height };
                // Generate box to display the stream in
                results[stream_index[*it]] = rxy.adjust_ratio((*it)->size);
                ++it;
            }
        }

        return results;
    }

    // Return the total display area of the layout
    // The bigger it is, the more details we can see
    float evaluate_layout(const std::map<int, rect>& l)
    {
        float res = 0.f;
        for (auto&& kvp : l) res += kvp.second.area();
        return res;
    }

    std::map<int, rect> viewer_model::calc_layout(const rect& r)
    {
        const int top_bar_height = 32;

        std::set<stream_model*> active_streams;
        std::map<stream_model*, int> stream_index;
        for (auto&& stream : streams)
        {
            if (stream.second.is_stream_visible())
            {
                active_streams.insert(&stream.second);
                stream_index[&stream.second] = stream.first;
            }
        }

        if (fullscreen)
        {
            if (active_streams.count(selected_stream) == 0) fullscreen = false;
        }

        std::map<int, rect> results;

        if (fullscreen)
        {
            results[stream_index[selected_stream]] = { r.x, r.y + top_bar_height,
                                                       r.w, r.h - top_bar_height };
        }
        else
        {
            // Go over all available fx(something) layouts
            for (int f = 1; f <= active_streams.size(); f++)
            {
                auto l = generate_layout(r, top_bar_height, f,
                    active_streams, stream_index);

                // Keep the "best" layout in result
                if (evaluate_layout(l) > evaluate_layout(results))
                    results = l;
            }
        }

        return get_interpolated_layout(results);
    }

    rs2::frame viewer_model::handle_ready_frames(const rect& viewer_rect, ux_window& window, int devices, std::string& error_message)
    {
        texture_buffer* texture_frame = nullptr;
        points p;
        frame f{}, res{};

        std::map<int, frame> last_frames;
        try
        {
            auto index = 0;
            while (ppf.resulting_queue.poll_for_frame(&f) && ++index < ppf.resulting_queue_max_size)
            {
                last_frames[f.get_profile().unique_id()] = f;
            }

            for(auto&& frame : last_frames)
            {
                auto f = frame.second;
                frameset frames;
                if (frames = f.as<frameset>())
                {
                    for (auto&& frame : frames)
                    {
                        if (frame.is<points>())  // find and store the 3d points frame for later use
                        {
                            p = frame.as<points>();
                            continue;
                        }

                        if (frame.is<depth_frame>() && !paused)
                            res = frame;

                        auto texture = upload_frame(std::move(frame));

                        if ((selected_tex_source_uid == -1 && frame.get_profile().format() == RS2_FORMAT_Z16) || frame.get_profile().format() != RS2_FORMAT_ANY && is_3d_texture_source(frame))
                        {
                            texture_frame = texture;
                        }
                    }
                }
                else if (!p)
                {
                    upload_frame(std::move(f));
                }
            }
        }
        catch (const error& ex)
        {
            error_message = error_to_string(ex);
        }
        catch (const std::exception& ex)
        {
            error_message = ex.what();
        }


        gc_streams();

        window.begin_viewport();

        draw_viewport(viewer_rect, window, devices, error_message, texture_frame, p);

        not_model.draw(window.get_font(), window.width(), window.height());

        popup_if_error(window.get_font(), error_message);

        return res;
    }

    void viewer_model::reset_camera(float3 p)
    {
        target = { 0.0f, 0.0f, 0.0f };
        pos = p;

        // initialize "up" to be tangent to the sphere!
        // up = cross(cross(look, world_up), look)
        {
            float3 look = { target.x - pos.x, target.y - pos.y, target.z - pos.z };
            look = look.normalize();

            float world_up[3] = { 0.0f, 1.0f, 0.0f };

            float across[3] = {
                look.y * world_up[2] - look.z * world_up[1],
                look.z * world_up[0] - look.x * world_up[2],
                look.x * world_up[1] - look.y * world_up[0],
            };

            up.x = across[1] * look.z - across[2] * look.y;
            up.y = across[2] * look.x - across[0] * look.z;
            up.z = across[0] * look.y - across[1] * look.x;

            float up_len = up.length();
            up.x /= -up_len;
            up.y /= -up_len;
            up.z /= -up_len;
        }
    }

    void viewer_model::render_2d_view(const rect& view_rect,
        ux_window& win, int output_height,
        ImFont *font1, ImFont *font2, size_t dev_model_num,
        const mouse_info &mouse, std::string& error_message)
    {
        static periodic_timer every_sec(std::chrono::seconds(1));
        static bool icon_visible = false;
        if (every_sec) icon_visible = !icon_visible;
        float alpha = icon_visible ? 1.f : 0.2f;

        glViewport(0, 0,
            win.framebuf_width(), win.framebuf_height());
        glLoadIdentity();
        glOrtho(0, win.width(), win.height(), 0, -1, +1);

        auto layout = calc_layout(view_rect);

        if ((layout.size() == 0) && (dev_model_num > 0))
        {
            show_no_stream_overlay(font2, view_rect.x, view_rect.y, win.width(), win.height() - output_height);
        }

        for (auto &&kvp : layout)
        {
            auto&& view_rect = kvp.second;
            auto stream = kvp.first;
            auto&& stream_mv = streams[stream];
            auto&& stream_size = stream_mv.size;
            auto stream_rect = view_rect.adjust_ratio(stream_size).grow(-3);

            stream_mv.show_frame(stream_rect, mouse, error_message);

            auto p = stream_mv.dev->dev.as<playback>();
            float pos = stream_rect.x + 5;

            if (stream_mv.dev->dev.is<recorder>())
            {
                show_recording_icon(font2, pos, stream_rect.y + 5, stream_mv.profile.unique_id(), alpha);
                pos += 23;
            }

            if (!stream_mv.is_stream_alive())
            {
                show_icon(font2, "warning_icon", u8"\uf071  No Frames Received!",
                    stream_rect.center().x - 100,
                    stream_rect.center().y - 25,
                    stream_mv.profile.unique_id(),
                    blend(dark_red, alpha),
                    "Did not receive frames from the platform within a reasonable time window,\nplease try reducing the FPS or the resolution");
            }

            if (stream_mv.dev->is_paused() || (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED))
                show_paused_icon(font2, pos, stream_rect.y + 5, stream_mv.profile.unique_id());

            stream_mv.show_stream_header(font1, stream_rect, *this);
            stream_mv.show_stream_footer(stream_rect, mouse);

            glColor3f(header_window_bg.x, header_window_bg.y, header_window_bg.z);
            stream_rect.y -= 32;
            stream_rect.h += 32;
            stream_rect.w += 1;
            draw_rect(stream_rect);
        }

        // Metadata overlay windows shall be drawn after textures to preserve z-buffer functionality
        for (auto &&kvp : layout)
        {
            if (streams[kvp.first].metadata_displayed)
                streams[kvp.first].show_metadata(mouse);
        }
    }

    void viewer_model::render_3d_view(const rect& viewer_rect, texture_buffer* texture, rs2::points points)
    {
        if(!paused)
        {
            if(points)
            {
                last_points = points;
            }
            if(texture)
            {
                last_texture = texture;
            }
        }
        glViewport(viewer_rect.x, 0,
            viewer_rect.w, viewer_rect.h);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(60, (float)viewer_rect.w / viewer_rect.h, 0.001f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(view);

        glDisable(GL_TEXTURE_2D);

        glEnable(GL_DEPTH_TEST);

        glLineWidth(1);
        glBegin(GL_LINES);
        glColor4f(0.4f, 0.4f, 0.4f, 1.f);

        glTranslatef(0, 0, -1);

        for (int i = 0; i <= 6; i++)
        {
            glVertex3i(i - 3, 1, 0);
            glVertex3i(i - 3, 1, 6);
            glVertex3i(-3, 1, i);
            glVertex3i(3, 1, i);
        }
        glEnd();

        texture_buffer::draw_axis(0.1, 1);

        if (draw_plane)
        {
            glLineWidth(2);
            glBegin(GL_LINES);

            if (is_valid(roi_rect))
            {
                glColor4f(yellow.x, yellow.y, yellow.z, 1.f);

                auto rects = subdivide(roi_rect);
                for (auto&& r : rects)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        auto j = (i + 1) % 4;
                        glVertex3f(r[i].x, r[i].y, r[i].z);
                        glVertex3f(r[j].x, r[j].y, r[j].z);
                    }
                }
            }

            glEnd();
        }

        glColor4f(1.f, 1.f, 1.f, 1.f);


        if (draw_frustrum && last_points)
        {
            glLineWidth(1.f);
            glBegin(GL_LINES);

            auto intrin = last_points.get_profile().as<video_stream_profile>().get_intrinsics();

            glColor4f(sensor_bg.x, sensor_bg.y, sensor_bg.z, 0.5f);

            for (float d = 1; d < 6; d += 2)
            {
                auto get_point = [&](float x, float y) -> float3
                {
                    float point[3];
                    float pixel[2]{ x, y };
                    rs2_deproject_pixel_to_point(point, &intrin, pixel, d);
                    glVertex3f(0.f, 0.f, 0.f);
                    glVertex3fv(point);
                    return{ point[0], point[1], point[2] };
                };

                auto top_left = get_point(0, 0);
                auto top_right = get_point(intrin.width, 0);
                auto bottom_right = get_point(intrin.width, intrin.height);
                auto bottom_left = get_point(0, intrin.height);

                glVertex3fv(&top_left.x); glVertex3fv(&top_right.x);
                glVertex3fv(&top_right.x); glVertex3fv(&bottom_right.x);
                glVertex3fv(&bottom_right.x); glVertex3fv(&bottom_left.x);
                glVertex3fv(&bottom_left.x); glVertex3fv(&top_left.x);
            }

            glEnd();

            glColor4f(1.f, 1.f, 1.f, 1.f);
        }

        if (last_points)
        {
            // Non-linear correspondence customized for non-flat surface exploration
            glPointSize(std::sqrt(viewer_rect.w / last_points.get_profile().as<video_stream_profile>().width()));

            if (selected_tex_source_uid >= 0)
            {
                auto tex = last_texture->get_gl_handle();
                glBindTexture(GL_TEXTURE_2D, tex);
                glEnable(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);
            }

            //glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);

            glBegin(GL_POINTS);

            auto vertices = last_points.get_vertices();
            auto tex_coords = last_points.get_texture_coordinates();

            for (int i = 0; i < last_points.size(); i++)
            {
                if (vertices[i].z)
                {
                    glVertex3fv(vertices[i]);
                    glTexCoord2fv(tex_coords[i]);
                }

            }
            glEnd();


        }

        glDisable(GL_DEPTH_TEST);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();


        if (ImGui::IsKeyPressed('R') || ImGui::IsKeyPressed('r'))
        {
            reset_camera();
        }
    }

    void viewer_model::show_top_bar(ux_window& window, const rect& viewer_rect)
    {
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::SetNextWindowPos({ panel_width, 0 });
        ImGui::SetNextWindowSize({ window.width() - panel_width, panel_y });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, button_color);
        ImGui::Begin("Toolbar Panel", nullptr, flags);

        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Border, black);

        ImGui::SetCursorPosX(window.width() - panel_width - panel_y * 2);
        ImGui::PushStyleColor(ImGuiCol_Text, is_3d_view ? light_grey : light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, is_3d_view ? light_grey : light_blue);
        if (ImGui::Button("2D", { panel_y, panel_y })) is_3d_view = false;
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        ImGui::SetCursorPosX(window.width() - panel_width - panel_y * 1);
        auto pos1 = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_Text, !is_3d_view ? light_grey : light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, !is_3d_view ? light_grey : light_blue);
        if (ImGui::Button("3D", { panel_y,panel_y }))
        {
            is_3d_view = true;
            update_3d_camera(viewer_rect, window.get_mouse(), true);
        }
        ImGui::PopStyleColor(3);

        ImGui::GetWindowDrawList()->AddLine({ pos1.x, pos1.y + 10 }, { pos1.x,pos1.y + panel_y - 10 }, ImColor(light_grey));


        ImGui::SameLine();
        ImGui::SetCursorPosX(window.width() - panel_width - panel_y);

        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        //if (ImGui::Button(u8"\uf013\uf0d7", { panel_y,panel_y }))
        //    ImGui::OpenPopup("global_menu");

        //ImGui::PushFont(font_14);
        //if (ImGui::BeginPopup("global_menu"))
        //{
        //    ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
        //    if (ImGui::Selectable("About RealSense Viewer"))
        //    {
        //    }

        //    ImGui::PopStyleColor();
        //    ImGui::EndPopup();
        //}
        //ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void viewer_model::update_3d_camera(const rect& viewer_rect,
        mouse_info& mouse, bool force)
    {
        auto now = std::chrono::high_resolution_clock::now();
        static auto view_clock = std::chrono::high_resolution_clock::now();
        auto sec_since_update = std::chrono::duration<double, std::milli>(now - view_clock).count() / 1000;
        view_clock = now;

        if (viewer_rect.contains(mouse.cursor) || force)
        {
            arcball_camera_update(
                (float*)&pos, (float*)&target, (float*)&up, view,
                sec_since_update,
                0.2f, // zoom per tick
                -0.1f, // pan speed
                3.0f, // rotation multiplier
                viewer_rect.w, viewer_rect.h, // screen (window) size
                mouse.prev_cursor.x, mouse.cursor.x,
                mouse.prev_cursor.y, mouse.cursor.y,
                ImGui::GetIO().MouseDown[2],
                ImGui::GetIO().MouseDown[0],
                mouse.mouse_wheel,
                0);
        }

        mouse.prev_cursor = mouse.cursor;
    }

    void viewer_model::begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p)
    {
        streams[p.unique_id()].begin_stream(d, p);
        ppf.frames_queue.emplace(p.unique_id(), rs2::frame_queue(5));
    }

    bool viewer_model::is_3d_texture_source(frame f)
    {
        auto index = f.get_profile().unique_id();
        auto mapped_index = streams_origin[index];

        if(index == selected_tex_source_uid || mapped_index  == selected_tex_source_uid || selected_tex_source_uid == -1)
            return true;
        return false;
    }

    bool viewer_model::is_3d_depth_source(frame f)
    {
        auto index = f.get_profile().unique_id();
        auto mapped_index = streams_origin[index];

        if(index == selected_depth_source_uid || mapped_index  == selected_depth_source_uid
                ||(selected_depth_source_uid == -1 && f.get_profile().stream_type() == RS2_STREAM_DEPTH))
            return true;
        return false;
    }

    texture_buffer* viewer_model::upload_frame(frame&& f)
    {
        if (f.get_profile().stream_type() == RS2_STREAM_DEPTH)
            ppf.depth_stream_active = true;

        auto index = f.get_profile().unique_id();

        std::lock_guard<std::mutex> lock(streams_mutex);
        return streams[streams_origin[index]].upload_frame(std::move(f));
    }

    void device_model::start_recording(const std::string& path, std::string& error_message)
    {
        if (_recorder != nullptr)
        {
            return; //already recording
        }

        try
        {
            _recorder = std::make_shared<recorder>(path, dev);
            std::vector<std::shared_ptr<subdevice_model>> record_sensors;
            for (auto&& sub : _recorder->query_sensors())
            {
                auto model = std::make_shared<subdevice_model>(*_recorder, std::make_shared<sensor>(sub), error_message);
                record_sensors.push_back(model);
            }
            live_subdevices = subdevices;
            subdevices.clear();
            subdevices.swap(record_sensors);
            for (int i = 0; i < live_subdevices.size(); i++)
            {
                subdevices[i]->ui.selected_res_id = live_subdevices[i]->ui.selected_res_id;
                subdevices[i]->ui.selected_shared_fps_id = live_subdevices[i]->ui.selected_shared_fps_id;
                subdevices[i]->ui.selected_format_id = live_subdevices[i]->ui.selected_format_id;
                subdevices[i]->show_single_fps_list = live_subdevices[i]->show_single_fps_list;
                subdevices[i]->fpses_per_stream = live_subdevices[i]->fpses_per_stream;
                subdevices[i]->ui.selected_fps_id = live_subdevices[i]->ui.selected_fps_id;
                subdevices[i]->stream_enabled = live_subdevices[i]->stream_enabled;
                subdevices[i]->fps_values_per_stream = live_subdevices[i]->fps_values_per_stream;
                subdevices[i]->format_values = live_subdevices[i]->format_values;
            }

            is_recording = true;
        }
        catch (const rs2::error& e)
        {
            error_message = error_to_string(e);
        }
        catch (const std::exception& e)
        {
            error_message = e.what();
        }
    }

    void device_model::stop_recording()
    {
        subdevices.clear();
        subdevices = live_subdevices;
        live_subdevices.clear();
        //this->streams.clear();
        _recorder.reset();
        is_recording = false;
    }

    void device_model::pause_record()
    {
        _recorder->pause();
    }

    void device_model::resume_record()
    {
        _recorder->resume();
    }

    int device_model::draw_playback_controls(ImFont* font, viewer_model& viewer)
    {
        auto p = dev.as<playback>();
        rs2_playback_status current_playback_status = p.current_status();

        ImGui::PushFont(font);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10,0 });
        const float button_dim = 30.f;
        const bool supports_playback_step = false;


        //////////////////// Step Backwards Button ////////////////////
        std::string label = to_string() << u8"\uf048" << "##Step Backwards " << id;

        if (ImGui::ButtonEx(label.c_str(), { button_dim, button_dim }, supports_playback_step ? 0 : ImGuiButtonFlags_Disabled))
        {
            //p.skip_frames(1);
        }


        if (ImGui::IsItemHovered())
        {
            std::string tooltip = to_string() << "Step Backwards" << (supports_playback_step ? "" : "(Not available)");
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
        ImGui::SameLine();
        //////////////////// Step Backwards Button ////////////////////


        //////////////////// Stop Button ////////////////////
        label = to_string() << u8"\uf04d" << "##Stop Playback " << id;

        if (ImGui::ButtonEx(label.c_str(), { button_dim, button_dim }))
        {
            bool prev = _playback_repeat;
            _playback_repeat = false;
            p.stop();
            _playback_repeat = prev;
        }
        if (ImGui::IsItemHovered())
        {
            std::string tooltip = to_string() << "Stop Playback";
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
        ImGui::SameLine();
        //////////////////// Stop Button ////////////////////



        //////////////////// Pause/Play Button ////////////////////
        if (current_playback_status == RS2_PLAYBACK_STATUS_PAUSED || current_playback_status == RS2_PLAYBACK_STATUS_STOPPED)
        {
            label = to_string() << u8"\uf04b" << "##Play " << id;
            if (ImGui::ButtonEx(label.c_str(), { button_dim, button_dim }))
            {
                if (current_playback_status == RS2_PLAYBACK_STATUS_STOPPED)
                {
                    play_defaults(viewer);
                }
                else
                {
                    p.resume();
                    for (auto&& s : subdevices)
                    {
                        if (s->streaming)
                            s->resume();
                    }
                }

            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(current_playback_status == RS2_PLAYBACK_STATUS_PAUSED ? "Resume Playback" : "Start Playback");
            }
        }
        else
        {
            label = to_string() << u8"\uf04c" << "##Pause Playback " << id;
            if (ImGui::ButtonEx(label.c_str(), { button_dim, button_dim }))
            {
                p.pause();
                for (auto&& s : subdevices)
                {
                    if (s->streaming)
                        s->pause();
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Pause Playback");
            }
        }

        ImGui::SameLine();
        //////////////////// Pause/Play Button ////////////////////




        //////////////////// Step Forward Button ////////////////////
        label = to_string() << u8"\uf051" << "##Step Forward " << id;
        if (ImGui::ButtonEx(label.c_str(), { button_dim, button_dim }, supports_playback_step ? 0 : ImGuiButtonFlags_Disabled))
        {
            //p.skip_frames(-1);
        }
        if (ImGui::IsItemHovered())
        {
            std::string tooltip = to_string() << "Step Forward" << (supports_playback_step ? "" : "(Not available)");
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
        ImGui::SameLine();
        //////////////////// Step Forward Button ////////////////////




        /////////////////// Repeat Button /////////////////////
        if (_playback_repeat)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, white);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        }
        label = to_string() << u8"\uf0e2" << "##Repeat " << id;
        if (ImGui::ButtonEx(label.c_str(), { button_dim, button_dim }))
        {
            _playback_repeat = !_playback_repeat;
        }
        if (ImGui::IsItemHovered())
        {
            std::string tooltip = to_string() << (_playback_repeat ? "Disable " : "Enable ") << "Repeat ";
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        /////////////////// Repeat Button /////////////////////


        ImGui::PopStyleVar();

        //////////////////// Speed combo box ////////////////////
        auto pos = ImGui::GetCursorPos();
        ImGui::SetCursorPos({ 200.0f, pos.y + 3 });
        ImGui::PushItemWidth(100);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);

        label = to_string() << "## " << id;
        if (ImGui::Combo(label.c_str(), &playback_speed_index, "Speed: x0.25\0Speed: x0.5\0Speed: x1\0Speed: x1.5\0Speed: x2\0\0"))
        {
            float speed = 1;
            switch (playback_speed_index)
            {
            case 0: speed = 0.25f; break;
            case 1: speed = 0.5f; break;
            case 2: speed = 1.0f; break;
            case 3: speed = 1.5f; break;
            case 4: speed = 2.0f; break;
            default:
                throw std::runtime_error(to_string() << "Speed #" << playback_speed_index << " is unhandled");
            }
            p.set_playback_speed(speed);
        }

        ImGui::PopStyleColor(2);

        //////////////////// Speed combo box ////////////////////
        ImGui::PopFont();

        return 35;
    }

    std::string device_model::pretty_time(std::chrono::nanoseconds duration)
    {
        using namespace std::chrono;
        auto hhh = duration_cast<hours>(duration);
        duration -= hhh;
        auto mm = duration_cast<minutes>(duration);
        duration -= mm;
        auto ss = duration_cast<seconds>(duration);
        duration -= ss;
        auto ms = duration_cast<milliseconds>(duration);

        std::ostringstream stream;
        stream << std::setfill('0') << std::setw(hhh.count() >= 10 ? 2 : 1) << hhh.count() << ':' <<
            std::setfill('0') << std::setw(2) << mm.count() << ':' <<
            std::setfill('0') << std::setw(2) << ss.count();// << '.' <<
            //std::setfill('0') << std::setw(3) << ms.count();
        return stream.str();
    }

    int device_model::draw_seek_bar()
    {
        auto pos = ImGui::GetCursorPos();

        auto p = dev.as<playback>();
        rs2_playback_status current_playback_status = p.current_status();
        int64_t playback_total_duration = p.get_duration().count();
        auto progress = p.get_position();
        double part = (1.0 * progress) / playback_total_duration;
        seek_pos = static_cast<int>(std::max(0.0, std::min(part, 1.0)) * 100);

        if (seek_pos != 0 && p.current_status() == RS2_PLAYBACK_STATUS_STOPPED)
        {
            seek_pos = 0;
        }
        float seek_bar_width = 290.0f;
        ImGui::PushItemWidth(seek_bar_width);
        std::string label1 = "## " + id;
        if (ImGui::SeekSlider(label1.c_str(), &seek_pos, ""))
        {
            //Seek was dragged
            auto duration_db = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(p.get_duration());
            auto single_percent = duration_db.count() / 100;
            auto seek_time = std::chrono::duration<double, std::nano>(seek_pos * single_percent);
            p.seek(std::chrono::duration_cast<std::chrono::nanoseconds>(seek_time));
        }

        ImGui::SetCursorPos({ pos.x, pos.y + 17 });

        std::string time_elapsed = pretty_time(std::chrono::nanoseconds(progress));
        std::string duration_str = pretty_time(std::chrono::nanoseconds(playback_total_duration));
        ImGui::Text("%s", time_elapsed.c_str());
        ImGui::SameLine();
        float pos_y = ImGui::GetCursorPosY();

        ImGui::SetCursorPos({ pos.x + seek_bar_width - 41 , pos_y });
        ImGui::Text("%s", duration_str.c_str());

        return 50;
    }

    int device_model::draw_playback_panel(ImFont* font, viewer_model& view)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0, 0xae, 0xff, 255));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);


        auto pos = ImGui::GetCursorPos();
        auto controls_height = draw_playback_controls(font, view);
        ImGui::SetCursorPos({ pos.x + 8, pos.y + controls_height });
        auto seek_bar_height = draw_seek_bar();

        ImGui::PopStyleColor(5);
        return controls_height + seek_bar_height;

    }

    std::vector<std::string> get_device_info(const device& dev, bool include_location)
    {
        std::vector<std::string> res;
        for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
        {
            auto info = static_cast<rs2_camera_info>(i);

            // When camera is being reset, either because of "hardware reset"
            // or because of switch into advanced mode,
            // we don't want to capture the info that is about to change
            if ((info == RS2_CAMERA_INFO_PHYSICAL_PORT ||
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

    std::vector<std::pair<std::string, std::string>> get_devices_names(const device_list& list)
    {
        std::vector<std::pair<std::string, std::string>> device_names;

        for (uint32_t i = 0; i < list.size(); i++)
        {
            try
            {
                auto dev = list[i];
                device_names.push_back(get_device_name(dev));        // push name and sn to list
            }
            catch (...)
            {
                device_names.push_back(std::pair<std::string, std::string>(to_string() << "Unknown Device #" << i, ""));
            }
        }
        return device_names;
    }

    bool yes_no_dialog(const std::string& title, const std::string& do_what, bool& approved)
    {
        auto clicked = false;
        ImGui::OpenPopup(title.c_str());
        if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::string msg = to_string() << "\t\tAre you sure you want to " << do_what << "?\t\t\n";
            ImGui::Text("\n%s\n", msg.c_str());
            ImGui::Separator();
            auto width = ImGui::GetWindowWidth();
            ImGui::Dummy(ImVec2(width / 4.f, 0));
            ImGui::SameLine();
            if (ImGui::Button("Yes", ImVec2(60, 30)))
            {
                ImGui::CloseCurrentPopup();
                approved = true;
                clicked = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(60, 30)))
            {
                ImGui::CloseCurrentPopup();
                approved = false;
                clicked = true;
            }
            ImGui::NewLine();
            ImGui::EndPopup();
        }
        return clicked;
    }

    void device_model::draw_advanced_mode_tab(viewer_model& view)
    {
        using namespace rs400;
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 0.9f, 0.9f, 0.9f, 1 });

        auto is_advanced_mode = dev.is<advanced_mode>();
        if (ImGui::TreeNode("Advanced Mode"))
        {
            try
            {
                if (!is_advanced_mode)
                {
                    // TODO: Why are we showing the tab then??
                    ImGui::TextColored(redish, "Selected device does not offer\nany advanced settings");
                }
                else
                {
                    auto advanced = dev.as<advanced_mode>();
                    if (advanced.is_enabled())
                    {
                        static bool show_dialog = false;
                        static bool disable_approved = true;
                        if (allow_remove)
                        {
                            if (ImGui::Button("Disable Advanced Mode", ImVec2{ 226, 0 }))
                            {
                                show_dialog = true;
                                disable_approved = false;
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Disabling advanced mode will reset depth generation to factory settings\nThis will not affect calibration");
                            }

                            if (show_dialog &&
                                yes_no_dialog("Advanced Mode", "Disable Advanced Mode", disable_approved))
                            {
                                if (disable_approved)
                                {
                                    advanced.toggle_advanced_mode(false);
                                    restarting_device_info = get_device_info(dev, false);
                                    view.not_model.add_log("Switching out of advanced mode...");
                                }
                                show_dialog = false;
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Disabling advanced mode will reset depth generation to factory settings\nThis will not affect calibration");
                            }
                        }

                        draw_advanced_mode_controls(advanced, amc, get_curr_advanced_controls);
                    }
                    else
                    {
                        if (allow_remove)
                        {
                            static bool show_dialog = false;
                            static bool enable_approved = true;
                            if (ImGui::Button("Enable Advanced Mode", ImVec2{ 226, 0 }))
                            {
                                show_dialog = true;
                                enable_approved = false;
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Advanced mode is a persistent camera state unlocking calibration formats and depth generation controls\nYou can always reset the camera to factory defaults by disabling advanced mode");
                            }

                            if (show_dialog &&
                                yes_no_dialog("Advanced Mode", "Enable Advanced Mode", enable_approved))
                            {
                                if (enable_approved)
                                {
                                    advanced.toggle_advanced_mode(true);
                                    restarting_device_info = get_device_info(dev, false);
                                    view.not_model.add_log("Switching into advanced mode...");
                                }
                                show_dialog = false;
                            }
                        }

                        ImGui::TextColored(redish, "Device is not in advanced mode!\nTo access advanced functionality\nclick \"Enable Advanced Mode\"");
                    }
                }
            }
            catch (...)
            {
                ImGui::TextColored(redish, "Couldn't fetch Advanced Mode settings");
            }

            ImGui::TreePop();
        }

        ImGui::PopStyleColor();
    }

    void device_model::draw_controls(float panel_width, float panel_height,
        ux_window& window,
        std::string& error_message,
        device_model*& device_to_remove,
        viewer_model& viewer, float windows_width,
        bool update_read_only_options,
        std::vector<std::function<void()>>& draw_later)
    {
        auto header_h = panel_height;
        if (dev.is<playback>()) header_h += 15;

        ImGui::PushFont(window.get_font());
        auto pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(pos, { pos.x + panel_width, pos.y + header_h }, ImColor(sensor_header_light_blue));
        ImGui::GetWindowDrawList()->AddLine({ pos.x,pos.y }, { pos.x + panel_width,pos.y }, ImColor(black));

        pos = ImGui::GetCursorPos();
        ImGui::PushStyleColor(ImGuiCol_Button, sensor_header_light_blue);
        ImGui::SetCursorPos({ 8, pos.y + 17 });

        std::string label{ "" };
        if (is_recording)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, redish);
            label = to_string() << u8"\uf111";
            ImGui::Text("%s", label.c_str());
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Recording");
            }
        }
        else if (dev.is<playback>())
        {
            label = to_string() << u8" \uf008";
            ImGui::Text("%s", label.c_str());
        }
        else
        {
            label = to_string() << u8" \uf03d";
            ImGui::Text("%s", label.c_str());
        }
        ImGui::SameLine();

        label = to_string() << dev.get_info(RS2_CAMERA_INFO_NAME);
        ImGui::Text("%s", label.c_str());

        ImGui::Columns(1);
        ImGui::SetCursorPos({ panel_width - 50, pos.y + 8 + (header_h - panel_height) / 2 });

        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        ImGui::PushFont(window.get_large_font());
        label = to_string() << "device_menu" << id;
        std::string settings_button_name = to_string() << u8"\uf0c9##" << id;

        if (ImGui::Button(settings_button_name.c_str(), { 33,35 }))
            ImGui::OpenPopup(label.c_str());
        ImGui::PopFont();

        ImGui::PushFont(window.get_font());

        if (ImGui::BeginPopup(label.c_str()))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
            if (!show_device_info && ImGui::Selectable("Show Device Details..."))
            {
                show_device_info = true;
            }
            if (show_device_info && ImGui::Selectable("Hide Device Details..."))
            {
                show_device_info = false;
            }
            if (!is_recording && !dev.is<playback>())
            {
                bool is_device_streaming = std::any_of(subdevices.begin(), subdevices.end(), [](const std::shared_ptr<subdevice_model>& s) { return s->streaming; });
                if (ImGui::Selectable("Record to File...", false, is_device_streaming ? ImGuiSelectableFlags_Disabled : 0))
                {
                    auto ret = file_dialog_open(save_file, "ROS-bag\0*.bag\0", NULL, NULL);

                    if (ret)
                    {
                        std::string filename = ret;
                        if (!ends_with(to_lower(filename), ".bag")) filename += ".bag";

                        start_recording(filename, error_message);
                    }
                }
                if (is_device_streaming)
                {
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Stop streaming to enable recording");
                }
            }

            if (auto adv = dev.as<advanced_mode>())
            {
                try
                {
                    ImGui::Separator();

                    if (ImGui::Selectable("Load Settings", false, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        auto ret = file_dialog_open(open_file, "JavaScript Object Notation (JSON)\0*.json\0", NULL, NULL);
                        if (ret)
                        {
                            std::ifstream t(ret);
                            viewer.not_model.add_log(to_string() << "Loading settings from \"" << ret << "\"...");
                            std::string str((std::istreambuf_iterator<char>(t)),
                                std::istreambuf_iterator<char>());

                            adv.load_json(str);
                            get_curr_advanced_controls = true;
                        }
                    }

                    if (ImGui::Selectable("Save Settings", false, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        auto ret = file_dialog_open(save_file, "JavaScript Object Notation (JSON)\0*.json\0", NULL, NULL);

                        if (ret)
                        {
                            std::string filename = ret;
                            if (!ends_with(to_lower(filename), ".json")) filename += ".json";

                            viewer.not_model.add_log(to_string() << "Saving settings to \"" << filename << "\"...");
                            std::ofstream out(filename);
                            out << adv.serialize_json();
                            out.close();
                        }
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
            }

            if (allow_remove)
            {
                ImGui::Separator();

                if (ImGui::Selectable("Hardware Reset"))
                {
                    try
                    {
                        restarting_device_info = get_device_info(dev, false);
                        dev.hardware_reset();
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

                ImGui::Separator();

                if (ImGui::Selectable("Remove Source"))
                {
                    for (auto&& sub : subdevices)
                    {
                        if (sub->streaming)
                            sub->stop(viewer);
                    }
                    device_to_remove = this;
                }
            }

            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::SetCursorPos({ 33, pos.y + panel_height - 9 });
        ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0xc3, 0xd5, 0xe5, 0xff));

        int playback_control_panel_height = 0;
        if (auto p = dev.as<playback>())
        {
            auto full_path = p.file_name();
            auto filename = get_file_name(full_path);

            ImGui::Text("File: \"%s\"", filename.c_str());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", full_path.c_str());

            auto playback_panel_pos = ImVec2{ 0, pos.y + panel_height + 18 };
            ImGui::SetCursorPos(playback_panel_pos);
            playback_panel_pos.y = draw_playback_panel(window.get_font(), viewer);
            playback_control_panel_height += (int)playback_panel_pos.y;
        }

        ImGui::SetCursorPos({ 0, pos.y + header_h + playback_control_panel_height });
        pos = ImGui::GetCursorPos();

        int info_control_panel_height = 0;
        if (show_device_info)
        {
            int line_h = 22;
            info_control_panel_height = (int)infos.size() * line_h + 5;

            const ImVec2 abs_pos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(abs_pos,
            { abs_pos.x + panel_width, abs_pos.y + info_control_panel_height },
                ImColor(device_info_color));
            ImGui::GetWindowDrawList()->AddLine({ abs_pos.x, abs_pos.y - 1 },
            { abs_pos.x + panel_width, abs_pos.y - 1 },
                ImColor(black), 1.f);

            for (auto&& pair : infos)
            {
                auto rc = ImGui::GetCursorPos();
                ImGui::SetCursorPos({ rc.x + 12, rc.y + 4 });
                ImGui::Text("%s:", pair.first.c_str()); ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_FrameBg, device_info_color);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::SetCursorPos({ rc.x + 130, rc.y + 1 });
                label = to_string() << "##" << id << " " << pair.first;
                ImGui::InputText(label.c_str(),
                    (char*)pair.second.data(),
                    pair.second.size() + 1,
                    ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor(3);

                ImGui::SetCursorPos({ rc.x, rc.y + line_h });
            }
        }

        ImGui::SetCursorPos({ 0, pos.y + info_control_panel_height });
        ImGui::PopStyleColor(2);
        ImGui::PopFont();

        auto sensor_top_y = ImGui::GetCursorPosY();
        ImGui::SetContentRegionWidth(windows_width - 36);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0xc3, 0xd5, 0xe5, 0xff));
        ImGui::PushFont(window.get_font());

        // Draw menu foreach subdevice with its properties
        for (auto&& sub : subdevices)
        {
            if (show_depth_only && !sub->s->is<depth_sensor>()) continue;

            const ImVec2 pos = ImGui::GetCursorPos();
            const ImVec2 abs_pos = ImGui::GetCursorScreenPos();
            //model_to_y[sub.get()] = pos.y;
            //model_to_abs_y[sub.get()] = abs_pos.y;

            if (!show_depth_only)
            {
                draw_later.push_back([&error_message, windows_width, &window, sub, pos, &viewer, this]()
                {
                    bool stop_recording = false;

                    ImGui::SetCursorPos({ windows_width - 35, pos.y + 3 });
                    ImGui::PushFont(window.get_font());

                    ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                    if (!sub->streaming)
                    {
                        std::string label = to_string() << u8"  \uf204\noff   ##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME);

                        ImGui::PushStyleColor(ImGuiCol_Text, redish);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                        if (sub->is_selected_combination_supported())
                        {
                            if (ImGui::Button(label.c_str(), { 30,30 }))
                            {
                                auto profiles = sub->get_selected_profiles();
                                try
                                {
                                    sub->play(profiles, viewer);
                                }
                                catch (const error& e)
                                {
                                    error_message = error_to_string(e);
                                }
                                catch (const std::exception& e)
                                {
                                    error_message = e.what();
                                }

                                for (auto&& profile : profiles)
                                {
                                    viewer.begin_stream(sub, profile);
                                }
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Start streaming data from this sensor");
                            }
                        }
                        else
                        {
                            ImGui::TextDisabled(u8"  \uf204\noff   ");
                            if (std::any_of(sub->stream_enabled.begin(), sub->stream_enabled.end(), [](std::pair<int, bool> const& s) { return s.second; }))
                            {
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Selected configuration (FPS, Resolution) is not supported");
                                }
                            }
                            else
                            {
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("No stream selected");
                                }
                            }

                        }
                    }
                    else
                    {
                        std::string label = to_string() << u8"  \uf205\n    on##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME);
                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                        if (ImGui::Button(label.c_str(), { 30,30 }))
                        {
                            sub->stop(viewer);

                            if (!std::any_of(subdevices.begin(), subdevices.end(),
                                [](const std::shared_ptr<subdevice_model>& sm)
                            {
                                return sm->streaming;
                            }))
                            {
                                stop_recording = true;
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Stop streaming data from selected sub-device");
                        }
                    }

                    ImGui::PopStyleColor(5);
                    ImGui::PopFont();

                    if (is_recording && stop_recording)
                    {
                        this->stop_recording();
                        for (auto&& sub : subdevices)
                        {
                            //TODO: Fix case where sensor X recorded stream 0, then stopped, and then started recording stream 1 (need 2 sensors for this to happen)
                            if (sub->is_selected_combination_supported())
                            {
                                auto profiles = sub->get_selected_profiles();
                                for (auto&& profile : profiles)
                                {
                                    viewer.streams[profile.unique_id()].dev = sub;
                                }
                            }
                        }
                    }
                });
            }

            ImGui::GetWindowDrawList()->AddLine({ abs_pos.x, abs_pos.y - 1 },
            { abs_pos.x + panel_width, abs_pos.y - 1 },
                ImColor(black), 1.f);

            label = to_string() << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "##" << id;
            ImGui::PushStyleColor(ImGuiCol_Header, sensor_header_light_blue);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 10 });
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
            ImGuiTreeNodeFlags flags{};
            if (show_depth_only) flags = ImGuiTreeNodeFlags_DefaultOpen;
            if (ImGui::TreeNodeEx(label.c_str(), flags))
            {
                ImGui::PopStyleVar();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, 2 });

                if (show_stream_selection)
                    sub->draw_stream_selection();

                static const std::vector<rs2_option> drawing_order{
                    RS2_OPTION_VISUAL_PRESET,
                    RS2_OPTION_EMITTER_ENABLED,
                    RS2_OPTION_ENABLE_AUTO_EXPOSURE };

                for (auto& opt : drawing_order)
                {
                    if (sub->draw_option(opt, dev.is<playback>() || update_read_only_options, error_message, viewer.not_model))
                    {
                        get_curr_advanced_controls = true;
                    }
                }

                label = to_string() << "Controls ##" << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << id;;
                if (ImGui::TreeNode(label.c_str()))
                {
                    for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                    {
                        auto opt = static_cast<rs2_option>(i);
                        if (opt == RS2_OPTION_FRAMES_QUEUE_SIZE) continue;
                        if (std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
                        {
                            if (sub->draw_option(opt, dev.is<playback>() || update_read_only_options, error_message, viewer.not_model))
                            {
                                get_curr_advanced_controls = true;
                            }
                        }
                    }

                    ImGui::TreePop();
                }

                if (dev.is<advanced_mode>() && sub->s->is<depth_sensor>())
                    draw_advanced_mode_tab(viewer);

                for (auto&& pb : sub->const_effects)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                    label = to_string() << pb->get_name() << "##" << id;
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                        {
                            auto opt = static_cast<rs2_option>(i);
                            if (opt == RS2_OPTION_FRAMES_QUEUE_SIZE) continue;
                            pb->get_option(opt).draw_option(
                                dev.is<playback>() || update_read_only_options,
                                false, error_message, viewer.not_model);
                        }

                        ImGui::TreePop();
                    }
                }

                if (sub->post_processing.size() > 0)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    const ImVec2 pos = ImGui::GetCursorPos();
                    const ImVec2 abs_pos = ImGui::GetCursorScreenPos();

                    draw_later.push_back([windows_width, &window, sub, pos, &viewer, this]() {
                        if (sub->streaming) ImGui::SetCursorPos({ windows_width - 35, pos.y - 1 });
                        else ImGui::SetCursorPos({ windows_width - 27, pos.y + 2 });
                        ImGui::PushFont(window.get_font());

                        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                        if (!sub->streaming)
                        {
                            if (!sub->post_processing_enabled)
                            {
                                std::string label = to_string() << u8"\uf204";

                                ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);
                                ImGui::TextDisabled("%s", label.c_str());
                            }
                            else
                            {
                                std::string label = to_string() << u8"\uf205";
                                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);
                                ImGui::TextDisabled("%s", label.c_str());
                            }
                        }
                        else
                        {
                            if (!sub->post_processing_enabled)
                            {
                                std::string label = to_string() << u8" \uf204##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << ",post";

                                ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                                if (ImGui::Button(label.c_str(), { 30,24 }))
                                {
                                    sub->post_processing_enabled = true;
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Enable post-processing filters");
                                }
                            }
                            else
                            {
                                std::string label = to_string() << u8" \uf205##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << ",post";
                                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                                if (ImGui::Button(label.c_str(), { 30,24 }))
                                {
                                    sub->post_processing_enabled = false;
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Disable post-processing filters");
                                }
                            }
                        }

                        ImGui::PopStyleColor(5);
                        ImGui::PopFont();
                    });

                    label = to_string() << "Post-Processing##" << id;
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        for (auto&& pb : sub->post_processing)
                        {
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                            const ImVec2 pos = ImGui::GetCursorPos();
                            const ImVec2 abs_pos = ImGui::GetCursorScreenPos();

                            draw_later.push_back([windows_width, &window, sub, pos, &viewer, this, pb]() {
                                if (!sub->streaming || !sub->post_processing_enabled) ImGui::SetCursorPos({ windows_width - 27, pos.y + 3 });
                                else ImGui::SetCursorPos({ windows_width - 35, pos.y - 1 });
                                ImGui::PushFont(window.get_font());

                                ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                                if (!sub->streaming || !sub->post_processing_enabled)
                                {
                                    if (!pb->enabled)
                                    {
                                        std::string label = to_string() << u8"\uf204";

                                        ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);
                                        ImGui::TextDisabled("%s", label.c_str());
                                    }
                                    else
                                    {
                                        std::string label = to_string() << u8"\uf205";
                                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);
                                        ImGui::TextDisabled("%s", label.c_str());
                                    }
                                }
                                else
                                {
                                    if (!pb->enabled)
                                    {
                                        std::string label = to_string() << u8" \uf204##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();

                                        ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                                        if (ImGui::Button(label.c_str(), { 30,24 }))
                                        {
                                            pb->enabled = true;
                                        }
                                        if (ImGui::IsItemHovered())
                                        {
                                            label = to_string() << "Enable " << pb->get_name() << " post-processing filter";
                                            ImGui::SetTooltip("%s", label.c_str());
                                        }
                                    }
                                    else
                                    {
                                        std::string label = to_string() << u8" \uf205##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();
                                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                                        if (ImGui::Button(label.c_str(), { 30,24 }))
                                        {
                                            pb->enabled = false;
                                        }
                                        if (ImGui::IsItemHovered())
                                        {
                                            label = to_string() << "Disable " << pb->get_name() << " post-processing filter";
                                            ImGui::SetTooltip("%s", label.c_str());
                                        }
                                    }
                                }

                                ImGui::PopStyleColor(5);
                                ImGui::PopFont();
                            });

                            label = to_string() << pb->get_name() << "##" << id;
                            if (ImGui::TreeNode(label.c_str()))
                            {
                                for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                                {
                                    auto opt = static_cast<rs2_option>(i);
                                    if (opt == RS2_OPTION_FRAMES_QUEUE_SIZE) continue;
                                    pb->get_option(opt).draw_option(
                                        dev.is<playback>() || update_read_only_options,
                                        false, error_message, viewer.not_model);
                                }

                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        }

        for (auto&& sub : subdevices)
        {
            sub->update(error_message, viewer.not_model);
        }

        ImGui::PopStyleColor(2);
        ImGui::PopFont();
    }

    void viewer_model::draw_viewport(const rect& viewer_rect, ux_window& window, int devices, std::string& error_message, texture_buffer* texture, points points)
    {
        if (!is_3d_view)
        {
            render_2d_view(viewer_rect, window,
                get_output_height(), window.get_font(), window.get_large_font(),
                devices, window.get_mouse(), error_message);
        }
        else
        {
            if (paused)
                show_paused_icon(window.get_large_font(), panel_width + 15, panel_y + 15 + 32, 0);

            show_3dviewer_header(window.get_font(), viewer_rect, paused);

            update_3d_camera(viewer_rect, window.get_mouse());

            rect window_size{ 0, 0, (float)window.width(), (float)window.height() };
            rect fb_size{ 0, 0, (float)window.framebuf_width(), (float)window.framebuf_height() };
            rect new_rect = viewer_rect.normalize(window_size).unnormalize(fb_size);

            render_3d_view(new_rect, texture, points);
        }

        if (ImGui::IsKeyPressed(' '))
        {
            if (paused)
            {
                for (auto&& s : streams)
                {
                    if (s.second.dev)
                    {
                        s.second.dev->resume();
                        if (s.second.dev->dev.is<playback>())
                        {
                            auto p = s.second.dev->dev.as<playback>();
                            p.resume();
                        }
                    }
                }
            }
            else
            {
                for (auto&& s : streams)
                {
                    if (s.second.dev)
                    {
                        s.second.dev->pause();
                        if (s.second.dev->dev.is<playback>())
                        {
                            auto p = s.second.dev->dev.as<playback>();
                            p.pause();
                        }
                    }
                }
            }
            paused = !paused;
        }
    }

    std::map<int, rect> viewer_model::get_interpolated_layout(const std::map<int, rect>& l)
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

        std::map<int, rect> results;
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

    notification_data::notification_data(std::string description,
        double timestamp,
        rs2_log_severity severity,
        rs2_notification_category category)
        : _description(description),
        _timestamp(timestamp),
        _severity(severity),
        _category(category) {}


    rs2_notification_category notification_data::get_category() const
    {
        return _category;
    }

    std::string notification_data::get_description() const
    {
        return _description;
    }


    double notification_data::get_timestamp() const
    {
        return _timestamp;
    }


    rs2_log_severity notification_data::get_severity() const
    {
        return _severity;
    }

    notification_model::notification_model()
    {
        message = "";
    }

    notification_model::notification_model(const notification_data& n)
    {
        message = n.get_description();
        timestamp = n.get_timestamp();
        severity = n.get_severity();
        created_time = std::chrono::high_resolution_clock::now();
    }

    double notification_model::get_age_in_ms() const
    {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - created_time).count();
    }

    void notification_model::set_color_scheme(float t) const
    {
        if (severity == RS2_LOG_SEVERITY_ERROR ||
            severity == RS2_LOG_SEVERITY_WARN)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.f, 0.f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.5f, 0.2f, 0.2f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.6f, 0.2f, 0.2f, 1 - t });
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.3f, 0.3f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.4f, 0.4f, 0.4f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.6f, 0.6f, 0.6f, 1 - t });
        }
    }

    void notification_model::draw(int w, int y, notification_model& selected)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        auto ms = get_age_in_ms() / MAX_LIFETIME_MS;
        auto t = smoothstep(static_cast<float>(ms), 0.7f, 1.0f);

        set_color_scheme(t);
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 - t });

        auto lines = static_cast<int>(std::count(message.begin(), message.end(), '\n') + 1);
        ImGui::SetNextWindowPos({ float(w - 330), float(y) });
        height = lines * 30 + 20;
        ImGui::SetNextWindowSize({ float(315), float(height) });
        std::string label = to_string() << "Hardware Notification #" << index;
        ImGui::Begin(label.c_str(), nullptr, flags);

        ImGui::Text("%s", message.c_str());

        if (lines == 1)
            ImGui::SameLine();

        ImGui::Text("(...)");

        if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered())
        {
            selected = *this;
        }

        ImGui::End();

        ImGui::PopStyleVar();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    void notifications_model::add_notification(const notification_data& n)
    {
        {
            std::lock_guard<std::mutex> lock(m); // need to protect the pending_notifications queue because the insertion of notifications
                                                 // done from the notifications callback and proccesing and removing of old notifications done from the main thread

            notification_model m(n);
            m.index = index++;
            m.timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
            pending_notifications.push_back(m);

            if (pending_notifications.size() > MAX_SIZE)
                pending_notifications.erase(pending_notifications.begin());
        }

        add_log(n.get_description());
    }

    void notifications_model::draw(ImFont* font, int w, int h)
    {
        ImGui::PushFont(font);
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
            auto height = 55;
            for (auto& noti : pending_notifications)
            {
                noti.draw(w, height, selected);
                height += noti.height + 4;
                idx++;
            }
        }


        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
        ImGui::Begin("Notification parent window", nullptr, flags);

        selected.set_color_scheme(0.f);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        if (selected.message != "")
            ImGui::OpenPopup("Notification from Hardware");
        if (ImGui::BeginPopupModal("Notification from Hardware", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Received the following notification:");
            std::stringstream ss;
            ss << "Timestamp: "
                << std::fixed << selected.timestamp
                << "\nSeverity: " << selected.severity
                << "\nDescription: " << selected.message;
            auto s = ss.str();
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            ImGui::InputTextMultiline("notification", const_cast<char*>(s.c_str()),
                s.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                selected.message = "";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(6);

        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    device_changes::device_changes(rs2::context& ctx)
    {
        _changes.emplace(rs2::device_list{}, ctx.query_devices());
        ctx.set_devices_changed_callback([&](event_information& info)
        {
            add_changes(info);
        });
    }

    void device_changes::add_changes(const event_information& c)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _changes.push(c);
    }

    bool device_changes::try_get_next_changes(event_information& removed_and_connected)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_changes.empty())
            return false;

        removed_and_connected = std::move(_changes.front());
        _changes.pop();
        return true;
    }

}
