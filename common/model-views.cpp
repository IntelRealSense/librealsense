// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <thread>
#include <algorithm>
#include <regex>
#include <cmath>
#include <librealsense2/rs_advanced_mode.hpp>
#include <librealsense2/rsutil.h>

#include "model-views.h"
#include <imgui_internal.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#include <noc_file_dialog.h>

#define ARCBALL_CAMERA_IMPLEMENTATION
#include <arcball_camera.h>

constexpr const char* recommended_fw_url = "https://downloadcenter.intel.com/download/27522/Latest-Firmware-for-Intel-RealSense-D400-Product-Family?v=t";

using namespace rs400;
using namespace nlohmann;

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

void open_url(const char* url)
{
#if (defined(_WIN32) || defined(_WIN64))
    if (reinterpret_cast<int>(ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOW)) < 32)
        throw std::runtime_error("Failed opening URL");
#elif defined __linux__ || defined(__linux__)
    std::string command_name = "xdg-open ";
    std::string command = command_name + url;
    if (system(command.c_str()))
        throw std::runtime_error("Failed opening URL");
#elif __APPLE__
    std::string command_name = "open ";
    std::string command = command_name + url;
    if (system(command.c_str()))
        throw std::runtime_error("Failed opening URL");
#else
#pragma message ( "\nLibrealsense couldn't establish OS/Build environment. \
Some auxillary functionalities might be affected. Please report this message if encountered")
#endif
}

std::vector<std::string> split_string(std::string& input, char delim)
{
    std::vector<std::string> result;
    auto e = input.end();
    auto i = input.begin();
    while (i != e) {
        i = find_if_not(i, e, [delim](char c) { return c == delim; });
        if (i == e) break;
        auto j = find(i, e, delim);
        result.emplace_back(i, j);
        i = j;
    }
    return result;
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

    void export_to_ply(const std::string& fname, notifications_model& ns, frameset frames, video_frame texture, bool notify)
    {
        std::thread([&ns, frames, texture, fname, notify]() mutable {

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
                if (notify) ns.add_notification({ to_string() << "Finished saving 3D view " << (texture ? "to " : "without texture to ") << fname,
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
        return stbi_write_png(filename, (int)pixel_width, (int)pixels_height, (int)bytes_per_pixel, raster_data, (int)stride_bytes);
    }

    bool save_frame_raw_data(const std::string& filename, rs2::frame frame)
    {
        bool ret = false;
        auto image = frame.as<video_frame>();
        if (image)
        {
            std::ofstream outfile(filename.data(), std::ofstream::binary);
            outfile.write(static_cast<const char*>(image.get_data()), image.get_height()*image.get_stride_in_bytes());

            outfile.close();
            ret = true;
        }

        return ret;
    }

    bool frame_metadata_to_csv(const std::string& filename, rs2::frame frame)
    {
        bool ret = false;
        auto image = frame.as<video_frame>();
        if (image)
        {
            std::ofstream csv;
            csv.open(filename);

            auto profile = image.get_profile();
            csv << "Frame Info: " << std::endl << "Type," << profile.stream_name() << std::endl;
            csv << "Format," << rs2_format_to_string(profile.format()) << std::endl;
            csv << "Frame Number," << image.get_frame_number() << std::endl;
            csv << "Timestamp (ms)," << std::fixed << std::setprecision(2) << image.get_timestamp() << std::endl;
            csv << "Resolution x," << (int)image.get_width() << std::endl;
            csv << "Resolution y," << (int)image.get_height() << std::endl;
            csv << "Bytes per pixel," << (int)image.get_bytes_per_pixel() << std::endl;

            if (auto vsp = profile.as<video_stream_profile>())
            {
                csv << std::endl << "Intrinsic:," << std::fixed << std::setprecision(6) <<std::endl;
                csv << "Fx," << vsp.get_intrinsics().fx << std::endl;
                csv << "Fy," << vsp.get_intrinsics().fy << std::endl;
                csv << "PPx,"<< vsp.get_intrinsics().ppx << std::endl;
                csv << "PPy,"<< vsp.get_intrinsics().ppy << std::endl;
                csv << "Distorsion," <<rs2_distortion_to_string(vsp.get_intrinsics().model) << std::endl;
            }

            csv.close();
            ret = true;
        }

        return ret;
    }

    std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec)
    {
        std::vector<const char*> res;
        for (auto&& s : vec) res.push_back(s.c_str());
        return res;
    }

    inline std::string get_event_type(const std::string& data)
    {
        std::regex event_type(R"REGX("Event Type"\s*:\s*"([^"]+)")REGX");
        std::smatch m;
        if (std::regex_search(data, m, event_type))
        {
            return m[1];
        }
        throw std::runtime_error(std::string("Failed to match Event Type in string: ") + data);
    }

    inline std::string get_subtype(const std::string& data)
    {
        std::regex subtype(R"REGX("Sub Type"\s*:\s*"([^"]+)")REGX");
        std::smatch m;
        if (std::regex_search(data, m, subtype))
        {
            return m[1];
        }
        throw std::runtime_error(std::string("Failed to match Sub Type in string: ") + data);
    }

    inline int get_id(const std::string& data)
    {
        std::regex id_regex(R"REGX("ID" : (\d+))REGX");
        std::smatch match;
        if (std::regex_search(data, match, id_regex))
        {
            return std::stoi(match[1].str());
        }
        throw std::runtime_error(std::string("Failed to match ID in string: ") + data);
    }

    inline std::array<uint8_t, 6> get_mac(const std::string& data)
    {
        std::regex mac_addr_regex(R"REGX("MAC" : \[(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\])REGX");
        std::smatch match;

        std::array<uint8_t, 6> mac_addr;
        if (std::regex_search(data, match, mac_addr_regex))
        {
            for (size_t i = 1; i < match.size(); i++)
            {
                mac_addr[i - 1] = static_cast<uint8_t>(std::stol(match[i].str()));
            }
            return mac_addr;
        }
        throw std::runtime_error(std::string("Failed to match MAC in string: ") + data);
    }

    bool option_model::draw(std::string& error_message, notifications_model& model, bool new_line, bool use_option_name)
    {
        auto res = false;
        if (supported)
        {
            // The option's rendering model supports an alternative option title derived from its description rather than name.
            // This is applied to the Holes Filling as its display must conform with the names used by a 3rd-party tools for consistency.
            if (opt == RS2_OPTION_HOLES_FILL)
                use_option_name = false;

            auto desc = endpoint->get_option_description(opt);

            // remain option to append to the current line
            if (!new_line)
                ImGui::SameLine();

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
                    ImGui::SetCursorPosX(read_only ? 268.f : 245.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, grey);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, grey);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 1.f,1.f,1.f,0.f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.f,1.f,1.f,0.f });
                    ImGui::PushStyleColor(ImGuiCol_Button, { 1.f,1.f,1.f,0.f });
                    ImGui::Button(textual_icons::question_mark, { 20, 20 });
                    ImGui::PopStyleColor(5);
                    if (ImGui::IsItemHovered() && desc)
                    {
                        ImGui::SetTooltip("%s", desc);
                    }

                    if (!read_only)
                    {
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(268);
                        if (!edit_mode)
                        {
                            std::string edit_id = to_string() << textual_icons::edit << "##" << id;
                            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.f,1.f,1.f,0.f });
                            ImGui::PushStyleColor(ImGuiCol_Button, { 1.f,1.f,1.f,0.f });
                            if (ImGui::Button(edit_id.c_str(), { 20, 20 }))
                            {
                                if (is_all_integers())
                                    edit_value = to_string() << (int)value;
                                else
                                    edit_value = to_string() << value;
                                edit_mode = true;
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Enter text-edit mode");
                            }
                            ImGui::PopStyleColor(4);
                        }
                        else
                        {
                            std::string edit_id = to_string() << textual_icons::edit << "##" << id;
                            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.f,1.f,1.f,0.f });
                            ImGui::PushStyleColor(ImGuiCol_Button, { 1.f,1.f,1.f,0.f });
                            if (ImGui::Button(edit_id.c_str(), { 20, 20 }))
                            {
                                edit_mode = false;
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Exit text-edit mode");
                            }
                            ImGui::PopStyleColor(4);
                        }
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
                                float dummy = std::floor(value);
                                ImGui::DragFloat(id.c_str(), &dummy, 1, 0, 0, text.c_str());
                                ImGui::PopStyleColor(2);
                            }
                        }
                        else if (edit_mode)
                        {
                            char buff[TEXT_BUFF_SIZE];
                            memset(buff, 0, TEXT_BUFF_SIZE);
                            strcpy(buff, edit_value.c_str());
                            if (ImGui::InputText(id.c_str(), buff, TEXT_BUFF_SIZE,
                                ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                float new_value;
                                if (!string_to_int(buff, new_value))
                                {
                                    error_message = "Invalid numeric input!";
                                }
                                else if (new_value < range.min || new_value > range.max)
                                {
                                    error_message = to_string() << new_value
                                        << " is out of bounds [" << range.min << ", "
                                        << range.max << "]";
                                }
                                else
                                {
                                    try
                                    {
                                        endpoint->set_option(opt, new_value);
                                        value = new_value;
                                    }
                                    catch (const error& e)
                                    {
                                        error_message = error_to_string(e);
                                    }
                                }

                                edit_mode = false;
                            }
                            edit_value = buff;
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
                                auto loffset = std::abs(fmod(value, range.step));
                                auto roffset = range.step - loffset;
                                if (value >= 0)
                                    value = (loffset < roffset) ? value - loffset : value + roffset;
                                else
                                    value = (loffset < roffset) ? value + loffset : value - roffset; 
                                value = (value < range.min) ? range.min : value;
                                value = (value > range.max) ? range.max : value;
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
                    std::string txt = to_string() << (use_option_name ? rs2_option_to_string(opt) : desc) << ":";

                    auto pos_x = ImGui::GetCursorPosX();

                    ImGui::Text("%s", txt.c_str());
                    if (ImGui::IsItemHovered() && desc)
                    {
                        ImGui::SetTooltip("%s", desc);
                    }

                    ImGui::SameLine();
                    if (new_line)
                        ImGui::SetCursorPosX(pos_x + 135);

                    ImGui::PushItemWidth(new_line? -1.f:100.f);

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
                            if (invalidate_flag) *invalidate_flag = true;
                            res = true;
                        }
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }

                    ImGui::PopStyleColor();

                    ImGui::PopItemWidth();
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
        const std::string& opt_base_label,
        subdevice_model* model,
        std::shared_ptr<options> options,
        bool* options_invalidated,
        std::string& error_message)
    {
        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            option_model metadata;
            auto opt = static_cast<rs2_option>(i);

            std::stringstream ss;
            ss << opt_base_label << "/" << rs2_option_to_string(opt);
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


    processing_block_model::processing_block_model(
        subdevice_model* owner,
        const std::string& name,
        std::shared_ptr<options> block,
        std::function<rs2::frame(rs2::frame)> invoker,
        std::string& error_message)
        : _name(name), _block(block), _invoker(invoker)
    {
        std::stringstream ss;
        ss << "##" << ((owner) ? owner->dev.get_info(RS2_CAMERA_INFO_NAME) : _name)
            << "/" << ((owner) ? (*owner->s).get_info(RS2_CAMERA_INFO_NAME) : "_");

        subdevice_model::populate_options(options_metadata,
            ss.str().c_str(),owner , block, owner ? &owner->options_invalidated : nullptr, error_message);
    }

    subdevice_model::subdevice_model(
        device& dev,
        std::shared_ptr<sensor> s, std::string& error_message)
        : s(s), dev(dev), ui(), last_valid_ui(),
        streaming(false), _pause(false),
        depth_colorizer(std::make_shared<rs2::colorizer>()),
        decimation_filter(),
        spatial_filter(),
        temporal_filter(),
        hole_filling_filter(),
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
                [=](rs2::frame f) { return decimate->process(f); },
                error_message);
            decimation_filter->enabled = true;
            post_processing.push_back(decimation_filter);

            auto depth_2_disparity = std::make_shared<rs2::disparity_transform>();
            depth_to_disparity = std::make_shared<processing_block_model>(
                this, "Depth->Disparity", depth_2_disparity,
                [=](rs2::frame f) { return depth_2_disparity->process(f); }, error_message);
            if (s->is<depth_stereo_sensor>())
            {
                depth_to_disparity->enabled = true;
                post_processing.push_back(depth_to_disparity);
            }

            auto spatial = std::make_shared<rs2::spatial_filter>();
            spatial_filter = std::make_shared<processing_block_model>(
                this, "Spatial Filter", spatial,
                [=](rs2::frame f) { return spatial->process(f); },
                error_message);
            spatial_filter->enabled = true;
            post_processing.push_back(spatial_filter);

            auto temporal = std::make_shared<rs2::temporal_filter>();
            temporal_filter = std::make_shared<processing_block_model>(
                this, "Temporal Filter", temporal,
                [=](rs2::frame f) { return temporal->process(f); }, error_message);
            temporal_filter->enabled = true;
            post_processing.push_back(temporal_filter);

            auto hole_filling = std::make_shared<rs2::hole_filling_filter>();
            hole_filling_filter = std::make_shared<processing_block_model>(
                this, "Hole Filling Filter", hole_filling,
                [=](rs2::frame f) { return hole_filling->process(f); }, error_message);
            hole_filling_filter->enabled = false;
            post_processing.push_back(hole_filling_filter);

            auto disparity_2_depth = std::make_shared<rs2::disparity_transform>(false);
            disparity_to_depth = std::make_shared<processing_block_model>(
                this, "Disparity->Depth", disparity_2_depth,
                [=](rs2::frame f) { return disparity_2_depth->process(f); }, error_message);
            disparity_to_depth->enabled = s->is<depth_stereo_sensor>();
            if (s->is<depth_stereo_sensor>())
            {
                disparity_to_depth->enabled = true;
                // the block will be internally available, but removed from UI
                //post_processing.push_back(disparity_to_depth);
            }
        }
        else
        {
            rs2_error* e = nullptr;
            if (rs2_is_sensor_extendable_to(s->get().get(), RS2_EXTENSION_VIDEO, &e) && !e)
            {
                auto decimate = std::make_shared<rs2::decimation_filter>();
                decimation_filter = std::make_shared<processing_block_model>(
                    this, "Decimation Filter", decimate,
                    [=](rs2::frame f) { return decimate->process(f); },
                    error_message);
                decimation_filter->enabled = true;
                post_processing.push_back(decimation_filter);
            }
        }

        std::stringstream ss;
        ss << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
            << "/" << s->get_info(RS2_CAMERA_INFO_NAME);
        populate_options(options_metadata, ss.str().c_str(), this, s, &options_invalidated, error_message);

        try
        {
            auto sensor_profiles = s->get_stream_profiles();
            reverse(begin(sensor_profiles), end(sensor_profiles));
            rs2_format def_format{ RS2_FORMAT_ANY };
            for (auto&& profile : sensor_profiles)
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

                if (profile.is_default())
                {
                    stream_enabled[profile.unique_id()] = true;
                    def_format = profile.format();
                }

                profiles.push_back(profile);
            }
            auto any_stream_enabled = std::any_of(std::begin(stream_enabled), std::end(stream_enabled), [](const std::pair<int, bool>& p) { return p.second; });
            if (!any_stream_enabled)
            {
                if(sensor_profiles.size() > 0)
                    stream_enabled[sensor_profiles.rbegin()->unique_id()] = true;
            }

            for (auto&& fps_list : fps_values_per_stream)
            {
                sort_together(fps_list.second, fpses_per_stream[fps_list.first]);
            }
            sort_together(shared_fps_values, shared_fpses);
            sort_together(res_values, resolutions);

            show_single_fps_list = is_there_common_fps();

            // set default selections. USB2 configuration requires low-res resolution/fps.
            int selection_index{};
            bool usb2 = false;
            if (dev.supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
            {
                std::string dev_usb_type(dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR));
                usb2 = (std::string::npos != dev_usb_type.find("2."));
            }

            int fps_constrain = usb2 ? 15 : 30;
            auto resolution_constrain = usb2 ? std::make_pair(640, 480) :std::make_pair(1280, 720);

            if (!show_single_fps_list)
            {
                for (auto fps_array : fps_values_per_stream)
                {
                    if (get_default_selection_index(fps_array.second, fps_constrain, &selection_index))
                    {
                        ui.selected_fps_id[fps_array.first] = selection_index;
                        break;
                    }
                }
            }
            else
            {
                if (get_default_selection_index(shared_fps_values, fps_constrain, &selection_index))
                    ui.selected_shared_fps_id = selection_index;
            }

            for (auto format_array : format_values)
            {
                if (get_default_selection_index(format_array.second, def_format, &selection_index))
                {
                    ui.selected_format_id[format_array.first] = selection_index;
                    break;
                }
            }

            get_default_selection_index(res_values, resolution_constrain, &selection_index);
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
        auto col1 = 155.f;

        // Draw combo-box with all resolution options for this device
        auto res_chars = get_string_pointers(resolutions);
        if (res_chars.size() > 0)
        {
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
        }

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
                    if (show_single_fps_list)
                    {
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(col1);
                    }

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

                        label = to_string() << "##" << s->get_info(RS2_CAMERA_INFO_NAME)
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
            if (stream_enabled[stream])
            {
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
                        if (res_values.size() > 0)
                        {
                            auto width = res_values[ui.selected_res_id].first;
                            auto height = res_values[ui.selected_res_id].second;

                            if (vid_prof.width() == width &&
                                vid_prof.height() == height &&
                                p.unique_id() == stream &&
                                p.fps() == fps &&
                                p.format() == format)
                                results.push_back(p);
                        }
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

        // Verify that the number of found matches corrseponds to the number of the requested streams
        // TODO - review whether the comparison can be made strict (==)
        return results.size() >= size_t(std::count_if(stream_enabled.begin(), stream_enabled.end(), [](const std::pair<int, bool>& kpv)-> bool { return kpv.second == true; }));
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
                auto format = format_values[stream][ui.selected_format_id[stream]];

                auto fps = 0;
                if (show_single_fps_list)
                    fps = shared_fps_values[ui.selected_shared_fps_id];
                else
                    fps = fps_values_per_stream[stream][ui.selected_fps_id[stream]];

                for (auto&& p : profiles)
                {
                    if (auto vid_prof = p.as<video_stream_profile>())
                    {
                        if (res_values.size() > 0)
                        {
                            auto width = res_values[ui.selected_res_id].first;
                            auto height = res_values[ui.selected_res_id].second;

                            error_message << "\n{" << stream_display_names[stream] << ","
                                << width << "x" << height << " at " << fps << "Hz, "
                                << rs2_format_to_string(format) << "} ";

                            if (vid_prof.width() == width &&
                                vid_prof.height() == height &&
                                p.unique_id() == stream &&
                                p.fps() == fps &&
                                p.format() == format)
                                results.push_back(p);
                        }
                    }
                    else
                    {
                        error_message << "\n{" << stream_display_names[stream] << ", at " << fps << "Hz, "
                            << rs2_format_to_string(format) << "} ";

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

    uint64_t subdevice_model::num_supported_non_default_options() const
    {
        return (uint64_t)std::count_if(
            std::begin(options_metadata),
            std::end(options_metadata),
            [](const std::pair<int, option_model>& p) {return p.second.supported && p.second.opt != RS2_OPTION_FRAMES_QUEUE_SIZE; });
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
        if (custom_draw_method)
            return custom_draw_method(*this, error_message, model);
        else
            return draw(error_message, model);
    }

    stream_model::stream_model()
        : texture(std::unique_ptr<texture_buffer>(new texture_buffer())),
        _stream_not_alive(std::chrono::milliseconds(1500))
    {}

    texture_buffer* stream_model::upload_frame(frame&& f)
    {
        if (dev && dev->is_paused() && !dev->dev.is<playback>()) return nullptr;

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

        glLineWidth((GLfloat)line_width);

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

            original_size = {
                static_cast<float>(vd.width()),
                static_cast<float>(vd.height()) };
        }
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
                    r = r.normalize(stream_rect).unnormalize(_normalized_zoom.unnormalize(get_original_stream_bounds()));
                    dev->roi_rect = r; // Store new rect in device coordinates into the subdevice object

                    // Send it to firmware:
                    // Step 1: get rid of negative width / height
                    region_of_interest roi{};
                    roi.min_x = static_cast<int>(std::min(r.x, r.x + r.w));
                    roi.max_x = static_cast<int>(std::max(r.x, r.x + r.w));
                    roi.min_y = static_cast<int>(std::min(r.y, r.y + r.h));
                    roi.max_y = static_cast<int>(std::max(r.y, r.y + r.h));

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

                        // Default ROI behavior is center 3/4 of the screen:
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
                r = r.normalize(_normalized_zoom.unnormalize(get_original_stream_bounds())).unnormalize(stream_rect).cut_by(stream_rect);
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

    void viewer_model::show_3dviewer_header(ImFont* font, rs2::rect stream_rect, bool& paused, std::string& error_message)
    {
        int combo_boxes = 0;
        const float combo_box_width = 200;

        // Initialize and prepare depth and texture sources
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
        if (depth_sources_str.size() > 0 && allow_3d_source_change) combo_boxes++;

        int selected_tex_source = 0;
        std::vector<std::string> tex_sources_str;
        std::vector<int> tex_sources;
        i = 0;
        for (auto&& s : streams)
        {
            if (s.second.is_stream_visible() &&
                s.second.texture->get_last_frame() &&
                (s.second.profile.stream_type() == RS2_STREAM_COLOR ||
                 s.second.profile.stream_type() == RS2_STREAM_INFRARED ||
                 s.second.profile.stream_type() == RS2_STREAM_DEPTH ||
                 s.second.profile.stream_type() == RS2_STREAM_FISHEYE))
            {
                if (selected_tex_source_uid == -1 && selected_depth_source_uid != -1)
                {
                    selected_tex_source_uid = streams_origin[s.second.profile.unique_id()];
                }
                if (streams_origin[s.second.profile.unique_id()] == selected_tex_source_uid)
                {
                    selected_tex_source = i;
                }

                // The texture source shall always refer to the raw (original) streams
                tex_sources.push_back(streams_origin[s.second.profile.unique_id()]);

                auto dev_name = s.second.dev ? s.second.dev->dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
                std::string stream_name = rs2_stream_to_string(s.second.profile.stream_type());
                if (s.second.profile.stream_index())
                    stream_name += "_" + std::to_string(s.second.profile.stream_index());
                tex_sources_str.push_back(to_string() << dev_name << " " << stream_name);

                i++;
            }
        }

        if (tex_sources_str.size() && depth_sources_str.size())
        {
            combo_boxes++;
            if (RS2_STREAM_COLOR == streams[selected_tex_source_uid].profile.stream_type())
                combo_boxes++;
        }

        auto top_bar_height = 32.f;
        const auto buttons_heights = top_bar_height;
        const auto num_of_buttons = 5;

        if (num_of_buttons * 40 + combo_boxes * (combo_box_width + 100) > stream_rect.w)
            top_bar_height = 2 * top_bar_height;

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

        if (depth_sources_str.size() > 0 && allow_3d_source_change)
        {
            ImGui::SetCursorPos({ 7, 7 });
            ImGui::Text("Depth Source:"); ImGui::SameLine();

            ImGui::SetCursorPosY(7);
            ImGui::PushItemWidth(combo_box_width);
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

            if (buttons_heights == top_bar_height) ImGui::SameLine();
        }

        if (!allow_3d_source_change) ImGui::SetCursorPos({ 7, 7 });

        // Only allow to change texture if we have something to put it on:
        if (tex_sources_str.size() && depth_sources_str.size())
        {
            if (buttons_heights == top_bar_height) ImGui::SetCursorPosY(7);
            else ImGui::SetCursorPos({ 7, buttons_heights + 7 });

            ImGui::Text("Texture Source:"); ImGui::SameLine();

            if (buttons_heights == top_bar_height) ImGui::SetCursorPosY(7);
            else ImGui::SetCursorPosY(buttons_heights + 7);

            ImGui::PushItemWidth(combo_box_width);
            draw_combo_box("##Tex Source", tex_sources_str, selected_tex_source);
            selected_tex_source_uid = tex_sources[selected_tex_source];
            texture.colorize = streams[tex_sources[selected_tex_source]].texture->colorize;
            ImGui::PopItemWidth();

            ImGui::SameLine();

            // Occlusion control for RGB UV-Map uses option's description as label
            // Position is dynamically adjusted to avoid overlapping on resize
            if (RS2_STREAM_COLOR==streams[selected_tex_source_uid].profile.stream_type())
            {
                ImGui::PushItemWidth(combo_box_width);
                ppf.get_pc_model()->get_option(rs2_option::RS2_OPTION_FILTER_MAGNITUDE).draw(error_message,
                    not_model, stream_rect.w < 1000, false);
                ImGui::PopItemWidth();
            }
        }

        ImGui::SetCursorPos({ stream_rect.w - 32 * num_of_buttons - 5, 0 });

        if (paused)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            label = to_string() << textual_icons::play << "##Resume 3d";
            if (ImGui::Button(label.c_str(), { 24, buttons_heights }))
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
            label = to_string() << textual_icons::pause << "##Pause 3d";
            if (ImGui::Button(label.c_str(), { 24, buttons_heights }))
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

        if (ImGui::Button(textual_icons::floppy, { 24, buttons_heights }))
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

        if (ImGui::Button(textual_icons::refresh, { 24, buttons_heights }))
        {
            reset_camera();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset View");

        ImGui::SameLine();

        if (render_quads)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            label = to_string() << textual_icons::cubes << "##Render Quads";
            if (ImGui::Button(label.c_str(), { 24, buttons_heights }))
            {
                render_quads = false;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Render Quads");
            }
            ImGui::PopStyleColor(2);
        }
        else
        {
            label = to_string() << textual_icons::cubes << "##Render Points";
            if (ImGui::Button(label.c_str(), { 24, buttons_heights }))
            {
                render_quads = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Render Points");
            }
        }
        ImGui::SameLine();

        if (support_non_syncronized_mode)
        {
            if (synchronization_enable)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                if (ImGui::Button(textual_icons::lock, { 24, buttons_heights }))
                {
                    synchronization_enable = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Disable syncronization between the pointcloud and the texture");
                ImGui::PopStyleColor(2);
            }
            else
            {
                if (ImGui::Button(textual_icons::unlock, { 24, buttons_heights }))
                {
                    synchronization_enable = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Keep the pointcloud and the texture sycronized");
            }
        }


        ImGui::End();

        // Draw pose header if pose stream exists
        bool render_pose = false;

        for (auto&& s : streams)
        {
            if (s.second.is_stream_visible() &&
                s.second.profile.stream_type() == RS2_STREAM_POSE)
            {
                render_pose = true;
            }
        }

        auto total_top_bar_height = top_bar_height; // may include single bar or additional bar for pose

        if (render_pose)
        {
            total_top_bar_height += top_bar_height; // add additional bar height for pose
            const int num_of_pose_buttons = 3; // trajectory, draw camera, boundary selection

            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + buttons_heights });
            ImGui::SetNextWindowSize({ stream_rect.w, buttons_heights });
            std::string pose_label = to_string() << "header of 3dviewer - pose";
            ImGui::Begin(pose_label.c_str(), nullptr, flags);

            // Draw selection buttons on the pose header
            ImGui::SetCursorPos({ stream_rect.w - 32 * num_of_pose_buttons - 5, 0 });

            // Draw camera object button
            if (ImGui::Button(tm2.camera_object_button.get_icon().c_str(), { 24, buttons_heights }))
            {
                tm2.camera_object_button.toggle_button();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", tm2.camera_object_button.get_tooltip().c_str());

            // Draw trajectory button
            ImGui::SameLine();
            bool color_icon = tm2.trajectory_button.is_pressed(); //draw trajectory is on - color the icon
            if (color_icon)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            }
            if (ImGui::Button(tm2.trajectory_button.get_icon().c_str(), { 24, buttons_heights }))
            {
                tm2.trajectory_button.toggle_button();
            }
            if (color_icon)
            {
                ImGui::PopStyleColor(2);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", tm2.trajectory_button.get_tooltip().c_str());

            // Draw boundary selection button
            ImGui::SameLine();
            color_icon = tm2.boundary_button.is_pressed(); //draw boundary is on - color the icon
            if (color_icon)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            }
            if (ImGui::Button(tm2.boundary_button.get_icon().c_str(), { 24, buttons_heights }))
            {
                tm2.boundary_button.toggle_button();
            }
            if (color_icon)
            {
                ImGui::PopStyleColor(2);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", tm2.boundary_button.get_tooltip().c_str());

            ImGui::End();
        }

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

        ImGui::SetNextWindowPos({ stream_rect.x + stream_rect.w - 265, stream_rect.y + total_top_bar_height + 5 });
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

    void stream_model::show_stream_header(ImFont* font, const rect &stream_rect, viewer_model& viewer)
    {
        const auto top_bar_height = 32.f;
        auto num_of_buttons = 4;
        if (!viewer.allow_stream_close) --num_of_buttons;
        if (viewer.streams.size() > 1) ++num_of_buttons;
        if (RS2_STREAM_DEPTH == profile.stream_type()) ++num_of_buttons; // Color map ruler button

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui_ScopePushFont(font);
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

            tooltip = to_string() << dev_name << " s.n:" << dev_serial << " | " << sensor_name << ", " << stream_name << " stream";
            const auto approx_char_width = 12;
            if (stream_rect.w - 32 * num_of_buttons >= (dev_name.size() + dev_serial.size() + sensor_name.size() + stream_name.size()) * approx_char_width)
                label = tooltip;
            else
            {
                // Use only the SKU type for compact representation and use only the last three digits for S.N
                auto short_name = split_string(dev_name, ' ').back();
                auto short_sn = dev_serial;
                short_sn.erase(0, dev_serial.size() - 5).replace(0, 2, "..");

                auto label_length = stream_rect.w - 32 * num_of_buttons;

                if (label_length >= (short_name.size() + dev_serial.size() + sensor_name.size() + stream_name.size()) * approx_char_width)
                    label = to_string() << short_name << " s.n:" << dev_serial << " | " << sensor_name << " " << stream_name << " stream";
                else if (label_length >= (short_name.size() + short_sn.size() + stream_name.size()) * approx_char_width)
                    label = to_string() << short_name << " s.n:" << short_sn << " " << stream_name << " stream";
                else if (label_length >= short_name.size() * approx_char_width)
                    label = to_string() << short_name << " " << stream_name;
                else
                    label = "";
            }
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

        if (RS2_STREAM_DEPTH == profile.stream_type())
        {
            label = to_string() << textual_icons::bar_chart << "##Color map";
            if (show_map_ruler)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
                {
                    show_map_ruler = false;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hide color map ruler");
                }
                ImGui::PopStyleColor(2);
            }
            else
            {
                if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
                {
                    show_map_ruler = true;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Show color map ruler");
                }
            }
            ImGui::SameLine();
        }

        auto p = dev->dev.as<playback>();
        if (dev->is_paused() || (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            label = to_string() << textual_icons::play << "##Resume " << profile.unique_id();
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
            label = to_string() << textual_icons::pause << "##Pause " << profile.unique_id();
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

        label = to_string() << textual_icons::camera << "##Snapshot " << profile.unique_id();
        if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
        {
            auto filename = file_dialog_open(save_file, "Portable Network Graphics (PNG)\0*.png\0", NULL, NULL);

            if (filename)
            {
                snapshot_frame(filename,viewer);
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save snapshot");
        }
        ImGui::SameLine();

        label = to_string() << textual_icons::info_circle << "##Info " << profile.unique_id();
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
                label = to_string() << textual_icons::window_maximize << "##Maximize " << profile.unique_id();

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

                label = to_string() << textual_icons::window_restore << "##Restore " << profile.unique_id();

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
            label = to_string() << textual_icons::times << "##Stop " << profile.unique_id();
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
            static const auto width_info_rect = 270.f;
            static const auto height_info_rect = 45.f;
            static const auto y_offset_info_rect = 5.f;
            static const auto x_offset_info_rect = 275.f;
            curr_info_rect = rect{ stream_rect.x + stream_rect.w - x_offset_info_rect,
                                   stream_rect.y + y_offset_info_rect,
                                   width_info_rect,
                                   height_info_rect };
            ImGui::SetNextWindowPos({ curr_info_rect.x, curr_info_rect.y });
            ImGui::SetNextWindowSize({ curr_info_rect.w, curr_info_rect.h });
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

    void stream_model::show_stream_footer(ImFont* font, const rect &stream_rect, const  mouse_info& mouse)
    {
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

                ss << std::dec << ", " << std::setprecision(2) << meters << " meters";
            }

            std::string msg(ss.str().c_str());

            auto flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoInputs;

            ImGui_ScopePushFont(font);

            // adjust windows size to the message length
            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + stream_rect.h - 35 });
            ImGui::SetNextWindowSize({ float(msg.size()*8), 20 });

            std::string label = to_string() << "Footer for stream of " << profile.unique_id();
            ImGui::Begin(label.c_str(), nullptr, flags);

            ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(9, 11, 13, 100));
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

            ImGui::Text("%s", msg.c_str());
            ImGui::PopStyleColor(3);

            ImGui::End();
        }
    }

    void stream_model::snapshot_frame(const char* filename, viewer_model& viewer) const
    {
        std::stringstream ss;
        std::string stream_desc{};
        std::string filename_base(filename);

        // Trim the file extension when provided. Note that this may amend user-provided file name in case it uses the "." character, e.g. "my.file.name"
        auto loc = filename_base.find_last_of(".");
        if (loc  != std::string::npos)
            filename_base.erase(loc, std::string::npos);

       // Snapshot the color-augmented version of the frame
        if (auto colorized_frame = texture->get_last_frame(true).as<video_frame>())
        {
            stream_desc = rs2_stream_to_string(colorized_frame.get_profile().stream_type());
            auto filename_png = filename_base + "_" + stream_desc + ".png";
            save_to_png(filename_png.data(), colorized_frame.get_width(), colorized_frame.get_height(), colorized_frame.get_bytes_per_pixel(),
                colorized_frame.get_data(), colorized_frame.get_width() * colorized_frame.get_bytes_per_pixel());

            ss << "PNG snapshot was saved to " << filename_png << std::endl;
        }

        auto original_frame = texture->get_last_frame(false).as<video_frame>();

        // For Depth-originated streams also provide a copy of the raw data accompanied by sensor-specific metadata
        if (original_frame && val_in_range(original_frame.get_profile().stream_type(), { RS2_STREAM_DEPTH , RS2_STREAM_INFRARED }))
        {
            stream_desc = rs2_stream_to_string(original_frame.get_profile().stream_type());

            //Capture raw frame
            auto filename = filename_base + "_" + stream_desc + ".raw";
            if (save_frame_raw_data(filename, original_frame))
                ss << "Raw data is captured into " << filename << std::endl;
            else
                viewer.not_model.add_notification({ to_string() << "Failed to save frame raw data  " << filename,
                    0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });

            // And the frame's attributes
            filename = filename_base + "_" + stream_desc + "_metadata.csv";
            if (frame_metadata_to_csv(filename, original_frame))
                ss << "The frame attributes are saved into " << filename;
            else
                viewer.not_model.add_notification({ to_string() << "Failed to save frame metadata file " << filename,
                    0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
        }

        if (ss.str().size())
            viewer.not_model.add_notification({ ss.str().c_str(), 0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT });

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

            r = r.normalize(_normalized_zoom.unnormalize(get_original_stream_bounds())).unnormalize(stream_rect).cut_by(stream_rect);

            glColor3f(yellow.x, yellow.y, yellow.z);
            draw_rect(r, 2);

            std::string message = "Metrics Region of Interest";
            auto msg_width = stb_easy_font_width((char*)message.c_str());
            if (msg_width < r.w)
                draw_text(static_cast<int>(r.x + r.w / 2 - msg_width / 2), static_cast<int>(r.y + 10), message.c_str());

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
            ImGui::Icon(textual_icons::minus); ImGui::SameLine();
            ImGui::PopStyleColor();

            rc = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ rc.x, rc.y - 4 });

            std::string label = to_string() << "##log_entry" << i++;
            ImGui::InputTextEx(label.c_str(),
                        (char*)line.data(),
                        static_cast<int>(line.size() + 1),
                        ImVec2(-1, ImGui::GetTextLineHeight() * 1.5f * float(std::max(1,(int)std::count(line.begin(),line.end(), '\n')))),
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

        ImGui_ScopePushFont(font_14);
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

        std::string name = std::string(textual_icons::exclamation_triangle) + " Oops, something went wrong!";

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
    }
    void viewer_model::show_icon(ImFont* font_18, const char* label_str, const char* text, int x, int y, int id,
        const ImVec4& text_color, const std::string& tooltip)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui_ScopePushFont(font_18);
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
    }
    void viewer_model::show_paused_icon(ImFont* font_18, int x, int y, int id)
    {
        show_icon(font_18, "paused_icon", textual_icons::pause, x, y, id, white);
    }
    void viewer_model::show_recording_icon(ImFont* font_18, int x, int y, int id, float alpha_delta)
    {
        show_icon(font_18, "recording_icon", textual_icons::circle, x, y, id, from_rgba(255, 46, 54, static_cast<uint8_t>(alpha_delta * 255)));
    }

    rs2::frame post_processing_filters::apply_filters(rs2::frame f)
    {
        rs2_stream stream_type = f.get_profile().stream_type();
        if (stream_type == RS2_STREAM_DEPTH || stream_type == RS2_STREAM_COLOR || stream_type == RS2_STREAM_INFRARED)
        {
            for (auto&& s : viewer.streams)
            {
                if (!s.second.dev) continue;
                auto dev = s.second.dev;

                if (s.second.original_profile.unique_id() == f.get_profile().unique_id())
                {
                    if (dev->post_processing_enabled)
                    {
                        auto dec_filter = s.second.dev->decimation_filter;
                        if (dec_filter->enabled)
                            f = dec_filter->invoke(f);

                        if (stream_type == RS2_STREAM_DEPTH)
                        {
                            auto depth_2_disparity = s.second.dev->depth_to_disparity;
                            auto spatial_filter = s.second.dev->spatial_filter;
                            auto temp_filter = s.second.dev->temporal_filter;
                            auto hole_filling = s.second.dev->hole_filling_filter;
                            auto disparity_2_depth = s.second.dev->disparity_to_depth;

                            if (depth_2_disparity->enabled)
                                f = depth_2_disparity->invoke(f);

                            if (spatial_filter->enabled)
                                f = spatial_filter->invoke(f);

                            if (temp_filter->enabled)
                                f = temp_filter->invoke(f);

                            if (disparity_2_depth->enabled)
                                f = disparity_2_depth->invoke(f);

                            if (hole_filling->enabled)
                                f = hole_filling->invoke(f);
                        }

                        break;
                    }
                }
            }
        }

        // Override the zero pixel in texture frame with black color for occlusion invalidation
        // TODO - this is a temporal solution to be refactored from the app level into the core library
        switch (stream_type)
        {
        case RS2_STREAM_COLOR:
        {
                auto rgb_stream = const_cast<uint8_t*>(static_cast<const uint8_t*>(f.get_data()));
                memset(rgb_stream, 0, 3);
                // Alternatively, enable the next two lines to render invalidation with magenta color for inspection
                //rgb_stream[0] = rgb_stream[2] = 0xff; // Use magenta to highlight the occlusion areas
                //rgb_stream[1] = 0;
            }
            break;
            case RS2_STREAM_INFRARED:
            {
                auto ir_stream = const_cast<uint8_t*>(static_cast<const uint8_t*>(f.get_data()));
                memset(ir_stream, 0, 2); // Override the first two bytes to cover Y8/Y16 formats
            }
            break;
            default:
                break;
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
        viewer.streams_origin[uid] = new_uid;
        viewer.streams_origin[new_uid] = uid;

        if(viewer.is_3d_view)
        {
            if(viewer.is_3d_depth_source(f))
            {
                res.push_back(pc->calculate(filtered));
            }
            if(viewer.is_3d_texture_source(f))
            {
                update_texture(filtered);
            }
        }

        return res;
    }

    void post_processing_filters::process(rs2::frame f, const rs2::frame_source& source)
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
        while (render_thread_active)
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
                    for (auto&& q : frames_queue_local)
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
        std::string text = to_string() << "Nothing is streaming! Toggle " << textual_icons::toggle_off << " to start";
        ImGui::Text("%s", text.c_str());
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
        frame f{}, depth{};

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
                            depth = frame;

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

        not_model.draw(window.get_font(), static_cast<int>(window.width()), static_cast<int>(window.height()));

        popup_if_error(window.get_font(), error_message);

        return f;
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

    void viewer_model::draw_color_ruler(const mouse_info& mouse,
                                        const stream_model& s_model,
                                        const rect& stream_rect,
                                        std::vector<rgb_per_distance> rgb_per_distance_vec,
                                        float ruler_length,
                                        const std::string& ruler_units)
    {
        if (rgb_per_distance_vec.empty() || (ruler_length <= 0.f))
            return;

        ruler_length = std::ceil(ruler_length);
        std::sort(rgb_per_distance_vec.begin(), rgb_per_distance_vec.end(), [](const rgb_per_distance& a,
            const rgb_per_distance& b) {
            return a.depth_val < b.depth_val;
        });

        const auto stream_height = stream_rect.y + stream_rect.h;
        const auto stream_width = stream_rect.x + stream_rect.w;

        static const auto ruler_distance_offset = 10;
        auto bottom_y_ruler = stream_height - ruler_distance_offset;
        if (s_model.texture->zoom_preview)
        {
            bottom_y_ruler = s_model.texture->curr_preview_rect.y - ruler_distance_offset;
        }

        static const auto top_y_offset = 50;
        auto top_y_ruler = stream_rect.y + top_y_offset;
        if (s_model.show_stream_details)
        {
            top_y_ruler = s_model.curr_info_rect.y + s_model.curr_info_rect.h + ruler_distance_offset;
        }

        static const auto left_x_colored_ruler_offset = 50;
        static const auto colored_ruler_width = 20;
        const auto left_x_colored_ruler = stream_width - left_x_colored_ruler_offset;
        const auto right_x_colored_ruler = stream_width - (left_x_colored_ruler_offset - colored_ruler_width);
        const auto first_rgb = rgb_per_distance_vec.begin()->rgb_val;
        assert((bottom_y_ruler - top_y_ruler) != 0.f);
        const auto ratio = (bottom_y_ruler - top_y_ruler) / ruler_length;

        // Draw numbered ruler
        float y_ruler_val = 0.f;
        auto flags = ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoScrollbar;
        static const auto numbered_ruler_width = 20.f;
        const auto numbered_ruler_height = bottom_y_ruler - top_y_ruler;
        ImGui::SetNextWindowPos({ right_x_colored_ruler, top_y_ruler });
        ImGui::SetNextWindowSize({ numbered_ruler_width, numbered_ruler_height });
        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f, 0.f, 0.f, 0.f });
        ImGui::Begin("numbered_ruler", nullptr, flags);

        const auto right_x_numbered_ruler = right_x_colored_ruler + numbered_ruler_width;
        static const auto hovered_numbered_ruler_opac = 0.8f;
        static const auto unhovered_numbered_ruler_opac = 0.6f;
        float colored_ruler_opac = unhovered_numbered_ruler_opac;
        float numbered_ruler_background_opac = unhovered_numbered_ruler_opac;
        bool is_ruler_hovered = false;
        if (mouse.cursor.x >= left_x_colored_ruler &&
            mouse.cursor.x <= right_x_numbered_ruler &&
            mouse.cursor.y >= top_y_ruler &&
            mouse.cursor.y <= bottom_y_ruler)
            is_ruler_hovered = true;

        if (is_ruler_hovered)
        {
            std::stringstream ss;
            auto relative_mouse_y = ImGui::GetMousePos().y - top_y_ruler;
            auto y = (bottom_y_ruler - top_y_ruler) - relative_mouse_y;
            ss << std::fixed << std::setprecision(2) << (y / ratio) << ruler_units;
            ImGui::SetTooltip("%s", ss.str().c_str());
            colored_ruler_opac = 1.f;
            numbered_ruler_background_opac = hovered_numbered_ruler_opac;
        }

        // Draw a background to the numbered ruler
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0, 0.0, 0.0, numbered_ruler_background_opac);
        glBegin(GL_POLYGON);
        glVertex2f(right_x_colored_ruler, top_y_ruler);
        glVertex2f(right_x_numbered_ruler , top_y_ruler);
        glVertex2f(right_x_numbered_ruler , bottom_y_ruler);
        glVertex2f(right_x_colored_ruler, bottom_y_ruler);
        glEnd();


        static const float x_ruler_val = 4.0f;
        ImGui::SetCursorPos({ x_ruler_val, y_ruler_val });
        const auto font_size = ImGui::GetFontSize();
        ImGui::TextUnformatted(std::to_string(static_cast<int>(ruler_length)).c_str());
        const auto skip_numbers = ((ruler_length / 10.f) - 1.f);
        auto to_skip = (skip_numbers < 0.f)?0.f: skip_numbers;
        for (int i = static_cast<int>(ruler_length - 1); i > 0; --i)
        {
            y_ruler_val += ((bottom_y_ruler - top_y_ruler) / ruler_length);
            ImGui::SetCursorPos({ x_ruler_val, y_ruler_val - font_size / 2 });
            if (((to_skip--) > 0))
                continue;

            ImGui::TextUnformatted(std::to_string(i).c_str());
            to_skip = skip_numbers;
        }
        y_ruler_val += ((bottom_y_ruler - top_y_ruler) / ruler_length);
        ImGui::SetCursorPos({ x_ruler_val, y_ruler_val - font_size });
        ImGui::Text("0");
        ImGui::End();
        ImGui::PopStyleColor();

        auto total_depth_scale = rgb_per_distance_vec.back().depth_val - rgb_per_distance_vec.front().depth_val;
        static const auto sensitivity_factor = 0.01f;
        auto sensitivity = sensitivity_factor * total_depth_scale;

        // Draw colored ruler
        auto last_y = bottom_y_ruler;
        auto last_depth_value = 0.f;
        auto last_index = 0;
        for (auto i = 1; i < rgb_per_distance_vec.size(); ++i)
        {
            auto curr_depth = rgb_per_distance_vec[i].depth_val;
            if ((((curr_depth - last_depth_value) < sensitivity) && (i != rgb_per_distance_vec.size() - 1)))
                continue;

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_QUADS);
            glColor4f(rgb_per_distance_vec[last_index].rgb_val.r / 255.f,
                      rgb_per_distance_vec[last_index].rgb_val.g / 255.f,
                      rgb_per_distance_vec[last_index].rgb_val.b / 255.f,
                      colored_ruler_opac);
            glVertex2f(left_x_colored_ruler, last_y);
            glVertex2f(right_x_colored_ruler, last_y);

            last_depth_value = curr_depth;
            last_index = i;

            auto y = bottom_y_ruler - ((rgb_per_distance_vec[i].depth_val) * ratio);
            if ((i == (rgb_per_distance_vec.size() - 1)) || (std::ceil(curr_depth) > ruler_length))
                y = top_y_ruler;

            glColor4f(rgb_per_distance_vec[i].rgb_val.r / 255.f,
                      rgb_per_distance_vec[i].rgb_val.g / 255.f,
                      rgb_per_distance_vec[i].rgb_val.b / 255.f,
                      colored_ruler_opac);

            glVertex2f(right_x_colored_ruler, y);
            glVertex2f(left_x_colored_ruler, y);
            last_y = y;
            glEnd();
        }

        // Draw ruler border
        static const auto top_line_offset = 0.5f;
        static const auto right_line_offset = top_line_offset / 2;
        glColor4f(0.0, 0.0, 0.0, colored_ruler_opac);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left_x_colored_ruler - top_line_offset, top_y_ruler - top_line_offset);
        glVertex2f(right_x_numbered_ruler + right_line_offset / 2, top_y_ruler - top_line_offset);
        glVertex2f(right_x_numbered_ruler + right_line_offset / 2, bottom_y_ruler + top_line_offset);
        glVertex2f(left_x_colored_ruler - top_line_offset, bottom_y_ruler + top_line_offset);
        glEnd();
    }

    float viewer_model::calculate_ruler_max_distance(const std::vector<float>& distances) const
    {
        assert(!distances.empty());

        float mean = std::accumulate(distances.begin(),
            distances.end(), 0.0f) / distances.size();

        float e = 0;
        float inverse = 1.f / distances.size();
        for (auto elem : distances)
        {
            e += pow(elem - mean, 2);
        }

        auto standard_deviation = sqrt(inverse * e);
        static const auto length_jump = 4.f;
        return std::ceil((mean + 1.5f * standard_deviation) / length_jump) * length_jump;
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
            static_cast<GLsizei>(win.framebuf_width()), static_cast<GLsizei>(win.framebuf_height()));
        glLoadIdentity();
        glOrtho(0, win.width(), win.height(), 0, -1, +1);

        auto layout = calc_layout(view_rect);

        if ((layout.size() == 0) && (dev_model_num > 0))
        {
            show_no_stream_overlay(font2, static_cast<int>(view_rect.x), static_cast<int>(view_rect.y), static_cast<int>(win.width()), static_cast<int>(win.height() - output_height));
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

            if (stream_mv.dev->_is_being_recorded)
            {
                show_recording_icon(font2, static_cast<int>(pos), static_cast<int>(stream_rect.y + 5), stream_mv.profile.unique_id(), alpha);
                pos += 23;
            }

            if (!stream_mv.is_stream_alive())
            {
                std::string message = to_string() << textual_icons::exclamation_triangle << " No Frames Received!";
                show_icon(font2, "warning_icon", message.c_str(),
                    static_cast<int>(stream_rect.center().x - 100),
                    static_cast<int>(stream_rect.center().y - 25),
                    stream_mv.profile.unique_id(),
                    blend(dark_red, alpha),
                    "Did not receive frames from the platform within a reasonable time window,\nplease try reducing the FPS or the resolution");
            }

            if (stream_mv.dev->is_paused() || (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED))
                show_paused_icon(font2, static_cast<int>(pos), static_cast<int>(stream_rect.y + 5), stream_mv.profile.unique_id());

            stream_mv.show_stream_header(font1, stream_rect, *this);
            stream_mv.show_stream_footer(font1, stream_rect, mouse);

            glColor3f(header_window_bg.x, header_window_bg.y, header_window_bg.z);
            stream_rect.y -= 32;
            stream_rect.h += 32;
            stream_rect.w += 1;
            draw_rect(stream_rect);

            auto frame = streams[stream].texture->get_last_frame().as<video_frame>();
            auto textured_frame = streams[stream].texture->get_last_frame(true).as<video_frame>();
            if (streams[stream].show_map_ruler && frame && textured_frame &&
                RS2_STREAM_DEPTH == stream_mv.profile.stream_type() &&
                RS2_FORMAT_Z16 == stream_mv.profile.format())
            {
                assert(RS2_FORMAT_RGB8 == textured_frame.get_profile().format());
                static const std::string depth_units = "m";
                float ruler_length = 0.f;
                auto depth_vid_profile = stream_mv.profile.as<video_stream_profile>();
                auto depth_width = depth_vid_profile.width();
                auto depth_height = depth_vid_profile.height();
                auto num_of_pixels = depth_width * depth_height;
                auto depth_data = static_cast<const uint16_t*>(frame.get_data());
                auto textured_depth_data = static_cast<const uint8_t*>(textured_frame.get_data());
                static const auto skip_pixels_factor = 30;
                std::vector<rgb_per_distance> rgb_per_distance_vec;
                std::vector<float> distances;
                for (uint64_t i = 0; i < depth_height; i+= skip_pixels_factor)
                {
                    for (uint64_t j = 0; j < depth_width; j+= skip_pixels_factor)
                    {
                        auto depth_index = i*depth_width + j;
                        auto length = depth_data[depth_index] * stream_mv.dev->depth_units;
                        if (length > 0.f)
                        {
                            auto textured_depth_index = depth_index * 3;
                            auto r = textured_depth_data[textured_depth_index];
                            auto g = textured_depth_data[textured_depth_index + 1];
                            auto b = textured_depth_data[textured_depth_index + 2];
                            rgb_per_distance_vec.push_back({ length, { r, g, b } });
                            distances.push_back(length);
                        }
                    }
                }

                if (!distances.empty())
                {
                    ruler_length = calculate_ruler_max_distance(distances);
                    draw_color_ruler(mouse, streams[stream], stream_rect, rgb_per_distance_vec, ruler_length, depth_units);
                }
            }
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
        glViewport(static_cast<GLint>(viewer_rect.x), 0,
            static_cast<GLsizei>(viewer_rect.w), static_cast<GLsizei>(viewer_rect.h));

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

        texture_buffer::draw_axis(0.1f, 1);

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

        for (auto&& stream : streams)
        {
            if (stream.second.profile.stream_type() == RS2_STREAM_POSE)
            {
                auto f = stream.second.texture->get_last_frame();
                auto pose = f.as<pose_frame>();
                if (!pose)
                    continue;

                rs2_pose pose_data = pose.get_pose_data();
                matrix4 pose_trans = tm2_pose_to_world_transformation(pose_data);
                float model[16];
                pose_trans.to_column_major(model);

                // set the pose transformation as the model matrix to draw the axis
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadMatrixf(view);
                glMultMatrixf(model); // = view x model

                if (stream.second.profile.stream_index() > 1) //TODO: use a more robust way to identfy this
                {
                    tm2.draw_controller_pose_object();
                }
                else
                {
                    tm2.draw_pose_object();
                }

                // remove model matrix from the rest of the render
                glPopMatrix();

                rs2_vector translation{ pose_trans.mat[0][3], pose_trans.mat[1][3], pose_trans.mat[2][3] };
                tracked_point p{ translation , pose_data.tracker_confidence }; //TODO: Osnat - use tracker_confidence or mapper_confidence ?
                tm2.draw_trajectory(p);
                tm2.draw_boundary(p);
            }
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
                auto top_right = get_point(static_cast<float>(intrin.width), 0);
                auto bottom_right = get_point(static_cast<float>(intrin.width), static_cast<float>(intrin.height));
                auto bottom_left = get_point(0, static_cast<float>(intrin.height));

                glVertex3fv(&top_left.x); glVertex3fv(&top_right.x);
                glVertex3fv(&top_right.x); glVertex3fv(&bottom_right.x);
                glVertex3fv(&bottom_right.x); glVertex3fv(&bottom_left.x);
                glVertex3fv(&bottom_left.x); glVertex3fv(&top_left.x);
            }

            glEnd();

            glColor4f(1.f, 1.f, 1.f, 1.f);
        }

        if (last_points && last_texture)
        {
            auto vf_profile = last_points.get_profile().as<video_stream_profile>();
            // Non-linear correspondence customized for non-flat surface exploration
            glPointSize(std::sqrt(viewer_rect.w / vf_profile.width()));

            auto tex = last_texture->get_gl_handle();
            glBindTexture(GL_TEXTURE_2D, tex);
            glEnable(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);

            //glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);

            auto vertices = last_points.get_vertices();
            auto tex_coords = last_points.get_texture_coordinates();
            if (!render_quads)
            {
                glBegin(GL_POINTS);
                for (int i = 0; i < last_points.size(); i++)
                {
                    if (vertices[i].z)
                    {
                        glVertex3fv(vertices[i]);
                        glTexCoord2fv(tex_coords[i + 1]);
                    }
                }
                glEnd();
            }
            else
            {
                // Visualization with quads produces better results but requires further optimization
                glBegin(GL_QUADS);

                const auto threshold = 0.05f;
                auto width = vf_profile.width(), height = vf_profile.height();
                for (int x = 0; x < width - 1; ++x) {
                    for (int y = 0; y < height - 1; ++y) {
                        auto a = y * width + x, b = y * width + x + 1, c = (y + 1)*width + x, d = (y + 1)*width + x + 1;
                        if (vertices[a].z && vertices[b].z && vertices[c].z && vertices[d].z
                            && abs(vertices[a].z - vertices[b].z) < threshold && abs(vertices[a].z - vertices[c].z) < threshold
                            && abs(vertices[b].z - vertices[d].z) < threshold && abs(vertices[c].z - vertices[d].z) < threshold) {
                            glVertex3fv(vertices[a]); glTexCoord2fv(tex_coords[a]);
                            glVertex3fv(vertices[b]); glTexCoord2fv(tex_coords[b]);
                            glVertex3fv(vertices[d]); glTexCoord2fv(tex_coords[d]);
                            glVertex3fv(vertices[c]); glTexCoord2fv(tex_coords[c]);
                        }
                    }
                }
                glEnd();
            }
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

        //if (ImGui::Button(u8"\uf013textual_icons::caret_down", { panel_y,panel_y }))
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
        auto sec_since_update = std::chrono::duration<float, std::milli>(now - view_clock).count() / 1000;
        view_clock = now;

        if (fixed_up)
            up = { 0.f, -1.f, 0.f };

        auto dir = target - pos;
        auto x_axis = cross(dir, up);
        auto step = sec_since_update * 0.3f;

        if (ImGui::IsKeyPressed('w') || ImGui::IsKeyPressed('W'))
        {
            pos = pos + dir * step;
            target = target + dir * step;
        }
        if (ImGui::IsKeyPressed('s') || ImGui::IsKeyPressed('S'))
        {
            pos = pos - dir * step;
            target = target - dir * step;
        }
        if (ImGui::IsKeyPressed('d') || ImGui::IsKeyPressed('D'))
        {
            pos = pos + x_axis * step;
            target = target + x_axis * step;
        }
        if (ImGui::IsKeyPressed('a') || ImGui::IsKeyPressed('A'))
        {
            pos = pos - x_axis * step;
            target = target - x_axis * step;
        }

        if (viewer_rect.contains(mouse.cursor) || force)
        {
            arcball_camera_update(
                (float*)&pos, (float*)&target, (float*)&up, view,
                sec_since_update,
                0.2f, // zoom per tick
                -0.1f, // pan speed
                3.0f, // rotation multiplier
                static_cast<int>(viewer_rect.w), static_cast<int>(viewer_rect.h), // screen (window) size
                static_cast<int>(mouse.prev_cursor.x), static_cast<int>(mouse.cursor.x),
                static_cast<int>(mouse.prev_cursor.y), static_cast<int>(mouse.cursor.y),
                (ImGui::GetIO().MouseDown[2] || ImGui::GetIO().MouseDown[1]) ? 1 : 0,
                ImGui::GetIO().MouseDown[0] ? 1 : 0,
                mouse.mouse_wheel,
                0);
        }

        mouse.prev_cursor = mouse.cursor;
    }

    void viewer_model::begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p)
    {
        // Starting post processing filter rendering thread
        ppf.start();
        streams[p.unique_id()].begin_stream(d, p);
        ppf.frames_queue.emplace(p.unique_id(), rs2::frame_queue(5));
    }

    bool viewer_model::is_3d_texture_source(frame f)
    {
        auto index = f.get_profile().unique_id();
        auto mapped_index = streams_origin[index];

        if (index == selected_tex_source_uid || mapped_index == selected_tex_source_uid || selected_tex_source_uid == -1)
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
            for (auto&& sub_dev_model : subdevices)
            {
                sub_dev_model->_is_being_recorded = true;
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

    void device_model::stop_recording(viewer_model& viewer)
    {
        auto saved_to_filename = _recorder->filename();
        _recorder.reset();
        for (auto&& sub_dev_model : subdevices)
        {
            sub_dev_model->_is_being_recorded = false;
        }
        is_recording = false;
        notification_data nd{ to_string() << "Saved recording to: " << saved_to_filename,
            (double)std::chrono::high_resolution_clock::now().time_since_epoch().count(),
            RS2_LOG_SEVERITY_INFO,
            RS2_NOTIFICATION_CATEGORY_COUNT };
        viewer.not_model.add_notification(nd);
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

        const int playback_control_height = 35;
        const float combo_box_width = 90.f;
        const float icon_width = 28;
        const float line_width = 255; //Ideally should use: ImGui::GetContentRegionMax().x
        //Line looks like this ("-" == space, "[]" == icon, "[     ]" == combo_box):  |-[]-[]-[]-[]-[]-[     ]-[]-|
        const int num_icons_in_line = 6;
        const int num_combo_boxes_in_line = 1;
        const int num_spaces_in_line = num_icons_in_line + num_combo_boxes_in_line + 1;
        const float required_row_width = (num_combo_boxes_in_line * combo_box_width) + (num_icons_in_line * icon_width);
        float space_width = std::max(line_width - required_row_width, 0.f) / num_spaces_in_line;
        ImVec2 button_dim = { icon_width, icon_width };

        const bool supports_playback_step = false;

        ImGui::PushFont(font);

        //////////////////// Step Backwards Button ////////////////////
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);

        std::string label = to_string() << textual_icons::step_backward << "##Step Backwards " << id;

        if (ImGui::ButtonEx(label.c_str(), button_dim, supports_playback_step ? 0 : ImGuiButtonFlags_Disabled))
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
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);
        label = to_string() << textual_icons::stop << "##Stop Playback " << id;

        if (ImGui::ButtonEx(label.c_str(), button_dim))
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
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);
        if (current_playback_status == RS2_PLAYBACK_STATUS_PAUSED || current_playback_status == RS2_PLAYBACK_STATUS_STOPPED)
        {
            label = to_string() << textual_icons::play << "##Play " << id;
            if (ImGui::ButtonEx(label.c_str(), button_dim))
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
            label = to_string() << textual_icons::pause << "##Pause Playback " << id;
            if (ImGui::ButtonEx(label.c_str(), button_dim))
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
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);
        label = to_string() << textual_icons::step_forward << "##Step Forward " << id;
        if (ImGui::ButtonEx(label.c_str(), button_dim, supports_playback_step ? 0 : ImGuiButtonFlags_Disabled))
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
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);
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
        label = to_string() << textual_icons::repeat << "##Repeat " << id;
        if (ImGui::ButtonEx(label.c_str(), button_dim))
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


        //////////////////// Speed combo box ////////////////////
        auto pos = ImGui::GetCursorPos();
        const float speed_combo_box_v_alignment = 3.f;
        ImGui::SetCursorPos({ pos.x + space_width, pos.y + speed_combo_box_v_alignment });
        ImGui::PushItemWidth(combo_box_width);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);

        label = to_string() << "## " << id;
        if (ImGui::Combo(label.c_str(), &playback_speed_index, "Speed:   x0.25\0Speed:   x0.5\0Speed:   x1\0Speed:   x1.5\0Speed:   x2\0\0", -1, false))
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
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Change playback speed rate");
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        //Restore Y movement
        pos = ImGui::GetCursorPos();
        ImGui::SetCursorPos({ pos.x, pos.y - speed_combo_box_v_alignment });
        //////////////////// Speed combo box ////////////////////

        ////////////////////    Info Icon    ////////////////////
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);
        draw_info_icon(button_dim);
        ////////////////////    Info Icon    ////////////////////

        ImGui::PopFont();

        return playback_control_height;
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
        auto playback_status = p.current_status();
        if (seek_pos != 0 && playback_status == RS2_PLAYBACK_STATUS_STOPPED)
        {
            seek_pos = 0;
        }
        float seek_bar_width = 300.f;
        ImGui::PushItemWidth(seek_bar_width);
        std::string label1 = "## " + id;
        if (ImGui::SeekSlider(label1.c_str(), &seek_pos, ""))
        {
            //Seek was dragged
            if (playback_status != RS2_PLAYBACK_STATUS_STOPPED) //Ignore seek when playback is stopped
            {
                auto duration_db = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(p.get_duration());
                auto single_percent = duration_db.count() / 100;
                auto seek_time = std::chrono::duration<double, std::nano>(seek_pos * single_percent);
                p.seek(std::chrono::duration_cast<std::chrono::nanoseconds>(seek_time));
            }
        }

        ImGui::SetCursorPos({ pos.x, pos.y + 17 });

        std::string time_elapsed = pretty_time(std::chrono::nanoseconds(progress));
        std::string duration_str = pretty_time(std::chrono::nanoseconds(playback_total_duration));
        ImGui::Text("%s", time_elapsed.c_str());
        ImGui::SameLine();
        float pos_y = ImGui::GetCursorPosY();

        ImGui::SetCursorPos({ pos.x + seek_bar_width - 45 , pos_y });
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
        float seek_bar_left_alignment = 4.f;
        ImGui::SetCursorPos({ pos.x + seek_bar_left_alignment, pos.y + controls_height });
        ImGui::PushFont(font);
        auto seek_bar_height = draw_seek_bar();
        ImGui::PopFont();
        ImGui::PopStyleColor(5);
        return controls_height + seek_bar_height;

    }

    void device_model::draw_controllers_panel(ImFont* font, bool is_device_streaming)
    {
        if (!is_device_streaming)
        {
            controllers.clear();
            available_controllers.clear();
            return;
        }

        if (controllers.size() > 0 || available_controllers.size() > 0)
        {
            int flags = dev.is<playback>() ? ImGuiButtonFlags_Disabled : 0;
            ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0, 0xae, 0xff, 255));
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushFont(font);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10,0 });
            const float button_dim = 30.f;
            for (auto&& c : available_controllers)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                std::string action = "Attach controller";
                std::string mac = to_string() << (int)c[0] << ":" << (int)c[1] << ":" << (int)c[2] << ":" << (int)c[3] << ":" << (int)c[4] << ":" << (int)c[5];
                std::string label = to_string() << u8"\uf11b" << "##" << action << mac;
                if (ImGui::ButtonEx(label.c_str(), { button_dim , button_dim }, flags))
                {
                    dev.as<tm2>().connect_controller(c);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", action.c_str());
                }
                ImGui::SameLine();
                ImGui::Text("%s", mac.c_str());
                ImGui::PopStyleColor(2);
            }
            for (auto&& c : controllers)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                std::string action = "Detach controller";
                std::string label = to_string() << u8"\uf11b" << "##" << action << c.first;
                if (ImGui::ButtonEx(label.c_str(), { button_dim , button_dim }, flags))
                {
                    dev.as<tm2>().disconnect_controller(c.first);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", action.c_str());
                }
                ImGui::SameLine();
                ImGui::Text("Controller #%d (connected)", c.first);
                ImGui::PopStyleColor(2);
            }
            ImGui::PopStyleVar();
            ImGui::PopFont();
            ImGui::PopStyleColor(5);
        }
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

    bool yes_no_dialog(const std::string& title, const std::string& message_text, bool& approved, ux_window& window)
    {
        ImGui_ScopePushFont(window.get_font());
        ImGui_ScopePushStyleColor(ImGuiCol_Button, button_color);
        ImGui_ScopePushStyleColor(ImGuiCol_ButtonHovered, sensor_header_light_blue); //TODO: Change color?
        ImGui_ScopePushStyleColor(ImGuiCol_ButtonActive, regular_blue); //TODO: Change color?
        ImGui_ScopePushStyleColor(ImGuiCol_Text, light_grey);
        ImGui_ScopePushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
        ImGui_ScopePushStyleColor(ImGuiCol_TitleBg, header_color);
        ImGui_ScopePushStyleColor(ImGuiCol_PopupBg, sensor_bg);
        ImGui_ScopePushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        auto clicked = false;
        ImGui::OpenPopup(title.c_str());
        if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("\n%s\n\n", message_text.c_str());
            auto width = ImGui::GetWindowWidth();
            ImGui::Dummy(ImVec2(0, 0));
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
    bool device_model::prompt_toggle_advanced_mode(bool enable_advanced_mode, const std::string& message_text, std::vector<std::string>& restarting_device_info, viewer_model& view, ux_window& window)
    {
        bool keep_showing = true;
        bool yes_was_chosen = false;
        if (yes_no_dialog("Advanced Mode", message_text, yes_was_chosen, window))
        {
            if (yes_was_chosen)
            {
                dev.as<advanced_mode>().toggle_advanced_mode(enable_advanced_mode);
                restarting_device_info = get_device_info(dev, false);
                view.not_model.add_log(enable_advanced_mode ? "Turning on advanced mode..." : "Turning off  advanced mode...");
            }
            keep_showing = false;
        }
        return keep_showing;
    }

    bool device_model::draw_advanced_controls(viewer_model& view, ux_window& window, std::string& error_message)
    {
        bool was_set = false;

        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 0.9f, 0.9f, 0.9f, 1 });

        auto is_advanced_mode = dev.is<advanced_mode>();
        if (is_advanced_mode && ImGui::TreeNode("Advanced Controls"))
        {
            try
            {
                auto advanced = dev.as<advanced_mode>();
                if (advanced.is_enabled())
                {
                    draw_advanced_mode_controls(advanced, amc, get_curr_advanced_controls, was_set, error_message);
                }
                else
                {
                    ImGui::TextColored(redish, "Device is not in advanced mode");
                    std::string button_text = to_string() << "Turn on Advanced Mode" << "##" << id;
                    static bool show_yes_no_modal = false;
                    if (ImGui::Button(button_text.c_str(), ImVec2{ 226, 0 }))
                    {
                        show_yes_no_modal = true;
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Advanced mode is a persistent camera state unlocking calibration formats and depth generation controls\nYou can always reset the camera to factory defaults by disabling advanced mode");
                    }
                    if (show_yes_no_modal)
                    {
                        show_yes_no_modal = prompt_toggle_advanced_mode(true, "\t\tAre you sure you want to turn on Advanced Mode?\t\t", restarting_device_info, view, window);
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
        return was_set;
    }

    void device_model::draw_info_icon(const ImVec2& size)
    {
        std::string info_button_name = to_string() << textual_icons::info_circle << "##" << id;
        auto info_button_color = show_device_info ? light_blue : light_grey;
        ImGui::PushStyleColor(ImGuiCol_Text, info_button_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, info_button_color);
        if (ImGui::Button(info_button_name.c_str(), size))
        {
            show_device_info = !show_device_info;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", show_device_info ? "Hide Device Details" : "Show Device Details");
        }
        ImGui::PopStyleColor(2);
    }

    float device_model::draw_device_panel(float panel_width,
                                          ux_window& window,
                                          std::string& error_message,
                                          viewer_model& viewer)
    {
        /*
        =============================
        [o]     [@]     [i]     [=]
        Record   Sync    Info    More
        =============================
        */

        const bool is_playback_device = dev.is<playback>();
        const float device_panel_height = 60.0f;
        auto panel_pos = ImGui::GetCursorPos();

        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        ////////////////////////////////////////
        // Draw recording icon
        ////////////////////////////////////////
        bool is_streaming = std::any_of(subdevices.begin(), subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm)
        {
            return sm->streaming;
        });
        textual_icon button_icon = is_recording ? textual_icons::stop : textual_icons::circle;
        const float icons_width = 78.0f;
        const ImVec2 device_panel_icons_size{ icons_width, 25 };
        std::string recorod_button_name = to_string() << button_icon << "##" << id;
        auto record_button_color = is_recording ? light_blue : light_grey;
        ImGui::PushStyleColor(ImGuiCol_Text, record_button_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, record_button_color);
        if (ImGui::ButtonEx(recorod_button_name.c_str(), device_panel_icons_size, (!is_streaming || is_playback_device) ? ImGuiButtonFlags_Disabled : 0))
        {
            if (is_recording) //is_recording is changed inside stop/start_recording
            {
                stop_recording(viewer);
            }
            else
            {
                auto path = rs2::get_folder_path(rs2::special_folder::user_documents) + rs2::get_timestamped_file_name() + ".bag";
                start_recording(path, error_message);
            }
        }
        if (ImGui::IsItemHovered())
        {
            std::string record_button_hover_text = (!is_streaming ? "Start streaming to enable recording" : (is_recording ? "Stop Recording" : "Start Recording"));
            ImGui::SetTooltip("%s", record_button_hover_text.c_str());
        }

        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ////////////////////////////////////////
        // Draw Sync icon
        ////////////////////////////////////////
        std::string sync_button_name = to_string() << textual_icons::refresh << "##" << id;
        bool is_sync_enabled = false; //TODO: use device's member
        auto sync_button_color = is_sync_enabled ? light_blue : light_grey;
        ImGui::PushStyleColor(ImGuiCol_Text, sync_button_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, sync_button_color);
        if (ImGui::ButtonEx(sync_button_name.c_str(), device_panel_icons_size, ImGuiButtonFlags_Disabled))
        {
            is_sync_enabled = !is_sync_enabled;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", is_sync_enabled ? "Disable streams synchronization" : "Enable streams synchronization");
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ////////////////////////////////////////
        // Draw Info icon
        ////////////////////////////////////////
        draw_info_icon(device_panel_icons_size);
        ImGui::SameLine();

        ////////////////////////////////////////
        // Draw Menu icon
        ////////////////////////////////////////
        std::string label = to_string() << "device_menu" << id;
        std::string bars_button_name = to_string() << textual_icons::bars << "##" << id;

        if (ImGui::Button(bars_button_name.c_str(), device_panel_icons_size))
        {
            ImGui::OpenPopup(label.c_str());
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", "Click for more");
        }
        ImGui::PopFont();
        ImGui::PushFont(window.get_font());
        static bool keep_showing_advanced_mode_modal = false;
        if (ImGui::BeginPopup(label.c_str()))
        {
            bool something_to_show = false;
            ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
            if (auto loopback = dev.as<rs2::tm2>())
            {
                something_to_show = true;
                try
                {
                    if (!loopback.is_loopback_enabled() && ImGui::Selectable("Enable loopback...", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        if (const char* ret = file_dialog_open(file_dialog_mode::open_file, "ROS-bag\0*.bag\0", NULL, NULL))
                        {
                            loopback.enable_loopback(ret);
                        }
                    }
                    if (loopback.is_loopback_enabled() && ImGui::Selectable("Disable loopback...", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        loopback.disable_loopback();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        if (is_streaming)
                            ImGui::SetTooltip("Stop streaming to use loopback functionality");
                        else
                            ImGui::SetTooltip("Enter the device to loopback mode (inject frames from file to FW)");
                    }
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

            if (allow_remove)
            {
                something_to_show = true;

                if (auto adv = dev.as<advanced_mode>())
                {
                    const bool is_advanced_mode_enabled = adv.is_enabled();
                    bool selected = is_advanced_mode_enabled;
                    if (ImGui::MenuItem("Advanced Mode", nullptr, &selected))
                    {
                        keep_showing_advanced_mode_modal = true;
                    }
                }

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
            }


            if (!something_to_show)
            {
                ImGui::Text("This device has no additional options");
            }

            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }

        if (keep_showing_advanced_mode_modal)
        {
            const bool is_advanced_mode_enabled = dev.as<advanced_mode>().is_enabled();
            std::string msg = to_string() << "\t\tAre you sure you want to " << (is_advanced_mode_enabled ? "turn off Advanced mode" : "turn on Advanced mode") << "\t\t";
            keep_showing_advanced_mode_modal = prompt_toggle_advanced_mode(!is_advanced_mode_enabled, msg, restarting_device_info, viewer, window);
        }
        ////////////////////////////////////////
        // Draw icons names
        ////////////////////////////////////////
        //Move to next line, and we want to keep the horizontal alignment
        ImGui::SetCursorPos({ panel_pos.x, ImGui::GetCursorPosY() });
        //Using transparent-non-actionable buttons to have the same locations
        ImGui::PushStyleColor(ImGuiCol_Button, ImColor(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(0,0,0,0));
        const ImVec2 device_panel_icons_text_size = { icons_width, 5 };
        ImGui::ButtonEx(is_recording ? "Stop" : "Record", device_panel_icons_size, (!is_streaming ? ImGuiButtonFlags_Disabled : 0));
        ImGui::SameLine();  ImGui::ButtonEx("Sync", device_panel_icons_size, ImGuiButtonFlags_Disabled);
        ImGui::SameLine(); ImGui::ButtonEx("Info", device_panel_icons_size);
        ImGui::SameLine(); ImGui::ButtonEx("More", device_panel_icons_size);
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(7);
        ImGui::PopFont();

        return device_panel_height;
    }

    template <typename T>
    std::string safe_call(T t)
    {
        try
        {
            t();
            return "";
        }
        catch (const error& e)
        {
            return error_to_string(e);
        }
        catch (const std::exception& e)
        {
            return e.what();
        }
        catch (...)
        {
            return "Unknown error occurred";
        }
    }

    void device_model::save_viewer_configurations(std::ofstream& outfile, json& j)
    {
        for (auto&& sub : subdevices)
        {
            int num_irs = 0;
            bool video_profile_saved = false;
            for (auto&& p : sub->get_selected_profiles())
            {
                rs2_stream stream_type = p.stream_type();
                std::string stream_format_key = to_string() << "stream-" << to_lower(rs2_stream_to_string(stream_type)) << "-format";
                std::string stream_format_value = rs2_format_to_string(p.format());

                if (stream_type == RS2_STREAM_DEPTH)
                {
                    stream_format_key = "stream-depth-format";
                }
                else if (stream_type == RS2_STREAM_INFRARED)
                {
                    stream_format_key = "stream-ir-format";
                    num_irs++;
                    if (num_irs == 2)
                    {
                        stream_format_value = "R8L8";
                    }
                }
                else
                {
                    continue; //TODO: Ignoring other streams for now
                }

                j[stream_format_key] = stream_format_value;
                if (!video_profile_saved)
                {
                    if (auto vp = p.as<rs2::video_stream_profile>())
                    {
                        j["stream-width"] = std::to_string(vp.width());
                        j["stream-height"] = std::to_string(vp.height());
                        j["stream-fps"] = std::to_string(vp.fps());
                        video_profile_saved = true;
                    }
                }
            }
        }
    }

    // Load viewer configuration for stereo module (depth/infrared streams) only 
    void device_model::load_viewer_configurations(const std::string& json_str)
    {
        json j = json::parse(json_str);
        struct video_stream
        {
            rs2_format format = RS2_FORMAT_ANY;
            int width = 0;
            int height = 0;
            int fps = 0;
        };

        std::map<std::pair<rs2_stream, int>, video_stream> requested_streams;
        auto it = j.find("stream-depth-format");
        if (it != j.end())
        {
            assert(it.value().is_string());
            std::string formatstr = it.value();
            bool found = false;
            for (int i = 0; i < static_cast<int>(RS2_FORMAT_COUNT); i++)
            {
                auto f = static_cast<rs2_format>(i);
                auto fstr = rs2_format_to_string(f);
                if (formatstr == fstr)
                {
                    requested_streams[std::make_pair(RS2_STREAM_DEPTH, 0)].format = f;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                throw std::runtime_error(to_string() << "Unsupported stream-depth-format: " << formatstr);
            }

            // Disable depth stream on all sub devices
            for (auto&& sub : subdevices)
            {
                int i = 0;
                for (auto&& profile : sub->profiles)
                {
                    if (profile.stream_type() == RS2_STREAM_DEPTH)
                    {
                        sub->stream_enabled[profile.unique_id()] = false;
                        break;
                    }
                }
            }
        }

        it = j.find("stream-ir-format");
        if (it != j.end())
        {
            assert(it.value().is_string());
            std::string formatstr = it.value();
            if (formatstr == "R8L8")
            {
                requested_streams[std::make_pair(RS2_STREAM_INFRARED, 1)].format = RS2_FORMAT_Y8;
                requested_streams[std::make_pair(RS2_STREAM_INFRARED, 2)].format = RS2_FORMAT_Y8;
            }
            else if (formatstr == "Y8")
            {
                requested_streams[std::make_pair(RS2_STREAM_INFRARED, 1)].format = RS2_FORMAT_Y8;
            }
            else if (formatstr == "UYVY")
            {
                requested_streams[std::make_pair(RS2_STREAM_INFRARED, 1)].format = RS2_FORMAT_UYVY;
            }
            else
            {
                throw std::runtime_error(to_string() << "Unsupported stream-ir-format: " << formatstr);
            }

            // Disable infrared stream on all sub devices
            for (auto&& sub : subdevices)
            {
                for (auto&& profile : sub->profiles)
                {
                    if (profile.stream_type() == RS2_STREAM_INFRARED)
                    {
                        sub->stream_enabled[profile.unique_id()] = false;
                        break;
                    }
                }
            }
        }

        // Setting the same Width,Height,FPS to every requested stereo module streams (depth,infrared) according to loaded JSON
        if (!requested_streams.empty())
        {
            try
            {
                std::string wstr = j["stream-width"];
                std::string hstr = j["stream-height"];
                std::string fstr = j["stream-fps"];
                int width = std::stoi(wstr);
                int height = std::stoi(hstr);
                int fps = std::stoi(fstr);
                for (auto& kvp : requested_streams)
                {
                    kvp.second.width = width;
                    kvp.second.height = height;
                    kvp.second.fps = fps;
                }
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(to_string() << "Error parsing streams from JSON: " << e.what());
            }
        }

        // Enable requested stereo module streams (depth,infrared)
        for (auto&& kvp : requested_streams)
        {
            std::string stream_name = to_string() << rs2_stream_to_string(kvp.first.first) << (kvp.first.second > 0 ? (" " + std::to_string(kvp.first.second)) : "");
            for (auto&& sub : subdevices)
            {
                auto itr = std::find_if(sub->stream_display_names.begin(), sub->stream_display_names.end(), [stream_name](const std::pair<int, std::string>& p) { return p.second == stream_name; });
                if (itr != sub->stream_display_names.end())
                {
                    int uid = itr->first;
                    sub->stream_enabled[uid] = true;

                    //Find format
                    int format_id = 0;
                    rs2_format requested_format = kvp.second.format;
                    for (; format_id < sub->format_values[uid].size(); format_id++)
                    {
                        if (sub->format_values[uid][format_id] == requested_format)
                            break;
                    }
                    if (format_id == sub->format_values[uid].size())
                    {
                        throw std::runtime_error(to_string() << "No match found for requested format: " << requested_format);
                    }
                    sub->ui.selected_format_id[uid] = format_id;

                    //Find fps
                    int requested_fps = kvp.second.fps;
                    int fps_id = 0;
                    for (; fps_id < sub->shared_fps_values.size(); fps_id++)
                    {
                        if (sub->shared_fps_values[fps_id] == requested_fps)
                            break;
                    }
                    if (fps_id == sub->shared_fps_values.size())
                    {
                        throw std::runtime_error(to_string() << "No match found for requested fps: " << requested_fps);
                    }
                    sub->ui.selected_shared_fps_id = fps_id;

                    //Find Resolution
                    std::pair<int, int> requested_res{ kvp.second.width,kvp.second.height };
                    int res_id = 0;
                    for (; res_id < sub->res_values.size(); res_id++)
                    {
                        if (sub->res_values[res_id] == requested_res)
                            break;
                    }
                    if (res_id == sub->res_values.size())
                    {
                        throw std::runtime_error(to_string() << "No match found for requested resolution: " << requested_res.first << "x" << requested_res.second);
                    }
                    sub->ui.selected_res_id = res_id;
                }
            }
        }
    }

    // Generic helper functions for comparison of fw versions
    std::vector<int> fw_version_to_int_vec(std::string fw_version)
    {
        size_t start{}, end{};
        std::vector<int> values;
        std::string delimiter(".");
        std::string substr;
        while ((end = fw_version.find(delimiter, start)) != std::string::npos)
        {
            substr = fw_version.substr(start, end-start);
            start = start + substr.length() + delimiter.length();
            values.push_back(atoi(substr.c_str()));
        }
        values.push_back(atoi(fw_version.substr(start, fw_version.length() - start).c_str()));
        return values;
    }

    bool fw_version_less_than(std::string fw_version, std::string min_fw_version)
    {
        std::vector<int> curr_values = fw_version_to_int_vec(fw_version);
        std::vector<int> min_values = fw_version_to_int_vec(min_fw_version);

        for (int i = 0; i < curr_values.size(); i++)
        {
            if (i >= min_values.size())
            {
                return false;
            }
            if (curr_values[i] < min_values[i])
            {
                return true;
            }
            if (curr_values[i] > min_values[i])
            {
                return false;
            }
        }
        return false;
    }

    float device_model::draw_preset_panel(float panel_width,
        ux_window& window,
        std::string& error_message,
        viewer_model& viewer,
        bool update_read_only_options,
        bool load_json_if_streaming,
        json_loading_func json_loading)
    {
        const float panel_height = 40.f;
        auto panel_pos = ImGui::GetCursorPos();
        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::PushFont(window.get_font());

        const auto load_json = [&](const std::string f) {
            std::ifstream file(f);
            if (!file.good())
            {
                //Failed to read file, removing it from the available ones
                advanced_mode_settings_file_names.erase(f);
                selected_file_preset.clear();
                throw std::runtime_error(to_string() << "Failed to read configuration file:\n\"" << f << "\"\nRemoving it from presets.");
            }
            std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (auto advanced = dev.as<advanced_mode>())
            {
                advanced.load_json(str);
                for (auto&& sub : subdevices)
                {
                    //If json was loaded correctly, we want the presets combo box to show the name of the configuration file
                    // And as a workaround, set the current preset to "custom", so that if the file is removed the preset will show "custom"
                    if (auto dpt = sub->s->as<depth_sensor>())
                    {
                        auto itr = sub->options_metadata.find(RS2_OPTION_VISUAL_PRESET);
                        if (itr != sub->options_metadata.end())
                        {
                            //TODO: Update to work with SR300 when the load json will update viewer configurations
                            itr->second.endpoint->set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_CUSTOM);
                        }
                    }
                }
            }
            load_viewer_configurations(str);
            get_curr_advanced_controls = true;
            advanced_mode_settings_file_names.insert(f);
            selected_file_preset = f;
            viewer.not_model.add_log(to_string() << "Loaded settings from \"" << f << "\"...");
        };

        const auto save_to_json = [&](std::string full_filename)
        {
            auto advanced = dev.as<advanced_mode>();
            if (!ends_with(to_lower(full_filename), ".json")) full_filename += ".json";
            std::ofstream outfile(full_filename);
            json saved_configuraion;
            if (auto advanced = dev.as<advanced_mode>())
            {
                saved_configuraion = json::parse(advanced.serialize_json());
            }
            save_viewer_configurations(outfile, saved_configuraion);
            outfile << saved_configuraion.dump(4);
            outfile.close();
            advanced_mode_settings_file_names.insert(full_filename);
            selected_file_preset = full_filename;
            viewer.not_model.add_log(to_string() << "Saved settings to \"" << full_filename << "\"...");

        };
        static const std::string popup_message = "\t\tTo use this feature, the device must be in Advanced Mode.\t\t\n\n\t\tWould you like to turn Advanced Mode?\t\t";
        ////////////////////////////////////////
        // Draw Combo Box
        ////////////////////////////////////////
        for (auto&& sub : subdevices)
        {
            if (auto dpt = sub->s->as<depth_sensor>())
            {
                ImGui::SetCursorPos({ panel_pos.x + 8, ImGui::GetCursorPosY() + 10 });
                //TODO: set this once!
                const auto draw_preset_combo_box = [&](option_model& opt_model, std::string& error_message, notifications_model& model)
                {
                    bool is_clicked = false;
                    assert(opt_model.opt == RS2_OPTION_VISUAL_PRESET);
                    ImGui::Text("Presets: ");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Select a preset configuration (or use the load button)");
                    }

                    ImGui::SameLine();
                    ImGui::PushItemWidth(185);

                    ///////////////////////////////////////////
                    //TODO: make this a member function
                    std::vector<const char*> labels;
                    auto selected = 0, counter = 0;
                    for (auto i = opt_model.range.min; i <= opt_model.range.max; i += opt_model.range.step, counter++)
                    {
                        if (std::fabs(i - opt_model.value) < 0.001f)
                        {
                            selected = counter;
                        }
                        labels.push_back(opt_model.endpoint->get_option_value_description(opt_model.opt, i));
                    }
                    ///////////////////////////////////////////

                    ImGui_ScopePushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui_ScopePushStyleColor(ImGuiCol_Button, button_color);
                    ImGui_ScopePushStyleColor(ImGuiCol_ButtonHovered, button_color + 0.1f);
                    ImGui_ScopePushStyleColor(ImGuiCol_ButtonActive, button_color + 0.1f);
                    ImVec2 padding{ 2,2 };
                    ImGui_ScopePushStyleVar(ImGuiStyleVar_FramePadding, padding);
                    ///////////////////////////////////////////
                    // Go over the loaded files and add them to the combo box
                    std::vector<std::string> full_files_names(advanced_mode_settings_file_names.begin(), advanced_mode_settings_file_names.end());
                    std::vector<std::string> files_labels;
                    int i = static_cast<int>(labels.size());
                    for (auto&& file : full_files_names)
                    {
                        files_labels.push_back(get_file_name(file));
                        if (selected_file_preset == file)
                        {
                            selected = i;
                        }
                        i++;
                    }
                    std::transform(files_labels.begin(), files_labels.end(), std::back_inserter(labels), [](const std::string& s) { return s.c_str(); });

                    try
                    {
                        static bool keep_showing_popup = false;
                        if (ImGui::Combo(opt_model.id.c_str(), &selected, labels.data(),
                            static_cast<int>(labels.size())))
                        {
                            auto advanced = dev.as<advanced_mode>();
                            if (advanced)
                            {
                                if (advanced.is_enabled())
                                {
                                    if (selected < static_cast<int>(labels.size() - files_labels.size()))
                                    {
                                        //Known preset was chosen
                                        opt_model.value = opt_model.range.min + opt_model.range.step * selected;
                                        model.add_log(to_string() << "Setting " << opt_model.opt << " to "
                                            << opt_model.value << " (" << labels[selected] << ")");
                                        opt_model.endpoint->set_option(opt_model.opt, opt_model.value);
                                        is_clicked = true;
                                        selected_file_preset = "";
                                    }
                                    else
                                    {
                                        //File was chosen
                                        auto f = full_files_names[selected - static_cast<int>(labels.size() - files_labels.size())];
                                        error_message = safe_call([&]() { load_json(f); });
                                        selected_file_preset = f;
                                    }
                                }
                                else
                                {
                                    keep_showing_popup = true;
                                }
                            }
                        }
                        if (keep_showing_popup)
                        {
                            keep_showing_popup = prompt_toggle_advanced_mode(true, popup_message, restarting_device_info, viewer, window);
                        }
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }

                    ImGui::PopItemWidth();
                    return is_clicked;
                };
                sub->options_metadata[RS2_OPTION_VISUAL_PRESET].custom_draw_method = draw_preset_combo_box;
                if (sub->draw_option(RS2_OPTION_VISUAL_PRESET, dev.is<playback>() || update_read_only_options, error_message, viewer.not_model))
                {
                    get_curr_advanced_controls = true;
                    selected_file_preset.clear();
                }
            }
        }

        ImGui::SameLine();
        const ImVec2 icons_size{ 20, 20 };
        //TODO: Change this once we have support for loading jsons with more data than only advanced controls
        bool is_streaming = std::any_of(subdevices.begin(), subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm) { return sm->streaming; });
        const int buttons_flags = dev.is<advanced_mode>() ? 0 : ImGuiButtonFlags_Disabled;
        static bool require_advanced_mode_enable_prompt = false;
        auto advanced_dev = dev.as<advanced_mode>();
        bool is_advanced_mode_enabled = false;
        if (advanced_dev)
        {
            is_advanced_mode_enabled = advanced_dev.is_enabled();
        }
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);

        ////////////////////////////////////////
        // Draw Load Icon
        ////////////////////////////////////////
        std::string upload_button_name = to_string() << textual_icons::upload << "##" << id;
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);

        if (ImGui::ButtonEx(upload_button_name.c_str(), icons_size, (is_streaming && !load_json_if_streaming) ? ImGuiButtonFlags_Disabled : buttons_flags))
        {
            if (is_advanced_mode_enabled)
            {
                json_loading([&]()
                {
                    auto ret = file_dialog_open(open_file, "JavaScript Object Notation (JSON)\0*.json\0", NULL, NULL);
                    if (ret)
                    {
                        error_message = safe_call([&]() { load_json(ret); });
                    }
                });
            }
            else
            {
                require_advanced_mode_enable_prompt = true;
            }
        }

        if (ImGui::IsItemHovered())
        {
            std::string tooltip = to_string() << "Load pre-configured stereo module settings" << (is_streaming && !load_json_if_streaming ? " (Disabled while streaming)" : "");
            ImGui::SetTooltip("%s", tooltip.c_str());
        }

        ImGui::SameLine();

        ////////////////////////////////////////
        // Draw Save Icon
        ////////////////////////////////////////
        std::string save_button_name = to_string() << textual_icons::download << "##" << id;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1); //Align the two icons to buttom
        if (ImGui::ButtonEx(save_button_name.c_str(), icons_size, buttons_flags))
        {
            if (is_advanced_mode_enabled)
            {
                auto ret = file_dialog_open(save_file, "JavaScript Object Notation (JSON)\0*.json\0", NULL, NULL);
                if (ret)
                {
                    error_message = safe_call([&]() { save_to_json(ret); });
                }
            }
            else
            {
                require_advanced_mode_enable_prompt = true;
            }

        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save current stereo module settings to file");
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        if (require_advanced_mode_enable_prompt)
        {
            require_advanced_mode_enable_prompt = prompt_toggle_advanced_mode(true, popup_message, restarting_device_info, viewer, window);
        }

        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(7);

        return panel_height;
    }


    void device_model::draw_controls(float panel_width, float panel_height,
        ux_window& window,
        std::string& error_message,
        device_model*& device_to_remove,
        viewer_model& viewer, float windows_width,
        bool update_read_only_options,
        std::vector<std::function<void()>>& draw_later,
        bool load_json_if_streaming,
        json_loading_func json_loading,
        bool draw_device_outline)
    {
        ////////////////////////////////////////
        // draw device header
        ////////////////////////////////////////
        const bool is_playback_device = dev.is<playback>();
        auto header_h = panel_height;
        if (is_playback_device) header_h += 15;

        ImColor device_header_background_color = title_color;
        const float left_space = 3.f;
        const float upper_space = 3.f;

        const ImVec2 initial_screen_pos = ImGui::GetCursorScreenPos();
        //Upper Space
        ImGui::GetWindowDrawList()->AddRectFilled({ initial_screen_pos.x,initial_screen_pos.y }, { initial_screen_pos.x + panel_width,initial_screen_pos.y + upper_space }, ImColor(black));
        if (draw_device_outline)
        {
            //Upper Line
            ImGui::GetWindowDrawList()->AddLine({ initial_screen_pos.x,initial_screen_pos.y + upper_space }, { initial_screen_pos.x + panel_width,initial_screen_pos.y + upper_space }, ImColor(header_color));
        }
        //Device Header area
        ImGui::GetWindowDrawList()->AddRectFilled({ initial_screen_pos.x + 1,initial_screen_pos.y + upper_space + 1 }, { initial_screen_pos.x + panel_width, initial_screen_pos.y + header_h + upper_space }, device_header_background_color);

        auto pos = ImGui::GetCursorPos();
        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_Button, device_header_background_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, device_header_background_color);

        ////////////////////////////////////////
        // Draw device name
        ////////////////////////////////////////
        const ImVec2 name_pos = { pos.x + 9, pos.y + 17 };
        ImGui::SetCursorPos(name_pos);
        std::stringstream ss;
        ss << dev.get_info(RS2_CAMERA_INFO_NAME);
        if (dev.supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
            ss << "   " << textual_icons::usb_type << " " << dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);

        ImGui::Text(" %s", ss.str().c_str());
        //ImGui::Text(" %s", dev.get_info(RS2_CAMERA_INFO_NAME));
        ImGui::PopFont();

        ////////////////////////////////////////
        // Draw X Button
        ////////////////////////////////////////
        ImGui::PushFont(window.get_font());
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        if (allow_remove)
        {
            ImGui::Columns(1);
            float horizontal_distance_from_right_side_of_panel = 47;
            ImGui::SetCursorPos({ panel_width - horizontal_distance_from_right_side_of_panel, pos.y + 9 + (header_h - panel_height) / 2 });
            std::string remove_source_button_label = to_string() << textual_icons::times << "##" << id;
            if (ImGui::Button(remove_source_button_label.c_str(), { 33,35 }))
            {
                for (auto&& sub : subdevices)
                {
                    if (sub->streaming)
                        sub->stop(viewer);
                }
                device_to_remove = this;
            }
        }
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
        ImGui::PopFont();

        ////////////////////////////////////////
        // Draw playback file name
        ////////////////////////////////////////
        ImGui::SetCursorPos({ pos.x + 10, pos.y + panel_height - 9 });
        if (auto p = dev.as<playback>())
        {
            ImGui::PushFont(window.get_font());
            auto full_path = p.file_name();
            auto filename = get_file_name(full_path);
            std::string file_name_and_icon = to_string() << " " << textual_icons::file_movie << " File: \"" << filename << "\"";
            ImGui::Text("%s", file_name_and_icon.c_str());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", full_path.c_str());
            ImGui::PopFont();
        }
        ImGui::SetCursorPos({ 0, pos.y + header_h });

        ////////////////////////////////////////
        // draw device control panel
        ////////////////////////////////////////
        if (!is_playback_device) //Not displaying these controls for playback devices (since only info is supported)
        {
            pos = ImGui::GetCursorPos();
            const float vertical_space_before_device_control = 10.0f;
            const float horizontal_space_before_device_control = 3.0f;
            auto device_panel_pos = ImVec2{ pos.x + horizontal_space_before_device_control, pos.y + vertical_space_before_device_control };
            ImGui::SetCursorPos(device_panel_pos);
            const float device_panel_height = draw_device_panel(panel_width, window, error_message, viewer);
            ImGui::SetCursorPos({ device_panel_pos.x, device_panel_pos.y + device_panel_height });
        }

        ////////////////////////////////////////
        // draw advanced mode panel
        ////////////////////////////////////////
        if (dev.is<advanced_mode>())
        {
            pos = ImGui::GetCursorPos();
            const float vertical_space_before_advanced_mode_control = 10.0f;
            const float horizontal_space_before_device_control = 3.0f;
            auto advanced_mode_pos = ImVec2{ pos.x + horizontal_space_before_device_control, pos.y + vertical_space_before_advanced_mode_control };
            ImGui::SetCursorPos(advanced_mode_pos);
            const float advanced_mode_panel_height = draw_preset_panel(panel_width, window, error_message, viewer, update_read_only_options, load_json_if_streaming, json_loading);
            ImGui::SetCursorPos({ advanced_mode_pos.x, advanced_mode_pos.y + advanced_mode_panel_height });
        }

        ////////////////////////////////////////
        // draw playback control panel
        ////////////////////////////////////////
        if (auto p = dev.as<playback>())
        {
            pos = ImGui::GetCursorPos();
            float space_before_playback_control = 18.0f;
            auto playback_panel_pos = ImVec2{ pos.x + 10, pos.y + space_before_playback_control };
            ImGui::SetCursorPos(playback_panel_pos);
            auto playback_panel_height = draw_playback_panel(window.get_font(), viewer);
            ImGui::SetCursorPos({ playback_panel_pos.x, playback_panel_pos.y + playback_panel_height });
        }

        bool is_streaming = std::any_of(subdevices.begin(), subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm)
        {
            return sm->streaming;
        });
        draw_controllers_panel(window.get_font(), is_streaming);

        pos = ImGui::GetCursorPos();

        ImVec2 rc;
        std::string fw_version;
        std::string min_fw_version;

        int info_control_panel_height = 0;
        if (show_device_info)
        {
            ImGui::PushFont(window.get_font());
            int line_h = 22;
            info_control_panel_height = (int)infos.size() * line_h + 5;
            for (auto&& pair : infos)
            {
                rc = ImGui::GetCursorPos();
                ImGui::SetCursorPos({ rc.x + 12, rc.y + 4 });
                std::string info_category;
                if (pair.first == "Recommended Firmware Version")
                {
                    info_category = "Min FW Version";
                    min_fw_version = pair.second;
                }
                else
                {
                    info_category = pair.first.c_str();
                }
                ImGui::Text("%s:", info_category.c_str());
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::SetCursorPos({ rc.x + 145, rc.y + 1 });
                std::string label = to_string() << "##" << id << " " << pair.first;
                if (pair.first == "Firmware Version")
                {
                    fw_version = pair.second;
                    ImGui::PushItemWidth(80);
                }
                ImGui::InputText(label.c_str(),
                    (char*)pair.second.data(),
                    pair.second.size() + 1,
                    ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                if (pair.first == "Firmware Version")
                {
                    ImGui::PopItemWidth();
                }
                ImGui::PopStyleColor(3);
                ImGui::SetCursorPos({ rc.x, rc.y + line_h });
            }

            ImGui::SetCursorPos({ rc.x + 225, rc.y - 127 });

            if (fw_version_less_than(fw_version, min_fw_version))
            {
                std::string label1 = to_string() << textual_icons::exclamation_triangle << "##" << id;
                ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_Text, yellow);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, yellow + 0.1f);

                if (ImGui::SmallButton(label1.c_str()))
                {
                    open_url(recommended_fw_url);
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Click here to update firmware\n(internet connection required)");
                }

                ImGui::PopStyleColor(5);
            }

            ImGui::PopFont();
        }

        ImGui::SetCursorPos({ 0, pos.y + info_control_panel_height });
        ImGui::PopStyleColor(2);

        auto sensor_top_y = ImGui::GetCursorPosY();
        ImGui::SetContentRegionWidth(windows_width - 36);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
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
                    ImGui_ScopePushFont(window.get_font());

                    ImGui_ScopePushStyleColor(ImGuiCol_Button, sensor_bg);
                    ImGui_ScopePushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                    ImGui_ScopePushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                    if (!sub->streaming)
                    {
                        std::string label = to_string() << "  " << textual_icons::toggle_off << "\noff   ##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME);

                        ImGui_ScopePushStyleColor(ImGuiCol_Text, redish);
                        ImGui_ScopePushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

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
                            std::string text = to_string() << "  " << textual_icons::toggle_off << "\noff   ";
                            ImGui::TextDisabled("%s", text.c_str());
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
                        std::string label = to_string() << "  " << textual_icons::toggle_on << "\n    on##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME);
                        ImGui_ScopePushStyleColor(ImGuiCol_Text, light_blue);
                        ImGui_ScopePushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                        if (ImGui::Button(label.c_str(), { 30,30 }))
                        {
                            sub->stop(viewer);

                            if (!std::any_of(subdevices.begin(), subdevices.end(),
                                [](const std::shared_ptr<subdevice_model>& sm)
                            {
                                return sm->streaming;
                            }))
                            {
                                // Stopping post processing filter rendering thread
                                viewer.ppf.stop();
                                stop_recording = true;
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Stop streaming data from selected sub-device");
                        }
                    }

                    if (is_recording && stop_recording)
                    {
                        this->stop_recording(viewer);
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

            //ImGui::GetWindowDrawList()->AddLine({ abs_pos.x, abs_pos.y - 1 },
            //{ abs_pos.x + panel_width, abs_pos.y - 1 },
            //    ImColor(black), 1.f);

            std::string label = to_string() << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "##" << id;
            ImGui::PushStyleColor(ImGuiCol_Header, sensor_bg);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, sensor_bg);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sensor_bg);
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

                static const std::vector<rs2_option> drawing_order = dev.is<advanced_mode>() ?
                    std::vector<rs2_option>{                           RS2_OPTION_EMITTER_ENABLED, RS2_OPTION_ENABLE_AUTO_EXPOSURE }
                  : std::vector<rs2_option>{ RS2_OPTION_VISUAL_PRESET, RS2_OPTION_EMITTER_ENABLED, RS2_OPTION_ENABLE_AUTO_EXPOSURE };

                for (auto& opt : drawing_order)
                {
                    if (sub->draw_option(opt, dev.is<playback>() || update_read_only_options, error_message, viewer.not_model))
                    {
                        get_curr_advanced_controls = true;
                        selected_file_preset.clear();
                    }
                }

                if (sub->num_supported_non_default_options())
                {
                    label = to_string() << "Controls ##" << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << id;
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                        {
                            auto opt = static_cast<rs2_option>(i);
                            if (opt == RS2_OPTION_FRAMES_QUEUE_SIZE) continue;
                            if (std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
                            {
                                if (dev.is<advanced_mode>() && opt == RS2_OPTION_VISUAL_PRESET)
                                    continue;
                                if (sub->draw_option(opt, dev.is<playback>() || update_read_only_options, error_message, viewer.not_model))
                                {
                                    get_curr_advanced_controls = true;
                                    selected_file_preset.clear();
                                }
                            }
                        }

                        ImGui::TreePop();
                    }
                }
                if (dev.is<advanced_mode>() && sub->s->is<depth_sensor>())
                {
                    if (draw_advanced_controls(viewer, window, error_message))
                    {
                        sub->options_invalidated = true;
                        selected_file_preset.clear();
                    }
                }


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

                    draw_later.push_back([windows_width, &window, sub, pos, &viewer, this]() {
                        if (!sub->streaming) ImGui::SetCursorPos({ windows_width -27 , pos.y - 1 });
                        else
                            ImGui::SetCursorPos({ windows_width - 32, pos.y - 3 });

                        ImGui::PushFont(window.get_font());

                        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                        if (!sub->streaming)
                        {
                            if (!sub->post_processing_enabled)
                            {
                                std::string label = to_string() << textual_icons::toggle_off;

                                ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);
                                ImGui::TextDisabled("%s", label.c_str());
                            }
                            else
                            {
                                std::string label = to_string() << textual_icons::toggle_on;
                                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);
                                ImGui::TextDisabled("%s", label.c_str());
                            }
                        }
                        else
                        {
                            if (!sub->post_processing_enabled)
                            {
                                std::string label = to_string() << " " << textual_icons::toggle_off << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << ",post";

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
                                std::string label = to_string() << " " << textual_icons::toggle_on << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << ",post";
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
                                if (!sub->streaming || !sub->post_processing_enabled) ImGui::SetCursorPos({ windows_width - 27, pos.y + 1 });
                                else
                                    ImGui::SetCursorPos({ windows_width - 35, pos.y - 3 });

                                ImGui::PushFont(window.get_font());

                                ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                                if (!sub->streaming || !sub->post_processing_enabled)
                                {
                                    if (!pb->enabled)
                                    {
                                        std::string label = to_string() << textual_icons::toggle_off;

                                        ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);
                                        ImGui::TextDisabled("%s", label.c_str());
                                    }
                                    else
                                    {
                                        std::string label = to_string() << textual_icons::toggle_on;
                                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);
                                        ImGui::TextDisabled("%s", label.c_str());
                                    }
                                }
                                else
                                {
                                    if (!pb->enabled)
                                    {
                                        std::string label = to_string() << " " << textual_icons::toggle_off << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();

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
                                        std::string label = to_string() << " " << textual_icons::toggle_on << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();
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
            ImGui::PopStyleColor(3);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        }

        for (auto&& sub : subdevices)
        {
            sub->update(error_message, viewer.not_model);
        }

        ImGui::PopStyleColor(2);
        ImGui::PopFont();

        auto end_screen_pos = ImGui::GetCursorScreenPos();

        if (draw_device_outline)
        {
            //Left space
            ImGui::GetWindowDrawList()->AddRectFilled({ initial_screen_pos.x,initial_screen_pos.y }, { end_screen_pos.x + left_space, end_screen_pos.y }, ImColor(black));
            //Left line
            ImGui::GetWindowDrawList()->AddLine({ initial_screen_pos.x + left_space,initial_screen_pos.y + upper_space }, { end_screen_pos.x + left_space, end_screen_pos.y }, ImColor(header_color));
            //Right line
            const float compenstaion_right = 17.f;;
            ImGui::GetWindowDrawList()->AddLine({ initial_screen_pos.x + panel_width - compenstaion_right, initial_screen_pos.y + upper_space }, { end_screen_pos.x + panel_width - compenstaion_right, end_screen_pos.y }, ImColor(header_color));
            //Button line
            const float compenstaion_button = 1.0f;
            ImGui::GetWindowDrawList()->AddLine({ end_screen_pos.x + left_space, end_screen_pos.y - compenstaion_button }, { end_screen_pos.x + left_space + panel_width, end_screen_pos.y - compenstaion_button }, ImColor(header_color));
        }
    }

    void device_model::handle_harware_events(const std::string& serialized_data)
    {
        //TODO: Move under hour glass
        std::string event_type = get_event_type(serialized_data);
        if (event_type == "Controller Event")
        {
            std::string subtype = get_subtype(serialized_data);
            if (subtype == "Connection")
            {
                std::array<uint8_t, 6> mac_addr = get_mac(serialized_data);
                int id = get_id(serialized_data);
                controllers[id] = mac_addr;
                available_controllers.erase(mac_addr);
            }
            else if (subtype == "Discovery")
            {
                std::array<uint8_t, 6> mac_addr = get_mac(serialized_data);
                available_controllers.insert(mac_addr);
            }
            else if (subtype == "Disconnection")
            {
                int id = get_id(serialized_data);
                controllers.erase(id);
            }
        }
    }


    void viewer_model::draw_viewport(const rect& viewer_rect, ux_window& window, int devices, std::string& error_message, texture_buffer* texture, points points)
    {
        if (!is_3d_view)
        {
            render_2d_view(viewer_rect, window,
                static_cast<int>(get_output_height()), window.get_font(), window.get_large_font(),
                devices, window.get_mouse(), error_message);
        }
        else
        {
            if (paused)
                show_paused_icon(window.get_large_font(), static_cast<int>(panel_width + 15), static_cast<int>(panel_y + 15 + 32), 0);

            show_3dviewer_header(window.get_font(), viewer_rect, paused, error_message);

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
        category = n._category;
    }

    double notification_model::get_age_in_ms() const
    {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - created_time).count();
    }

    // Pops the 6 colors that were pushed in set_color_scheme
    void notification_model::unset_color_scheme() const
    {
        ImGui::PopStyleColor(6);
    }

    /* Sets color scheme for notifications, must be used with unset_color_scheme to pop all colors in the end
       Parameter t indicates the transparency of the nofication interface */
    void notification_model::set_color_scheme(float t) const
    {
        ImGui::PushStyleColor(ImGuiCol_CloseButton, { 0, 0, 0, 0 });
        ImGui::PushStyleColor(ImGuiCol_CloseButtonActive, { 0, 0, 0, 0 });
        if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 33/255.f, 40/255.f, 46/255.f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_TitleBg, { 62 / 255.f, 77 / 255.f, 89 / 255.f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 62 / 255.f, 77 / 255.f, 89 / 255.f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_CloseButtonHovered, { 62 / 255.f + 0.1f, 77 / 255.f + 0.1f, 89 / 255.f + 0.1f, 1 - t });
        }
        else
        {
            if (severity == RS2_LOG_SEVERITY_ERROR ||
                severity == RS2_LOG_SEVERITY_WARN)
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.f, 0.f, 1 - t });
                ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.5f, 0.2f, 0.2f, 1 - t });
                ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.6f, 0.2f, 0.2f, 1 - t });
                ImGui::PushStyleColor(ImGuiCol_CloseButtonHovered, { 0.5f + 0.1f, 0.2f + 0.1f, 0.2f + 0.1f, 1 - t });
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.3f, 0.3f, 1 - t });
                ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.4f, 0.4f, 0.4f, 1 - t });
                ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.6f, 0.6f, 0.6f, 1 - t });
                ImGui::PushStyleColor(ImGuiCol_CloseButtonHovered, { 0.6f + 0.1f, 0.6f + 0.1f, 0.6f + 0.1f, 1 - t });
            }
        }
    }

    const int notification_model::get_max_lifetime_ms() const
    {
        if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
        {
            return 30000;
        }
        return 10000;
    }

    void notification_model::draw(int w, int y, notification_model& selected)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        auto ms = get_age_in_ms() / get_max_lifetime_ms();
        auto t = smoothstep(static_cast<float>(ms), 0.7f, 1.0f);

        set_color_scheme(t);
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 - t });

        auto lines = static_cast<int>(std::count(message.begin(), message.end(), '\n') + 1);
        ImGui::SetNextWindowPos({ float(w - 330), float(y) });
        height = lines * 30 + 20;
        ImGui::SetNextWindowSize({ float(315), float(height) });

        bool opened = true;
        std::string label;

        if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
        {
            label = to_string() << "Firmware update recommended" << "##" << index;
        }
        else
        {
            label = to_string() << "Hardware Notification #" << index;
        }

        ImGui::Begin(label.c_str(), &opened, flags);

        if (!opened)
            to_close = true;

        if (category == RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
        {
            std::regex version_regex("([0-9]+.[0-9]+.[0-9]+.[0-9]+\n)");
            std::smatch sm;
            std::regex_search(message, sm, version_regex);
            std::string message_prefix = sm.prefix();
            std::string curr_version = sm.str();
            std::string message_suffix = sm.suffix();
            ImGui::Text("%s", message_prefix.c_str());
            ImGui::SameLine(0, 0);
            ImGui::PushStyleColor(ImGuiCol_Text, { (float)255 / 255, (float)46 / 255, (float)54 / 255, 1 - t });
            ImGui::Text("%s", curr_version.c_str());
            ImGui::PopStyleColor();
            ImGui::Text("%s", message_suffix.c_str());

            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1, 1, 1, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_Button, { 62 / 255.f, 77 / 255.f, 89 / 255.f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 62 / 255.f + 0.1f, 77 / 255.f + 0.1f, 89 / 255.f + 0.1f, 1 - t });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 62 / 255.f - 0.1f, 77 / 255.f - 0.1f, 89 / 255.f - 0.1f, 1 - t });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);
            std::string button_name = to_string() << "Download update" << "##" << index;
            ImGui::Indent(80);
            if (ImGui::Button(button_name.c_str(), { 130, 30 }))
            {
                open_url(recommended_fw_url);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", "Internet connection required");
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
        }
        else
        {
            ImGui::Text("%s", message.c_str());
        }

        if (lines == 1)
            ImGui::SameLine();

        if (category != RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED)
        {
            ImGui::Text("(...)");

            if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered())
            {
                selected = *this;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(1);
        unset_color_scheme();
        ImGui::PopStyleVar();
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
                return (n.get_age_in_ms() > n.get_max_lifetime_ms() || n.to_close);
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
        ImGui::PopStyleColor(3);
        selected.unset_color_scheme();
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
    void tm2_model::draw_controller_pose_object()
    {
        const float sphere_radius = 0.02f;
        const float controller_height = 0.2f;
        //TODO: Draw controller holder as cylinder
        texture_buffer::draw_circle(1, 0, 0, 0, 1, 0, sphere_radius, { 0.0, controller_height + sphere_radius, 0.0 }, 1.0f);
        texture_buffer::draw_circle(0, 1, 0, 0, 0, 1, sphere_radius, { 0.0, controller_height + sphere_radius, 0.0 }, 1.0f);
        texture_buffer::draw_circle(1, 0, 0, 0, 0, 1, sphere_radius, { 0.0, controller_height + sphere_radius, 0.0 }, 1.0f);
    }

    void tm2_model::draw_pose_object()
    {
        if (!camera_object_button.is_pressed()) // draw camera box
        {
            glBegin(GL_QUADS);
            for (auto&& colored_face : camera_box)
            {
                auto& c = colored_face.second;
                glColor3f(c[0], c[1], c[2]);
                for (auto&& v : colored_face.first)
                {
                    glVertex3f(v.x, v.y, v.z);
                }
            }
            glEnd();

            texture_buffer::draw_circle(1, 0, 0, 0, 1, 0, lens_radius, center_left, 1.0f);
            texture_buffer::draw_circle(1, 0, 0, 0, 1, 0, lens_radius, center_right, 1.0f);
        }
        else //draw axis
        {
            texture_buffer::draw_axis(0.1f, 1.f);
        }
    }

    void tm2_model::draw_trajectory(tracked_point& p)
    {
        if (!trajectory_button.is_pressed())
        {
            if (trajectory.size() > 0)
            {
                //cleanup last trajectory
                trajectory.clear();
            }
            return;
        }
        add_to_trajectory(p);

        glLineWidth(3.0f);
        glBegin(GL_LINE_STRIP);
        for (auto&& v : trajectory)
        {
            switch (v.second) //color the line according to confidence
            {
            case 3:
                glColor3f(0.0f, 1.0f, 0.0f); //green
                break;
            case 2:
                glColor3f(1.0f, 1.0f, 0.0f); //yellow
                break;
            case 1:
                glColor3f(1.0f, 0.0f, 0.0f); //red
                break;
            case 0:
                glColor3f(0.7f, 0.7f, 0.7f); //grey - failed pose
                break;
            default:
                throw std::runtime_error("Invalid pose confidence value");
            }
            glVertex3f(v.first.x, v.first.y, v.first.z);
        }
        glEnd();
    }

    void tm2_model::add_to_trajectory(tracked_point& p)
    {
        //insert first element anyway
        if (trajectory.size() == 0)
        {
            trajectory.push_back(p);
        }
        else
        {
            //check if new element is far enough - more than 1 mm
            rs2_vector prev = trajectory.back().first;
            rs2_vector curr = p.first;
            if (sqrt(pow((curr.x - prev.x), 2) + pow((curr.y - prev.y), 2) + pow((curr.z - prev.z), 2)) < 0.001)
            {
                //if too close - check confidence and replace element
                if (p.second > trajectory.back().second)
                {
                    trajectory.back() = p;
                }
                //else - discard this sample
            }
            else
            {
                //sample is far enough - keep it
                trajectory.push_back(p);
            }
        }
    }

    void tm2_model::draw_boundary(tracked_point& p)
    {
        if (!boundary_button.is_pressed())
        {
            //TODO - separate button
            if (boundary.size() > 0)
            {
                //cleanup last boundary
                boundary.clear();
            }
            return;
        }

        // if new boundary - grab from trajectory
        if (boundary.size() == 0)
        {
            std::vector<float2> trajectory_projection;
            //create the boundary from the trajectory
            for (auto&& v : trajectory)
            {
                // project the trajectory on XZ plane - ignore y coordinate of the point
                float2 p{ v.first.x, v.first.z };
                trajectory_projection.push_back(p);
            }
            boundary = simplify_line(trajectory_projection);
        }
        // check if there is any boundary to render
        if (boundary.size() == 0)
        {
            return;
        }

        // check if the current position is inside or outside the boundary, to color it accordingly
        float2 point{ p.first.x, p.first.z };
        bool inside = point_in_polygon_2D(boundary, point);
        color c;
        if (inside)
        {
            c = { 0.0f, 1.0f, 0.0f };
        }
        else
        {
            c = { 1.0f, 0.0f, 0.0f };
        }
        // draw the boundary lines parallel to XZ plane
        glLineWidth(1.0f);
        for (float height = -1.0f; height < 1.0f; height += 0.2f)
        {
            glBegin(GL_LINE_STRIP);
            glColor3f(c[0], c[1], c[2]);
            for (auto&& v : boundary)
            {
                glVertex3f(v.x, height, v.y);
            }
            glVertex3f(boundary[0].x, height, boundary[0].y);
            glEnd();
        }

        // draw vertical lines along the boundary
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glColor3f(c[0], c[1], c[2]);
        for (auto&& v : boundary)
        {
            glVertex3f(v.x, -1.0f, v.y);
            glVertex3f(v.x, 1.0f, v.y);
        }
        glEnd();
    }

    std::string get_timestamped_file_name()
    {
        std::time_t now = std::time(NULL);
        std::tm * ptm = std::localtime(&now);
        char buffer[16];
        // Format: 20170529_205500
        std::strftime(buffer, 16, "%Y%m%d_%H%M%S", ptm);
        return buffer;
    }
    std::string get_folder_path(special_folder f)
    {
        std::string res;
#ifdef _WIN32
        if (f == temp_folder)
        {
            TCHAR buf[MAX_PATH];
            if (GetTempPath(MAX_PATH, buf) != 0)
            {
                char str[1024];
                wcstombs(str, buf, 1023);
                res = str;
            }
        }
        else
        {
            GUID folder;
            switch (f)
            {
            case user_desktop: folder = FOLDERID_Desktop;
                break;
            case user_documents: folder = FOLDERID_Documents;
                break;
            case user_pictures: folder = FOLDERID_Pictures;
                break;
            case user_videos: folder = FOLDERID_Videos;
                break;
            default:
                throw std::invalid_argument(
                    std::string("Value of f (") + std::to_string(f) + std::string(") is not supported"));
            }
            PWSTR folder_path = NULL;
            HRESULT hr = SHGetKnownFolderPath(folder, KF_FLAG_DEFAULT_PATH, NULL, &folder_path);
            if (SUCCEEDED(hr))
            {
                char str[1024];
                wcstombs(str, folder_path, 1023);
                CoTaskMemFree(folder_path);
                res = str;
                res += "\\";
            }
            else
            {
                throw std::runtime_error("Failed to get requested special folder");
            }
        }
#endif //_WIN32
#if defined __linux__ || defined __APPLE__
        if (f == special_folder::temp_folder)
        {
            const char* tmp_dir = getenv("TMPDIR");
            res = tmp_dir ? tmp_dir : "/tmp/";
        }
        else
        {
            const char* home_dir = getenv("HOME");
            if (!home_dir)
            {
                struct passwd* pw = getpwuid(getuid());
                home_dir = (pw && pw->pw_dir) ? pw->pw_dir : "";
            }
            if (home_dir)
            {
                res = home_dir;
                switch (f)
                {
                case user_desktop: res += "/Desktop/";
                    break;
                case user_documents: res += "/Documents/";
                    break;
                case user_pictures: res += "/Pictures/";
                    break;
                case user_videos: res += "/Videos/";
                    break;
                default:
                    throw std::invalid_argument(
                        std::string("Value of f (") + std::to_string(f) + std::string(") is not supported"));
                }
            }
        }
#endif // defined __linux__ || defined __APPLE__
        return res;
    }
}
