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

#include <opengl3.h>

#include <librealsense2/rs_advanced_mode.hpp>
#include <librealsense2/rsutil.h>

#include "model-views.h"
#include "notifications.h"
#include "fw-update-helper.h"
#include "on-chip-calib.h"
#include "viewer.h"
#include "post-processing-filters-list.h"
#include "post-processing-block-model.h"
#include <imgui_internal.h>
#include <time.h>

#include "os.h"

#include "metadata-helper.h"

using namespace rs400;
using namespace nlohmann;


static rs2_sensor_mode resolution_from_width_height(int width, int height)
{
    if ((width == 640 && height == 480) || (height == 640 && width == 480))
        return RS2_SENSOR_MODE_VGA;
    else if ((width == 1024 && height == 768) || (height == 768 && width == 1024))
        return RS2_SENSOR_MODE_XGA;
    else
        return RS2_SENSOR_MODE_COUNT;
}

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

struct attribute
{
    std::string name;
    std::string value;
    std::string description;
};

namespace rs2
{
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

    std::vector<uint8_t> bytes_from_bin_file(const std::string& filename)
    {
        std::ifstream file(filename.c_str(), std::ios::binary);
        if (!file.good())
            throw std::runtime_error(to_string() << "Invalid binary file specified " << filename << " verify the source path and location permissions");

        // Determine the file length
        file.seekg(0, std::ios_base::end);
        std::size_t size = file.tellg();
        if (!size)
            throw std::runtime_error(to_string() << "Invalid binary file " << filename  <<  " provided  - zero-size ");
        file.seekg(0, std::ios_base::beg);

        // Create a vector to store the data
        std::vector<uint8_t> v(size);

        // Load the data
        file.read((char*)&v[0], size);

        return v;
    }

    // Flush binary stream to file, override previous if exists
    void bin_file_from_bytes(const std::string& filename, const std::vector<uint8_t> bytes)
    {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file.good())
            throw std::runtime_error(to_string() << "Invalid binary file specified " << filename << " verify the target path and location permissions");
        file.write((char*)bytes.data(), bytes.size());
    }

    void imgui_easy_theming(ImFont*& font_14, ImFont*& font_18)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        const int OVERSAMPLE = config_file::instance().get(configurations::performance::font_oversample);

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

    void save_processing_block_to_config_file(const char* name, 
        std::shared_ptr<rs2::processing_block> pb, bool enable)
    {
        for (auto opt : pb->get_supported_options())
        {
            auto val = pb->get_option(opt);
            std::string key = name;
            key += ".";
            key += pb->get_option_name(opt);
            config_file::instance().set(key.c_str(), val); 
        }

        std::string key = name;
        key += ".enabled";
        config_file::instance().set(key.c_str(), enable);
    }

    bool restore_processing_block(const char* name, 
        std::shared_ptr<rs2::processing_block> pb, bool enable = true)
    {
        for (auto opt : pb->get_supported_options())
        {
            std::string key = name;
            key += ".";
            key += pb->get_option_name(opt);
            if (config_file::instance().contains(key.c_str()))
            {
                float val = config_file::instance().get(key.c_str());
                try
                {
                    auto range = pb->get_option_range(opt);
                    if (val >= range.min && val <= range.max)
                        pb->set_option(opt, val);
                }
                catch (...)
                {
                }
            }
        }

        std::string key = name;
        key += ".enabled";
        if (config_file::instance().contains(key.c_str()))
        {
            return config_file::instance().get(key.c_str());
        }
        return enable;
    }

    void hyperlink(ux_window& window, const char* title, const char* link)
    {
        if (ImGui::Button(title))
        {
            open_url(link);
        }
        if (ImGui::IsItemHovered())
        {
            window.link_hovered();
        }
    }

    void open_issue(std::string body)
    {
        std::string link = "https://github.com/IntelRealSense/librealsense/issues/new?body=" + url_encode(body);
        open_url(link.c_str());
    }

    void open_issue(const device_models_list& devices)
    {
        std::stringstream ss;

        rs2_error* e = nullptr;

        ss << "| | |\n";
        ss << "|---|---|\n";
        ss << "|**librealsense**|" << api_version_to_string(rs2_get_api_version(&e)) << (is_debug() ? " DEBUG" : " RELEASE") << "|\n";
        ss << "|**OS**|" << get_os_name() << "|\n";

        for (auto& dm : devices)
        {
            for (auto& kvp : dm->infos)
            {
                if (kvp.first != "Recommended Firmware Version" &&
                    kvp.first != "Debug Op Code" &&
                    kvp.first != "Physical Port" &&
                    kvp.first != "Product Id")
                    ss << "|**" << kvp.first << "**|" << kvp.second << "|\n";
            }
        }

        ss << "\nPlease provide a description of the problem";

        open_issue(ss.str());
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

    void export_frame(const std::string& fname, std::unique_ptr<rs2::filter> exporter, 
        notifications_model& ns, frame data, bool notify)
    {
        auto manager = std::make_shared<export_manager>(fname, std::move(exporter), data);

        auto n = std::make_shared<export_notification_model>(manager);
        ns.add_notification(n);
        n->forced = true;

        manager->start(n);
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
            std::ofstream csv(filename);

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
                auto intrinsics = vsp.get_intrinsics();
                csv << std::endl << "Intrinsic:," << std::fixed << std::setprecision(6) << std::endl;
                csv << "Fx," << intrinsics.fx << std::endl;
                csv << "Fy," << intrinsics.fy << std::endl;
                csv << "PPx," << intrinsics.ppx << std::endl;
                csv << "PPy," << intrinsics.ppy << std::endl;
                csv << "Distorsion," << rs2_distortion_to_string(intrinsics.model) << std::endl;
            }

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

    option_model create_option_model(rs2_option opt,
        const std::string& opt_base_label,
        subdevice_model* model,
        std::shared_ptr<options> options,
        bool* options_invalidated,
        std::string& error_message)
    {
        option_model option;

        std::stringstream ss;

        ss << opt_base_label << "/" << options->get_option_name(opt);
        option.id = ss.str();
        option.opt = opt;
        option.endpoint = options;
        option.label = options->get_option_name(opt) + std::string("##") + ss.str();
        option.invalidate_flag = options_invalidated;
        option.dev = model;

        option.supported = options->supports(opt);
        if (option.supported)
        {
            try
            {
                option.range = options->get_option_range(opt);
                option.read_only = options->is_option_read_only(opt);
                option.value = options->get_option(opt);
            }
            catch (const error& e)
            {
                option.range = { 0, 1, 0, 0 };
                option.value = 0;
                error_message = error_to_string(e);
            }
        }
        return option;
    }

    bool option_model::draw(std::string& error_message, notifications_model& model, bool new_line, bool use_option_name)
    {
        auto res = false;
        if (endpoint->supports(opt))
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
                    std::string txt = to_string() << endpoint->get_option_name(opt) << ":";
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
                            ImVec2 vec{ 0, 20 };
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
                    std::string txt = to_string() << (use_option_name ? endpoint->get_option_name(opt) : desc) << ":";

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

            if (!read_only && opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE && dev->auto_exposure_enabled  && dev->s->is<roi_sensor>() && dev->streaming)
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
            if ((supported = endpoint->supports(opt)))
            {
                value = endpoint->get_option(opt);
                range = endpoint->get_option_range(opt);
                read_only = endpoint->is_option_read_only(opt);
            }
        }
        catch (const error& e)
        {
            if (read_only) {
                model.add_notification({ to_string() << "Could not refresh read-only option " << endpoint->get_option_name(opt) << ": " << e.what(),
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
        for (auto&& i: options->get_supported_options())
        {
            auto opt = static_cast<rs2_option>(i);

            opt_container[opt] = create_option_model(opt, opt_base_label, model, options, options_invalidated, error_message);
        }
    }

    std::string get_device_sensor_name(subdevice_model* sub)
    {
        std::stringstream ss;
        ss << configurations::viewer::post_processing
            << "." << sub->dev.get_info(RS2_CAMERA_INFO_NAME)
            << "." << sub->s->get_info(RS2_CAMERA_INFO_NAME);
        return ss.str();
    }

    processing_block_model::processing_block_model(
        subdevice_model* owner,
        const std::string& name,
        std::shared_ptr<rs2::filter> block,
        std::function<rs2::frame(rs2::frame)> invoker,
        std::string& error_message, bool enable)
        : _owner(owner), _name(name), _block(block), _invoker(invoker), _enabled(enable)
    {
        std::stringstream ss;
        ss << "##" << ((owner) ? owner->dev.get_info(RS2_CAMERA_INFO_NAME) : _name)
            << "/" << ((owner) ? (*owner->s).get_info(RS2_CAMERA_INFO_NAME) : "_")
            << "/" << (long long)this;

        if (_owner)
            _full_name = get_device_sensor_name(_owner) + "." + _name;
        else
            _full_name = _name;

        _enabled = restore_processing_block(_full_name.c_str(),
                                            block, _enabled);

        populate_options(ss.str().c_str(), owner, owner ? &owner->_options_invalidated : nullptr, error_message);
    }

    void processing_block_model::save_to_config_file()
    {
        save_processing_block_to_config_file(_full_name.c_str(), _block, _enabled);
    }

    option_model& processing_block_model::get_option(rs2_option opt)
    {
        if (options_metadata.find(opt) != options_metadata.end()) 
            return options_metadata[opt];

        std::string error_message;
        options_metadata[opt] = create_option_model(opt, get_name(), _owner, _block, _owner ? &_owner->_options_invalidated : nullptr, error_message);
        return options_metadata[opt];
    }

    void processing_block_model::populate_options(const std::string& opt_base_label,
        subdevice_model* model,
        bool* options_invalidated,
        std::string& error_message)
    {
        for (auto opt : _block->get_supported_options())
        {
            options_metadata[opt] = create_option_model(opt, opt_base_label, model, _block, model ? &model->_options_invalidated : nullptr, error_message);
        }
    }

    subdevice_model::subdevice_model(
        device& dev,
        std::shared_ptr<sensor> s,
        std::shared_ptr< atomic_objects_in_frame > device_detected_objects,
        std::string& error_message,
        viewer_model& viewer
    )
        : s( s ), dev( dev ), tm2(), ui(), last_valid_ui(),
        streaming(false), _pause(false),
        depth_colorizer(std::make_shared<rs2::gl::colorizer>()),
        yuy2rgb(std::make_shared<rs2::gl::yuy_decoder>()),
        depth_decoder(std::make_shared<rs2::depth_huffman_decoder>()),
        viewer(viewer),
        detected_objects( device_detected_objects )
    {
        supported_options = s->get_supported_options();
        restore_processing_block("colorizer", depth_colorizer);
        restore_processing_block("yuy2rgb", yuy2rgb);

        std::string device_name( dev.get_info( RS2_CAMERA_INFO_NAME ));
        std::string sensor_name( s->get_info( RS2_CAMERA_INFO_NAME ));

        std::stringstream ss;
        ss << configurations::viewer::post_processing 
           << "." << device_name
           << "." << sensor_name;
        auto key = ss.str();

        bool const is_rgb_camera = s->is< color_sensor >();

        if (config_file::instance().contains(key.c_str()))
        {
            post_processing_enabled = config_file::instance().get(key.c_str());
        }

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

        auto filters = s->get_recommended_filters();
        
        auto it  = std::find_if(filters.begin(), filters.end(), [&](const filter &f)
        {
            if (f.is<zero_order_invalidation>())
                return true;
            return false;
        });

        auto is_zo = it != filters.end();

        for (auto&& f : s->get_recommended_filters())
        {
            auto shared_filter = std::make_shared<filter>(f);
            auto model = std::make_shared<processing_block_model>(
                this, shared_filter->get_info(RS2_CAMERA_INFO_NAME), shared_filter,
                [=](rs2::frame f) { return shared_filter->process(f); }, error_message);

            if (shared_filter->is<depth_huffman_decoder>())
                model->visible = false;

            if (is_zo)
            {
                if (shared_filter->is<zero_order_invalidation>())
                {
                    zero_order_artifact_fix = model;
                    viewer.zo_sensors++;
                }
                else
                    model->enable( false );
            }

            if (shared_filter->is<hole_filling_filter>())
                model->enable( false );

            if (shared_filter->is<decimation_filter>())
            {
                if( is_rgb_camera )
                    model->enable( false );
            }

            post_processing.push_back(model);
        }

        if( is_rgb_camera )
        {
            for( auto & create_filter : post_processing_filters_list::get() )
            {
                auto filter = create_filter();
                if( !filter )
                    continue;
                filter->start( *this );
                std::shared_ptr< processing_block_model > model(
                    new post_processing_block_model {
                        this, filter,
                        [=]( rs2::frame f ) { return filter->process( f ); },
                        error_message
                    } );
                post_processing.push_back( model );
            }
        }

        auto colorizer = std::make_shared<processing_block_model>(
            this, "Depth Visualization", depth_colorizer,
            [=](rs2::frame f) { return depth_colorizer->colorize(f); }, error_message);
        const_effects.push_back(colorizer);

        ss.str("");
        ss << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
            << "/" << s->get_info(RS2_CAMERA_INFO_NAME)
            << "/" << (long long)this;

        if (s->supports(RS2_CAMERA_INFO_PHYSICAL_PORT) && dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE))
        {
            std::string product = dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE);
            std::string id = s->get_info(RS2_CAMERA_INFO_PHYSICAL_PORT);

            bool has_metadata = !rs2::metadata_helper::instance().can_support_metadata(product)
                || rs2::metadata_helper::instance().is_enabled(id);
            static bool showed_metadata_prompt = false;

            if (!has_metadata && !showed_metadata_prompt)
            {
                auto n = std::make_shared<metadata_warning_model>();
                viewer.not_model.add_notification(n);
                showed_metadata_prompt = true;
            }
        }

        try
        {
            auto sensor_profiles = s->get_stream_profiles();
            reverse(begin(sensor_profiles), end(sensor_profiles));
            rs2_format def_format{ RS2_FORMAT_ANY };
            auto default_resolution = std::make_pair(1280, 720);
            for (auto&& profile : sensor_profiles)
            {
                std::stringstream res;
                if (auto vid_prof = profile.as<video_stream_profile>())
                {
                    if (profile.is_default())
                        default_resolution = std::pair<int, int>(vid_prof.width(), vid_prof.height());
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
            auto resolution_constrain = usb2 ? std::make_pair(640, 480) : default_resolution;

            // TODO: Once GLSL parts are properly optimised
            // and tested on all types of hardware
            // make sure we use Full-HD YUY overriding the default
            // This will lower CPU utilisation and generally be faster
            // if (!usb2 && std::string(s->get_info(RS2_CAMERA_INFO_NAME)) == "RGB Camera")
            // {
            //     if (config_file::instance().get(configurations::performance::glsl_for_rendering))
            //     {
            //         resolution_constrain = std::make_pair(1920, 1080);
            //         def_format = RS2_FORMAT_YUYV;
            //     }
            // }

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

            if (s->supports(RS2_OPTION_SENSOR_MODE))
                s->set_option(RS2_OPTION_SENSOR_MODE, resolution_from_width_height(res_values[ui.selected_res_id].first, res_values[ui.selected_res_id].second));

            while (ui.selected_res_id >= 0 && !is_selected_combination_supported()) ui.selected_res_id--;
            last_valid_ui = ui;
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
        populate_options(options_metadata, ss.str().c_str(), this, s, &_options_invalidated, error_message);

    }

    subdevice_model::~subdevice_model()
    {
        if(zero_order_artifact_fix)
            viewer.zo_sensors--;
    }

    bool subdevice_model::is_there_common_fps()
    {
        std::vector<int> first_fps_group;
        size_t group_index = 0;
        for (; group_index < fps_values_per_stream.size(); ++group_index)
        {
            if (!fps_values_per_stream[(rs2_stream)group_index].empty())
            {
                first_fps_group = fps_values_per_stream[(rs2_stream)group_index];
                break;
            }
        }

        for (size_t i = group_index + 1; i < fps_values_per_stream.size(); ++i)
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
                    _options_invalidated = true;

                    if (s->supports(RS2_OPTION_SENSOR_MODE))
                    {
                        auto width = res_values[ui.selected_res_id].first;
                        auto height = res_values[ui.selected_res_id].second;
                        auto res = resolution_from_width_height(width, height);
                        if (res >= RS2_SENSOR_MODE_XGA && res < RS2_SENSOR_MODE_COUNT)
                            s->set_option(RS2_OPTION_SENSOR_MODE, res);
                    }
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
                        auto tmp = stream_enabled;
                        label = to_string() << stream_display_names[f.first] << "##" << f.first;
                        if (ImGui::Checkbox(label.c_str(), &stream_enabled[f.first]))
                        {
                            prev_stream_enabled = tmp;
                        }
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

        if (results.size() == 0)
            return false;
        // Verify that the number of found matches corrseponds to the number of the requested streams
        // TODO - review whether the comparison can be made strict (==)
        return results.size() >= size_t(std::count_if(stream_enabled.begin(), stream_enabled.end(), [](const std::pair<int, bool>& kpv)-> bool { return kpv.second == true; }));
    }

    void subdevice_model::update_ui(std::vector<stream_profile> profiles_vec)
    {
        if (profiles_vec.empty())
            return;
        for (auto& s : stream_enabled)
            s.second = false;
        for (auto& p : profiles_vec)
        {
            stream_enabled[p.unique_id()] = true;

            auto format_vec = format_values[p.unique_id()];
            for (int i = 0; i < format_vec.size(); i++)
            {
                if (format_vec[i] == p.format())
                {
                    ui.selected_format_id[p.unique_id()] = i;
                    break;
                }
            }
            for (int i = 0; i < res_values.size(); i++)
            {
                if (auto vid_prof = p.as<video_stream_profile>())
                    if (res_values[i].first == vid_prof.width() && res_values[i].second == vid_prof.height())
                    {
                        ui.selected_res_id = i;
                        break;
                    }
            }
            for (int i = 0; i < shared_fps_values.size(); i++)
            {
                if (shared_fps_values[i] == p.fps())
                {
                    ui.selected_shared_fps_id = i;
                    break;
                }
            }
        }
        last_valid_ui = ui;
        prev_stream_enabled = stream_enabled; // prev differs from curr only after user changes
    }

    template<typename T, typename V>
    bool subdevice_model::check_profile (stream_profile p, T cond, std::map<V, std::map<int, stream_profile>>& profiles_map,
        std::vector<stream_profile>& results, V key, int num_streams, stream_profile& def_p)
    {
        bool found = false;
        if (auto vid_prof = p.as<video_stream_profile>())
        {
            for (auto& s : stream_enabled)
            {
                // find profiles that have an enabled stream and match the required condition
                if (s.second == true && vid_prof.unique_id() == s.first && cond(vid_prof))
                {
                    profiles_map[key].insert(std::pair<int, stream_profile>(p.unique_id(), p));
                    if (profiles_map[key].size() == num_streams)
                    {
                        results.clear(); // make sure only current profiles are saved
                        for (auto& it : profiles_map[key])
                            results.push_back(it.second);
                        found = true;
                    }
                    else if (results.empty() && num_streams > 1 && profiles_map[key].size() == num_streams-1)
                    {
                        for (auto& it : profiles_map[key])
                            results.push_back(it.second);
                    }
                }
                else if (!def_p.get() && cond(vid_prof))
                    def_p = p; // in case no matching profile for current stream will be found, we'll use some profile that matches the condition
            }
        }
        return found;
    }

    
    void subdevice_model::get_sorted_profiles(std::vector<stream_profile>& profiles)
    {
        auto fps = shared_fps_values[ui.selected_shared_fps_id];
        auto width = res_values[ui.selected_res_id].first;
        auto height = res_values[ui.selected_res_id].second;
        std::sort(profiles.begin(), profiles.end(), [&](stream_profile a, stream_profile b) {
            int score_a = 0, score_b = 0;
            if (a.fps() != fps)
                score_a++;
            if (b.fps() != fps)
                score_b++;

            if (a.format() != format_values[a.unique_id()][ui.selected_format_id[a.unique_id()]])
                score_a++;
            if (b.format() != format_values[b.unique_id()][ui.selected_format_id[b.unique_id()]])
                score_b++;

            auto a_vp = a.as<video_stream_profile>();
            auto b_vp = a.as<video_stream_profile>();
            if (!a_vp || !b_vp)
                return score_a < score_b;
            if (a_vp.width() != width || a_vp.height() != height)
                score_a++;
            if (b_vp.width() != width || b_vp.height() != height)
                score_b++;
            return score_a < score_b;
        });
    }

    std::vector<stream_profile> subdevice_model::get_supported_profiles()
    {
        std::vector<stream_profile> results;
        if (!show_single_fps_list || res_values.size() == 0)
            return results;

        int num_streams = 0;
        for (auto& s : stream_enabled)
            if (s.second == true)
                num_streams++;
        stream_profile def_p;
        auto fps = shared_fps_values[ui.selected_shared_fps_id];
        auto width = res_values[ui.selected_res_id].first;
        auto height = res_values[ui.selected_res_id].second;
        std::vector<stream_profile> sorted_profiles = profiles;

        if (ui.selected_res_id != last_valid_ui.selected_res_id)
        {
            get_sorted_profiles(sorted_profiles);
            std::map<int, std::map<int, stream_profile>> profiles_by_fps;
            for (auto&& p : sorted_profiles)
            {
                if (check_profile(p, [&](video_stream_profile vsp)
                { return (vsp.width() == width && vsp.height() == height); },
                    profiles_by_fps, results, p.fps(), num_streams, def_p))
                    break;
            }
        }
        else if (ui.selected_shared_fps_id != last_valid_ui.selected_shared_fps_id)
        {
            get_sorted_profiles(sorted_profiles);
            std::map<std::tuple<int,int>, std::map<int, stream_profile>> profiles_by_res;

            for (auto&& p : sorted_profiles)
            {
                if (auto vid_prof = p.as<video_stream_profile>())
                {
                    if (check_profile(p, [&](video_stream_profile vsp) { return (vsp.fps() == fps); },
                        profiles_by_res, results, std::make_tuple(vid_prof.width(), vid_prof.height()), num_streams, def_p))
                        break;
                }
            }
        }
        else if (ui.selected_format_id != last_valid_ui.selected_format_id)
        {
            if (num_streams == 0)
            {
                last_valid_ui = ui;
                return results;
            }
            get_sorted_profiles(sorted_profiles);
            std::vector<stream_profile> matching_profiles;
            std::map<std::tuple<int,int,int>, std::map<int, stream_profile>> profiles_by_fps_res; //fps, width, height
            rs2_format format;
            int stream_id;
            // find the stream to which the user made changes
            for (auto& it : ui.selected_format_id)
            {
                if (stream_enabled[it.first])
                {
                    auto last_valid_it = last_valid_ui.selected_format_id.find(it.first);
                    if ((last_valid_it == last_valid_ui.selected_format_id.end() || it.second != last_valid_it->second))
                    {
                        format = format_values[it.first][it.second];
                        stream_id = it.first;
                    }
                }
            }
            for (auto&& p : sorted_profiles)
            {
                if (auto vid_prof = p.as<video_stream_profile>())
                    if (p.unique_id() == stream_id && p.format() == format) // && stream_enabled[stream_id]
                    {
                        profiles_by_fps_res[std::make_tuple(p.fps(), vid_prof.width(), vid_prof.height())].insert(std::pair<int, stream_profile>(p.unique_id(), p));
                        matching_profiles.push_back(p);
                        if (!def_p.get())
                            def_p = p;
                    }
            }
            // take profiles not in matching_profiles with enabled stream and fps+resolution matching some profile in matching_profiles
            for (auto&& p : sorted_profiles)
            {
                if (auto vid_prof = p.as<video_stream_profile>())
                {
                    auto key = std::make_tuple(p.fps(), vid_prof.width(), vid_prof.height());
                    if (check_profile(p, [&](stream_profile prof) { return (std::find_if(matching_profiles.begin(), matching_profiles.end(), [&](stream_profile sp)
                                { return (stream_id != p.unique_id() && sp.fps() == p.fps() && sp.as<video_stream_profile>().width() == vid_prof.width() &&
                                sp.as<video_stream_profile>().height() == vid_prof.height()); })  != matching_profiles.end()); },
                        profiles_by_fps_res, results, std::make_tuple(p.fps(), vid_prof.width(), vid_prof.height()), num_streams, def_p))
                        break;
                }
            }
        }
        else if (stream_enabled != prev_stream_enabled)
        {
            if (num_streams == 0)
                return results;
            get_sorted_profiles(sorted_profiles);
            std::vector<stream_profile> matching_profiles;
            std::map<rs2_format, std::map<int, stream_profile>> profiles_by_format;

            for (auto&& p : sorted_profiles)
            {
                // first try to find profile from the new stream to meatch the current configuration
                if (check_profile(p, [&](video_stream_profile vid_prof)
                { return (p.fps() == fps && vid_prof.width() == width && vid_prof.height() == height); },
                    profiles_by_format, results, p.format(), num_streams, def_p))
                    break;
            }
            if (results.size() < num_streams)
            {
                results.clear();
                std::map<std::tuple<int, int, int>, std::map<int, stream_profile>> profiles_by_fps_res;
                for (auto&& p : sorted_profiles)
                {
                    if (auto vid_prof = p.as<video_stream_profile>())
                    {
                        // if no stream with current configuration was found, try to find some configuration to match all enabled streams
                        if (check_profile(p, [&](video_stream_profile vsp) { return true; }, profiles_by_fps_res, results,
                            std::make_tuple(p.fps(), vid_prof.width(), vid_prof.height()), num_streams, def_p))
                            break;
                    }
                }
            }
        }
        if (results.empty())
            results.push_back(def_p);
        update_ui(results);
        return results;
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

        for_each(stream_display_names.begin(), stream_display_names.end(), [this, &viewer](std::pair<int, std::string> kvp)
        {
            if ("Pose" == kvp.second)
            {
                 this->tm2.reset_trajectory();
                 this->tm2.record_trajectory(false);
            }
        });

        streaming = false;
        _pause = false;
        
        if( profiles[0].stream_type() == RS2_STREAM_COLOR )
        {
            std::lock_guard< std::mutex > lock( detected_objects->mutex );
            detected_objects->clear();
            detected_objects->sensor_is_on = false;
        }

        s->stop();

        _options_invalidated = true;

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

    bool subdevice_model::can_enable_zero_order()
    {
        auto ir = std::find_if(profiles.begin(), profiles.end(), [&](stream_profile p) { return (p.stream_type() == RS2_STREAM_INFRARED); });
        auto depth = std::find_if(profiles.begin(), profiles.end(), [&](stream_profile p) { return (p.stream_type() == RS2_STREAM_DEPTH); });

        return !(ir == profiles.end() || depth == profiles.end() || !stream_enabled[depth->unique_id()] || !stream_enabled[ir->unique_id()]);
    }

    void subdevice_model::verify_zero_order_conditions()
    {
         if(!can_enable_zero_order())
             throw std::runtime_error(to_string() << "Zero order filter requires both IR and Depth streams turned on.\nPlease rectify the configuration and rerun");
    }

    //The function decides if specific frame should be sent to the syncer
    bool subdevice_model::is_synchronized_frame(viewer_model& viewer, const frame& f)
    {
        if(zero_order_artifact_fix && zero_order_artifact_fix->is_enabled() && 
            (f.get_profile().stream_type() == RS2_STREAM_DEPTH || f.get_profile().stream_type() == RS2_STREAM_INFRARED || f.get_profile().stream_type() == RS2_STREAM_CONFIDENCE))
            return true;
        if (!viewer.is_3d_view || viewer.is_3d_depth_source(f) || viewer.is_3d_texture_source(f))
            return true;

        return false;
    }

    void subdevice_model::play(const std::vector<stream_profile>& profiles, viewer_model& viewer, std::shared_ptr<rs2::asynchronous_syncer> syncer)
    {
        if (post_processing_enabled && zero_order_artifact_fix && zero_order_artifact_fix->is_enabled())
        {
            verify_zero_order_conditions();
        }

        std::stringstream ss;
        ss << "Starting streaming of ";
        for (size_t i = 0; i < profiles.size(); i++)
        {
            ss << profiles[i].stream_type();
            if (i < profiles.size() - 1) ss << ", ";
        }
        ss << "...";
        viewer.not_model.add_log(ss.str());

        // TODO  - refactor tm2 from viewer to subdevice
        for_each(stream_display_names.begin(), stream_display_names.end(), [this, &viewer](std::pair<int, std::string> kvp)
        {
            if ("Pose" == kvp.second)
            {
                this->tm2.record_trajectory(true);
            }
        });

        s->open(profiles);

        try {
            s->start([&, syncer](frame f)
            {
                if (viewer.synchronization_enable && is_synchronized_frame(viewer, f))
                {
                    syncer->invoke(f);
                }
                else
                {
                    auto id = f.get_profile().unique_id();
                    viewer.ppf.frames_queue[id].enqueue(f);

                    on_frame();
                }
            });
            }

        catch (...)
        {
            s->close();
            throw;
        }

        _options_invalidated = true;
        streaming = true;
        if( s->is< color_sensor >() )
        {
            std::lock_guard< std::mutex > lock( detected_objects->mutex );
            detected_objects->sensor_is_on = true;
        }
    }
    void subdevice_model::update(std::string& error_message, notifications_model& notifications)
    {
        if (_options_invalidated)
        {
            next_option = 0;
            _options_invalidated = false;

            save_processing_block_to_config_file("colorizer", depth_colorizer);
            save_processing_block_to_config_file("yuy2rgb", yuy2rgb);

            for (auto&& pbm : post_processing) pbm->save_to_config_file();
        }

        if (next_option < supported_options.size())
        {
            auto next = supported_options[next_option];
            if (options_metadata.find(static_cast<rs2_option>(next)) != options_metadata.end())
            {
                auto& opt_md = options_metadata[static_cast<rs2_option>(next)];
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
            }

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
            if (skip_option(opt)) continue;
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
            [](const std::pair<int, option_model>& p) {return p.second.supported && !skip_option(p.second.opt); });
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
    {
        show_map_ruler = config_file::instance().get_or_default(
            configurations::viewer::show_map_ruler, true);
        show_stream_details = config_file::instance().get_or_default(
            configurations::viewer::show_stream_details, false);
    }

    std::shared_ptr<texture_buffer> stream_model::upload_frame(frame&& f)
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

        view_fps.add_timestamp(glfwGetTime() * 1000, count++);

        // populate frame metadata attributes
        for (auto i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
        {
            if (f.supports_frame_metadata((rs2_frame_metadata_value)i))
                frame_md.md_attributes[i] = std::make_pair(true, f.get_frame_metadata((rs2_frame_metadata_value)i));
            else
                frame_md.md_attributes[i].first = false;
        }

        texture->upload(f);
        return texture;
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
        texture->yuy2rgb = d->yuy2rgb;
        texture->depth_decode = d->depth_decoder;

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

    bool draw_combo_box(const std::string& id, const std::vector<std::string>& device_names, int& new_index)
    {
        std::vector<const char*>  device_names_chars = get_string_pointers(device_names);
        return ImGui::Combo(id.c_str(), &new_index, device_names_chars.data(), static_cast<int>(device_names.size()));
    }

    void stream_model::show_stream_header(ImFont* font, const rect &stream_rect, viewer_model& viewer)
    {
        const auto top_bar_height = 32.f;
        auto num_of_buttons = 5;

        if (!viewer.allow_stream_close) --num_of_buttons;
        if (viewer.streams.size() > 1) ++num_of_buttons;
        if (RS2_STREAM_DEPTH == profile.stream_type()) ++num_of_buttons; // Color map ruler button

        ImGui_ScopePushFont(font);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);

        std::string label = to_string() << "Stream of " << profile.unique_id();

        ImGui::GetWindowDrawList()->AddRectFilled({ stream_rect.x, stream_rect.y - top_bar_height },
                    { stream_rect.x + stream_rect.w, stream_rect.y }, ImColor(sensor_bg));

        int offset = 5;
        if (dev->_is_being_recorded) offset += 23;
        auto p = dev->dev.as<playback>();
        if (dev->is_paused() || (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED)) offset += 23;

        ImGui::SetCursorScreenPos({ stream_rect.x + 4 + offset, stream_rect.y - top_bar_height + 7 });

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

        ImGui::PushTextWrapPos(stream_rect.x + stream_rect.w - 32 * num_of_buttons - 5);
        ImGui::Text("%s", label.c_str());
        if (tooltip != label && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip.c_str());
        ImGui::PopTextWrapPos();

        ImGui::SetCursorScreenPos({ stream_rect.x + stream_rect.w - 32 * num_of_buttons, stream_rect.y - top_bar_height });


        label = to_string() << textual_icons::metadata << "##Metadata" << profile.unique_id();
        if (show_metadata)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                show_metadata = false;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Hide frame metadata");
            }
            ImGui::PopStyleColor(2);
        }
        else
        {
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                show_metadata = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Show frame metadata");
            }
        }
        ImGui::SameLine();

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
                    config_file::instance().set(configurations::viewer::show_map_ruler, show_map_ruler);
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
                    config_file::instance().set(configurations::viewer::show_map_ruler, show_map_ruler);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Show color map ruler");
                }
            }
            ImGui::SameLine();
        }

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
                viewer.paused = false;
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
                viewer.paused = true;
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
                config_file::instance().set(
                    configurations::viewer::show_stream_details, 
                    show_stream_details);
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
                config_file::instance().set(
                    configurations::viewer::show_stream_details, 
                    show_stream_details);
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

        ImGui::PopStyleColor(5);

        _info_height = (show_stream_details || show_metadata) ? (show_metadata ? stream_rect.h : 32.f) : 0.f;

        static const auto y_offset_info_rect = 0.f;
        static const auto x_offset_info_rect = 0.f;
        auto width_info_rect = stream_rect.w - 2.f * x_offset_info_rect;

        curr_info_rect = rect{ stream_rect.x + x_offset_info_rect,
            stream_rect.y + y_offset_info_rect,
            width_info_rect,
            _info_height };

        ImGui::GetWindowDrawList()->AddRectFilled({ curr_info_rect.x, curr_info_rect.y },
        { curr_info_rect.x + curr_info_rect.w, curr_info_rect.y + curr_info_rect.h },
            ImColor(dark_sensor_bg));

        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);

        float line_y = curr_info_rect.y + 8;

        if (show_stream_details && !show_metadata)
        {
            if (_info_height.get() > line_y + ImGui::GetTextLineHeight() - curr_info_rect.y)
            {
                ImGui::SetCursorScreenPos({ curr_info_rect.x + 10, line_y });

                if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
                    ImGui::PushStyleColor(ImGuiCol_Text, redish);

                label = to_string() << "Time: " << std::left << std::fixed << std::setprecision(1) << timestamp << " ";

                if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME) label = to_string() << textual_icons::exclamation_triangle << label;

                ImGui::Text("%s", label.c_str());

                if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME) ImGui::PopStyleColor();

                if (ImGui::IsItemHovered())
                {
                    if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(450.0f);
                        ImGui::TextUnformatted("Timestamp Domain: System Time. Hardware Timestamps unavailable!\nPlease refer to frame_metadata.md for more information");
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                    else if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME)
                    {
                        ImGui::SetTooltip("Timestamp: Global Time");
                    }
                    else
                    {
                        ImGui::SetTooltip("Timestamp: Hardware Clock");
                    }
                }

                ImGui::SameLine();

                label = to_string() << " Frame: " << std::left << frame_number;
                ImGui::Text("%s", label.c_str());

                ImGui::SameLine();

                std::string res;
                if (profile.as<rs2::video_stream_profile>())
                    res = to_string() << size.x << "x" << size.y << ",  ";
                label = to_string() << res << truncate_string(rs2_format_to_string(profile.format()), 9) << ", ";
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Stream Resolution, Format");
                }

                ImGui::SameLine();

                label = to_string() << "FPS: " << std::setprecision(2) << std::setw(7) << std::fixed << fps.get_fps();
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "FPS is calculated based on timestamps and not viewer time");
                }

                line_y += ImGui::GetTextLineHeight() + 5;
            }
        }

        

        if (show_metadata)
        {
            std::vector<attribute> stream_details;

            if (true) // Always show stream details options
            {
                stream_details.push_back({ "Frame Timestamp",
                    to_string() << std::fixed << std::setprecision(1) << timestamp, 
                    "Frame Timestamp is normalized represetation of when the frame was taken.\n"
                    "It's a property of every frame, so when exact creation time is not provided by the hardware, an approximation will be used.\n"
                    "Clock Domain feilds helps to interpret the meaning of timestamp\n"
                    "Timestamp is measured in milliseconds, and is allowed to roll-over (reset to zero) in some situations" });
                stream_details.push_back({ "Clock Domain",
                    to_string() << rs2_timestamp_domain_to_string(timestamp_domain), 
                    "Clock Domain describes the format of Timestamp field. It can be one of the following:\n"
                    "1. System Time - When no hardware timestamp is available, system time of arrival will be used.\n"
                    "                 System time benefits from being comparable between device, but suffers from not being able to approximate latency.\n"
                    "2. Hardware Clock - Hardware timestamp is attached to the frame by the device, and is consistent accross device sensors.\n"
                    "                    Hardware timestamp encodes precisely when frame was captured, but cannot be compared across devices\n"
                    "3. Global Time - Global time is provided when the device can both offer hardware timestamp and implements Global Timestamp Protocol.\n"
                    "                 Global timestamps encode exact time of capture and at the same time are comparable accross devices." });
                stream_details.push_back({ "Frame Number",
                    to_string() << frame_number, "Frame Number is a rolling ID assigned to frames.\n"
                    "Most devices do not guarantee consequitive frames to have conseuquitive frame numbers\n"
                    "But it is true most of the time" });

                if (profile.as<rs2::video_stream_profile>())
                {
                    stream_details.push_back({ "Hardware Size",
                        to_string() << original_size.x << " x " << original_size.y, "" });

                    stream_details.push_back({ "Display Size",
                        to_string() << size.x << " x " << size.y, 
                        "When Post-Processing is enabled, the actual display size of the frame may differ from original capture size" });
                }
                stream_details.push_back({ "Pixel Format",
                    to_string() << rs2_format_to_string(profile.format()), "" });

                stream_details.push_back({ "Hardware FPS",
                    to_string() << std::setprecision(2) << std::fixed << fps.get_fps(), 
                    "Hardware FPS captures the number of frames per second produced by the device.\n"
                    "It is possible and likely that not all of these frames will make it to the application." });
                
                stream_details.push_back({ "Viewer FPS",
                    to_string() << std::setprecision(2) << std::fixed << view_fps.get_fps(),
                    "Viewer FPS captures how many frames the application manages to render.\n"
                    "Frame drops can occur for variety of reasons." });

                stream_details.push_back({ "", "", "" });
            }

            const std::string no_md = "no md";

            if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
            {
                stream_details.push_back({ no_md, "", "" });
            }

            std::map<rs2_frame_metadata_value, std::string> descriptions = {
                { RS2_FRAME_METADATA_FRAME_COUNTER                        , "A sequential index managed per-stream. Integer value" },
                { RS2_FRAME_METADATA_FRAME_TIMESTAMP                      , "Timestamp set by device clock when data readout and transmit commence. Units are device dependent" },
                { RS2_FRAME_METADATA_SENSOR_TIMESTAMP                     , "Timestamp of the middle of sensor's exposure calculated by device. usec" },
                { RS2_FRAME_METADATA_ACTUAL_EXPOSURE                      , "Sensor's exposure width. When Auto Exposure (AE) is on the value is controlled by firmware. usec" },
                { RS2_FRAME_METADATA_GAIN_LEVEL                           , "A relative value increasing which will increase the Sensor's gain factor.\n"
                                                                            "When AE is set On, the value is controlled by firmware. Integer value" },
                { RS2_FRAME_METADATA_AUTO_EXPOSURE                        , "Auto Exposure Mode indicator. Zero corresponds to AE switched off. " },
                { RS2_FRAME_METADATA_WHITE_BALANCE                        , "White Balance setting as a color temperature. Kelvin degrees" },
                { RS2_FRAME_METADATA_TIME_OF_ARRIVAL                      , "Time of arrival in system clock " },
                { RS2_FRAME_METADATA_TEMPERATURE                          , "Temperature of the device, measured at the time of the frame capture. Celsius degrees " },
                { RS2_FRAME_METADATA_BACKEND_TIMESTAMP                    , "Timestamp get from uvc driver. usec" },
                { RS2_FRAME_METADATA_ACTUAL_FPS                           , "Actual hardware FPS. May differ from requested due to Auto-Exposure" },
                { RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE               , "Laser power mode. Zero corresponds to Laser power switched off and one for switched on." },
                { RS2_FRAME_METADATA_EXPOSURE_PRIORITY                    , "Exposure priority. When enabled Auto-exposure algorithm is allowed to reduce requested FPS to sufficiently increase exposure time (an get enough light)" },
                { RS2_FRAME_METADATA_POWER_LINE_FREQUENCY                 , "Power Line Frequency for anti-flickering Off/50Hz/60Hz/Auto. " },
            };

            for (auto i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
            {
                auto&& kvp = frame_md.md_attributes[i];
                if (kvp.first)
                {
                    auto val = (rs2_frame_metadata_value)i;
                    std::string name = to_string() << rs2_frame_metadata_to_string(val);
                    std::string desc = "";
                    if (descriptions.find(val) != descriptions.end()) desc = descriptions[val];
                    stream_details.push_back({ name, to_string() << kvp.second, desc });
                }
            }

            float max_text_width = 0.;
            for (auto&& kvp : stream_details)
                max_text_width = std::max(max_text_width, ImGui::CalcTextSize(kvp.name.c_str()).x);

            for (auto&& at : stream_details)
            {
                if (_info_height.get() > line_y + ImGui::GetTextLineHeight() - curr_info_rect.y)
                {
                    ImGui::SetCursorScreenPos({ curr_info_rect.x + 10, line_y });

                    if (at.name == no_md)
                    {
                        auto text = "Per-frame metadata is not enabled at the OS level!\nPlease follow the installation guide for the details";
                        auto size = ImGui::CalcTextSize(text);

                        for (int i = 3; i > 0; i-=1)
                            ImGui::GetWindowDrawList()->AddRectFilled({ curr_info_rect.x + 10 - i, line_y - i },
                            { curr_info_rect.x + 10 + i + size.x, line_y + size.y + i },
                            ImColor(alpha(sensor_bg, 0.1f)));

                        ImGui::PushStyleColor(ImGuiCol_Text, redish);
                        ImGui::Text("%s", text);
                        ImGui::PopStyleColor();

                        line_y += ImGui::GetTextLineHeight() + 3;
                    }
                    else
                    {
                        std::string text = "";
                        if (at.name != "") text = to_string() << at.name << ":";
                        auto size = ImGui::CalcTextSize(text.c_str());

                        for (int i = 3; i > 0; i-=1)
                            ImGui::GetWindowDrawList()->AddRectFilled({ curr_info_rect.x + 10 - i, line_y - i },
                            { curr_info_rect.x + 10 + i + size.x, line_y + size.y + i },
                            ImColor(alpha(sensor_bg, 0.1f)));

                        ImGui::PushStyleColor(ImGuiCol_Text, white);
                        ImGui::Text("%s", text.c_str()); ImGui::SameLine();

                        if (at.description != "")
                        {
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("%s", at.description.c_str());
                            }
                        }

                        text = at.value;
                        size = ImGui::CalcTextSize(text.c_str());

                        for (int i = 3; i > 0; i-=1)
                            ImGui::GetWindowDrawList()->AddRectFilled({ curr_info_rect.x + 20 + max_text_width - i, line_y - i },
                            { curr_info_rect.x + 30 + max_text_width + i + size.x, line_y + size.y + i },
                            ImColor(alpha(sensor_bg, 0.1f)));

                        ImGui::PopStyleColor();

                        ImGui::SetCursorScreenPos({ curr_info_rect.x + 20 + max_text_width, line_y });

                        std::string id = to_string() << "##" << at.name << "-" << profile.unique_id();

                        ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);

                        ImGui::InputText(id.c_str(),
                            (char*)text.c_str(),
                            text.size() + 1,
                            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

                        ImGui::PopStyleColor(2);
                    }

                    line_y += ImGui::GetTextLineHeight() + 3;
                }
            }
        }

        ImGui::PopStyleColor(5);
    }

    void stream_model::show_stream_footer(ImFont* font, const rect &stream_rect, const mouse_info& mouse, viewer_model& viewer)
    {
        auto non_visual_stream = (profile.stream_type() == RS2_STREAM_GYRO) 
            || (profile.stream_type() == RS2_STREAM_ACCEL) 
            || (profile.stream_type() == RS2_STREAM_GPIO) 
            || (profile.stream_type() == RS2_STREAM_POSE);

        if (stream_rect.contains(mouse.cursor) && !non_visual_stream && !show_metadata)
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
                ss << " 0x" << std::hex << static_cast<int>(round(val)) << " =";
            }

            if (texture->get_last_frame().is<depth_frame>())
            {
                auto meters = texture->get_last_frame().as<depth_frame>().get_distance(x, y);

                if (viewer.metric_system)
                    ss << std::dec << " " << std::setprecision(3) << meters << " meters";
                else
                    ss << std::dec << " " << std::setprecision(3) << meters / FEET_TO_METER << " feet";
            }

            std::string msg(ss.str().c_str());

            ImGui_ScopePushFont(font);

            // adjust windows size to the message length

            auto width = float(msg.size()*8);
            auto align = 20;
            width += align - (int)width % align;

            ImVec2 pos { stream_rect.x + 5, stream_rect.y + stream_rect.h - 35 };
            ImGui::GetWindowDrawList()->AddRectFilled({ pos.x, pos.y },
                    { pos.x + width, pos.y + 30 }, ImColor(dark_sensor_bg));

            ImGui::SetCursorScreenPos({ pos.x + 10, pos.y + 5 });

            std::string label = to_string() << "Footer for stream of " << profile.unique_id();

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

            ImGui::Text("%s", msg.c_str());
            ImGui::PopStyleColor(2);
        }
    }

    void stream_model::show_stream_imu(ImFont* font, const rect &stream_rect, const  rs2_vector& axis, const mouse_info& mouse)
    {
        if (stream_rect.contains(mouse.cursor))
        {
            const auto precision = 3;
            rs2_stream stream_type = profile.stream_type();

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

            ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);

            float y_offset = 0;
            if (show_stream_details)
            {
                y_offset += 30;
            }

            std::string label = to_string() << "IMU Stream Info of " << profile.unique_id();

            ImVec2 pos { stream_rect.x, stream_rect.y + y_offset };
            ImGui::SetCursorScreenPos({ pos.x + 5, pos.y + 5 });

            struct motion_data {
                std::string name;
                float coordinate;
                std::string units;
                std::string toolTip;
                ImVec4 colorFg;
                ImVec4 colorBg;
                int nameExtraSpace;
            };

            float norm = std::sqrt((axis.x*axis.x) + (axis.y*axis.y) + (axis.z*axis.z));

            std::map<rs2_stream, std::string> motion_unit = { { RS2_STREAM_GYRO, "Radians/Sec" },{ RS2_STREAM_ACCEL, "Meter/Sec^2" } };
            std::vector<motion_data> motion_vector = { { "X", axis.x, motion_unit[stream_type].c_str(), "Vector X", from_rgba(233, 0, 0, 255, true) , from_rgba(233, 0, 0, 255, true), 0},
                                                    { "Y", axis.y, motion_unit[stream_type].c_str(), "Vector Y", from_rgba(0, 255, 0, 255, true) , from_rgba(2, 100, 2, 255, true), 0},
                                                    { "Z", axis.z, motion_unit[stream_type].c_str(), "Vector Z", from_rgba(85, 89, 245, 255, true) , from_rgba(0, 0, 245, 255, true), 0},
                                                    { "N", norm, "Norm", "||V|| = SQRT(X^2 + Y^2 + Z^2)",from_rgba(255, 255, 255, 255, true) , from_rgba(255, 255, 255, 255, true), 0} };

            int line_h = 18;
            for (auto&& motion : motion_vector)
            {
                auto rc = ImGui::GetCursorPos();
                ImGui::SetCursorPos({ rc.x + 12, rc.y + 4 });
                ImGui::PushStyleColor(ImGuiCol_Text, motion.colorFg);
                ImGui::Text("%s:", motion.name.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s",motion.toolTip.c_str());
                }
                ImGui::PopStyleColor(1);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, black);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, motion.colorBg);

                ImGui::PushItemWidth(100);
                ImGui::SetCursorPos({ rc.x + 27 + motion.nameExtraSpace, rc.y + 1 });
                std::string label = to_string() << "##" << profile.unique_id() << " " << motion.name.c_str();
                std::string coordinate = to_string() << std::fixed << std::setprecision(precision) << std::showpos << motion.coordinate;
                ImGui::InputText(label.c_str(), (char*)coordinate.c_str(), coordinate.size() + 1, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();

                ImGui::SetCursorPos({ rc.x + 80 + motion.nameExtraSpace, rc.y + 4 });
                ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(255, 255, 255, 100, true));
                ImGui::Text("(%s)", motion.units.c_str());

                ImGui::PopStyleColor(3);
                ImGui::SetCursorPos({ rc.x, rc.y + line_h });
            }

            ImGui::PopStyleColor(5);
        }
    }

    void stream_model::show_stream_pose(ImFont* font, const rect &stream_rect, 
        const rs2_pose& pose_frame, rs2_stream stream_type, bool fullScreen, float y_offset, 
        viewer_model& viewer)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);

        std::string label = to_string() << "Pose Stream Info of " << profile.unique_id();

        ImVec2 pos { stream_rect.x, stream_rect.y + y_offset };
        ImGui::SetCursorScreenPos({ pos.x + 5, pos.y + 5 });
        
        std::string confidenceName[4] = { "Failed", "Low", "Medium", "High" };
        struct pose_data {
            std::string name;
            float floatData[4];
            std::string strData;
            uint8_t precision;
            bool signedNumber;
            std::string units;
            std::string toolTip;
            uint32_t nameExtraSpace;
            bool showOnNonFullScreen;
            bool fixedPlace;
            bool fixedColor;
        };

        rs2_vector velocity = pose_frame.velocity;
        rs2_vector acceleration = pose_frame.acceleration;
        rs2_vector translation = pose_frame.translation;
        const float feetTranslator = 3.2808f;
        std::string unit = viewer.metric_system ? "meters" : "feet";

        if (!viewer.metric_system)
        {
            velocity.x *= feetTranslator; velocity.y *= feetTranslator; velocity.z *= feetTranslator;
            acceleration.x *= feetTranslator; acceleration.y *= feetTranslator; acceleration.z *= feetTranslator;
            translation.x *= feetTranslator; translation.y *= feetTranslator; translation.z *= feetTranslator;
        }

        std::vector<pose_data> pose_vector = {
            { "Confidence",{ FLT_MAX , FLT_MAX , FLT_MAX , FLT_MAX }, confidenceName[pose_frame.tracker_confidence], 3, true, "", "Tracker confidence: High=Green, Medium=Yellow, Low=Red, Failed=Grey", 50, false, true, false },
            { "Velocity", {velocity.x, velocity.y , velocity.z , FLT_MAX }, "", 3, true, "(" + unit + "/Sec)", "Velocity: X, Y, Z values of velocity, in " + unit + "/Sec", 50, false, true, false},
            { "Angular Velocity",{ pose_frame.angular_velocity.x, pose_frame.angular_velocity.y , pose_frame.angular_velocity.z , FLT_MAX }, "", 3, true, "(Radians/Sec)", "Angular Velocity: X, Y, Z values of angular velocity, in Radians/Sec", 50, false, true, false },
            { "Acceleration",{ acceleration.x, acceleration.y , acceleration.z , FLT_MAX }, "", 3, true, "(" + unit + "/Sec^2)", "Acceleration: X, Y, Z values of acceleration, in " + unit + "/Sec^2", 50, false, true, false },
            { "Angular Acceleration",{ pose_frame.angular_acceleration.x, pose_frame.angular_acceleration.y , pose_frame.angular_acceleration.z , FLT_MAX }, "", 3, true, "(Radians/Sec^2)", "Angular Acceleration: X, Y, Z values of angular acceleration, in Radians/Sec^2", 50, false, true, false },
            { "Translation",{ translation.x, translation.y , translation.z , FLT_MAX }, "", 3, true, "(" + unit + ")", "Translation: X, Y, Z values of translation in " + unit + " (relative to initial position)", 50, true, true, false },
            { "Rotation",{ pose_frame.rotation.x, pose_frame.rotation.y , pose_frame.rotation.z , pose_frame.rotation.w }, "", 3, true,  "(Quaternion)", "Rotation: Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position)", 50, true, true, false },
        };

        int line_h = 18;
        if (fullScreen)
        {
            line_h += 2;
        }

        for (auto&& pose : pose_vector)
        {
            if ((fullScreen == false) && (pose.showOnNonFullScreen == false))
            {
                continue;
            }

            auto rc = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ rc.x + 12, rc.y + 4 });
            ImGui::Text("%s:", pose.name.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s",pose.toolTip.c_str());
            }

            if (pose.fixedColor == false)
            {
                switch (pose_frame.tracker_confidence) //color the line according to confidence
                {
                    case 3: // High confidence - Green
                        ImGui::PushStyleColor(ImGuiCol_Text, green);
                        break;
                    case 2: // Medium confidence - Yellow
                        ImGui::PushStyleColor(ImGuiCol_Text, yellow);
                        break;
                    case 1: // Low confidence - Red
                        ImGui::PushStyleColor(ImGuiCol_Text, red);
                        break;
                    case 0: // Failed confidence - Grey
                    default: // Fall thourgh
                        ImGui::PushStyleColor(ImGuiCol_Text, grey);
                        break;
                }
            }

            ImGui::SetCursorPos({ rc.x + 100 + (fullScreen?pose.nameExtraSpace:0), rc.y + 1 });
            std::string label = to_string() << "##" << profile.unique_id() << " " << pose.name.c_str();
            std::string data = "";

            if (pose.strData.empty())
            {
                data = "[";
                std::string comma = "";
                unsigned int i = 0;
                while ((i<4) && (pose.floatData[i] != FLT_MAX))
                {

                    data += to_string() << std::fixed << std::setprecision(pose.precision) << (pose.signedNumber ? std::showpos : std::noshowpos) << comma << pose.floatData[i];
                    comma = ", ";
                    i++;
                }
                data += "]";
            }
            else
            {
                data = pose.strData;
            }

            auto textSize = ImGui::CalcTextSize((char*)data.c_str(), (char*)data.c_str() + data.size() + 1);
            ImGui::PushItemWidth(textSize.x);
            ImGui::InputText(label.c_str(), (char*)data.c_str(), data.size() + 1, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();

            if (pose.fixedColor == false)
            {
                ImGui::PopStyleColor(1);
            }

            if (pose.fixedPlace == true)
            {
                ImGui::SetCursorPos({ rc.x + 300 + (fullScreen?pose.nameExtraSpace:0), rc.y + 4 });
            }
            else
            {
                ImGui::SameLine();
            }

            ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(255, 255, 255, 100, true));
            ImGui::Text("%s", pose.units.c_str());
            ImGui::PopStyleColor(1);

            ImGui::SetCursorPos({ rc.x, rc.y + line_h });
        }

        ImGui::PopStyleColor(5);
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
                    RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });

            // And the frame's attributes
            filename = filename_base + "_" + stream_desc + "_metadata.csv";

            try
            {
                if (frame_metadata_to_csv(filename, original_frame))
                    ss << "The frame attributes are saved into " << filename;
                else
                    viewer.not_model.add_notification({ to_string() << "Failed to save frame metadata file " << filename,
                        RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
            catch (std::exception& e)
            {
                viewer.not_model.add_notification({ to_string() << e.what(),
                    RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
        }

        if (ss.str().size())
            viewer.not_model.add_notification(notification_data{ 
                ss.str().c_str(), RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT });

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

    void device_model::reset()
    {
        syncer->remove_syncer(dev_syncer);
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

    device_model::~device_model()
    {
        for (auto&& n : related_notifications) n->dismiss(false);
    }

    void device_model::refresh_notifications(viewer_model& viewer)
    {
        for (auto&& n : related_notifications) n->dismiss(false);

        auto name = get_device_name(dev);

        if ((bool)config_file::instance().get(configurations::update::recommend_updates))
        {
            bool fw_update_required = false;
            for (auto&& sub : dev.query_sensors())
            {
                if (sub.supports(RS2_CAMERA_INFO_FIRMWARE_VERSION) &&
                    sub.supports(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION) &&
                    sub.supports(RS2_CAMERA_INFO_PRODUCT_LINE))
                {
                    std::string fw = sub.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION);
                    std::string recommended = sub.get_info(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION);

                    int product_line = parse_product_line(sub.get_info(RS2_CAMERA_INFO_PRODUCT_LINE));

                    bool allow_rc_firmware = config_file::instance().get_or_default(configurations::update::allow_rc_firmware, false);
                    bool is_rc = (product_line == RS2_PRODUCT_LINE_D400) && allow_rc_firmware;
                    std::string available = get_available_firmware_version(product_line);

                    std::shared_ptr<firmware_update_manager> manager = nullptr;

                    if (is_upgradeable(fw, available))
                    {
                        recommended = available;

                        static auto table = create_default_fw_table();

                        manager = std::make_shared<firmware_update_manager>(*this, dev, viewer.ctx, table[product_line], true);
                    }

                    if (is_upgradeable(fw, recommended))
                    {
                        std::stringstream msg;
                        msg << name.first << " (S/N " << name.second << ")\n"
                            << "Current Version: " << fw << "\n";

                        if (is_rc)
                            msg << "Release Candidate: " << recommended << " Pre-Release";
                        else
                            msg << "Recommended Version: " << recommended;

                        if (!fw_update_required)
                        {
                            auto n = std::make_shared<fw_update_notification_model>(
                                msg.str(), manager, false);
                            viewer.not_model.add_notification(n);

                            fw_update_required = true;

                            related_notifications.push_back(n);
                        }
                    }
                }
            }
        }

        if ((bool)config_file::instance().get(configurations::update::recommend_calibration))
        {
            for (auto&& model : subdevices)
            {
                if (model->supports_on_chip_calib())
                {
                    // Make sure we don't spam calibration remainders too often:
                    time_t rawtime;
                    time(&rawtime);
                    std::string id = to_string() << configurations::viewer::last_calib_notice << "." << name.second;
                    long long last_time = config_file::instance().get_or_default(id.c_str(), (long long)0);

                    std::string msg = to_string()
                        << name.first << " (S/N " << name.second << ")";
                    auto manager = std::make_shared<on_chip_calib_manager>(viewer, model, *this, dev);
                    auto n = std::make_shared<autocalib_notification_model>(
                        msg, manager, false);

                    // Recommend calibration once a week per device
                    if (rawtime - last_time < 60)
                    {
                        n->snoozed = true;
                    }

                    // NOTE: For now do not pre-emptively suggest auto-calibration
                    // TODO: Revert in later release
                    //viewer.not_model.add_notification(n);
                    //related_notifications.push_back(n);
                }
            }
        }
    }

    device_model::device_model(device& dev, std::string& error_message, viewer_model& viewer)
        : dev(dev),
          syncer(viewer.syncer),
           _update_readonly_options_timer(std::chrono::seconds(6))
        , _detected_objects( std::make_shared< atomic_objects_in_frame >() )
    {        
        auto name = get_device_name(dev);
        id = to_string() << name.first << ", " << name.second;

        for (auto&& sub : dev.query_sensors())
        {
            auto model = std::make_shared<subdevice_model>(dev, std::make_shared<sensor>(sub), _detected_objects, error_message, viewer);
            subdevices.push_back(model);
        }

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

        refresh_notifications(viewer);
    }
    void device_model::play_defaults(viewer_model& viewer)
    {
        if(!dev_syncer)
            dev_syncer = viewer.syncer->create_syncer();
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

                std::string friendly_name = sub->s->get_info(RS2_CAMERA_INFO_NAME);
                if ((friendly_name.find("Tracking") != std::string::npos) ||
                    (friendly_name.find("Motion") != std::string::npos))
                {
                    viewer.synchronization_enable = false;
                }
                sub->play(profiles, viewer, dev_syncer);

                for (auto&& profile : profiles)
                {
                    viewer.begin_stream(sub, profile);
                }
            }
        }
    }

    rs2::frame post_processing_filters::apply_filters(rs2::frame f, const rs2::frame_source& source)
    {
        std::vector<rs2::frame> frames;
        if (auto composite = f.as<rs2::frameset>())
        {
            for (auto&& f : composite)
                frames.push_back(f);
        }
        else
            frames.push_back(f);

        auto res = f;

        //In order to know what are the processing blocks we need to apply
        //We should find all the sub devices releted to the frames
        std::set<std::shared_ptr<subdevice_model>> subdevices;
        for (auto f : frames)
        {
            auto sub = get_frame_origin(f);
            if(sub)
                subdevices.insert(sub);
        }

        for (auto sub : subdevices)
        {
            if (!sub->post_processing_enabled)
                continue;

            for(auto&& pp : sub->post_processing)
                if (pp->is_enabled())
                    res = pp->invoke(res);
        }

        // Override the zero pixel in texture frame with black color for occlusion invalidation
        // TODO - this is a temporal solution to be refactored from the app level into the core library
        if (auto set = res.as<frameset>())
        {
            for (auto f : set)
            {
                zero_first_pixel(f);
            }
        }    
        else
        {
            zero_first_pixel(f);
        }     
        return res;
    }

    std::shared_ptr<subdevice_model> post_processing_filters::get_frame_origin(const rs2::frame& f)
    {
        for (auto&& s : viewer.streams)
        {
            if (s.second.dev)
            {
                auto dev = s.second.dev;

                if (s.second.original_profile.unique_id() == f.get_profile().unique_id())
                {
                    return dev;
                }
            }
        }
        return nullptr;
    }

    //Zero the first pixel on frame ,used to invalidate the occlusion pixels
    void post_processing_filters::zero_first_pixel(const rs2::frame& f)
    {
        auto stream_type = f.get_profile().stream_type();

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
    }

    void post_processing_filters::map_id(rs2::frame new_frame, rs2::frame old_frame)
    {
        if (auto new_set = new_frame.as<rs2::frameset>())
        {
            if (auto old_set = old_frame.as<rs2::frameset>())
            {
                map_id_frameset_to_frameset(new_set, old_set);
            }
            else
            {
                map_id_frameset_to_frame(new_set, old_frame);
            }
        }
        else if (auto old_set = old_frame.as<rs2::frameset>())
        {
            map_id_frameset_to_frame(old_set, new_frame);
        }
        else
            map_id_frame_to_frame(new_frame, old_frame);
    }

    void post_processing_filters::map_id_frameset_to_frame(rs2::frameset first, rs2::frame second)
    {
        if(auto f = first.first_or_default(second.get_profile().stream_type()))
        {
            auto first_uid = f.get_profile().unique_id();
            auto second_uid = second.get_profile().unique_id();

            viewer.streams_origin[first_uid] = second_uid;
            viewer.streams_origin[second_uid] = first_uid;
        }
    }

    void post_processing_filters::map_id_frameset_to_frameset(rs2::frameset first, rs2::frameset second)
    {
        for (auto&& f : first)
        {
            auto first_uid = f.get_profile().unique_id();
            if (auto second_f = second.first_or_default(f.get_profile().stream_type()))
            {
                auto second_uid = second_f.get_profile().unique_id();

                viewer.streams_origin[first_uid] = second_uid;
                viewer.streams_origin[second_uid] = first_uid;
            }
        }
    }

    void rs2::post_processing_filters::map_id_frame_to_frame(rs2::frame first, rs2::frame second)
    {
        if (first.get_profile().stream_type() == second.get_profile().stream_type())
        {
            auto first_uid = first.get_profile().unique_id();
            auto second_uid = second.get_profile().unique_id();

            viewer.streams_origin[first_uid] = second_uid;
            viewer.streams_origin[second_uid] = first_uid;
        }
    }


    std::vector<rs2::frame> post_processing_filters::handle_frame(rs2::frame f, const rs2::frame_source& source)
    {
        std::vector<rs2::frame> res;

        if (uploader) f = uploader->process(f);

        auto filtered = apply_filters(f, source);

        map_id(filtered, f);

        if (auto composite = filtered.as<rs2::frameset>())
        {
            for (auto&& frame : composite)
            {
                res.push_back(frame);
            }
        }
        else
            res.push_back(filtered);

        if(viewer.is_3d_view)
        {
            if(auto depth = viewer.get_3d_depth_source(filtered))
            {
                switch (depth.get_profile().format())
                {
                    case RS2_FORMAT_DISPARITY32: depth = disp_to_depth.process(depth); break;
                    case RS2_FORMAT_Z16H: depth = depth_decoder.process(depth); break;
                    default: break;
                }

                res.push_back(pc->calculate(depth));
            }
            if(auto texture = viewer.get_3d_texture_source(filtered))
            {
                update_texture(texture);
            }
        }

        return res;
    }


    void post_processing_filters::process(rs2::frame f, const rs2::frame_source& source)
    {
        points p;
        std::vector<frame> results;

        auto res = handle_frame(f, source);
        auto frame = source.allocate_composite_frame(res);

        if(frame)
            source.frame_ready(std::move(frame));
    }

    void post_processing_filters::start()
    {
        stop();
        if (render_thread_active.exchange(true) == false)
        {
            viewer.syncer->start();
            render_thread = std::make_shared<std::thread>([&](){post_processing_filters::render_loop();});
        }
    }

    void post_processing_filters::stop()
    {
        if (render_thread_active.exchange(false) == true)
        {
            viewer.syncer->stop();
            render_thread->join();
            render_thread.reset();
        }
    }
    void post_processing_filters::render_loop()
    {
        while (render_thread_active)
        {
            try
            {
                if(viewer.synchronization_enable)
                {
                    auto frames = viewer.syncer->try_wait_for_frames();
                    for(auto f:frames)
                    {
                        processing_block.invoke(f);
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
                        if (q.second.try_wait_for_frame(&frm, 30))
                        {
                            processing_block.invoke(frm);
                        }
                    }
                }
            }
            catch (...) {}
        }
    }

    void device_model::start_recording(const std::string& path, std::string& error_message)
    {
        if (_recorder != nullptr)
        {
            return; //already recording
        }

        try
        {
            int compression_mode = config_file::instance().get(configurations::record::compression_mode);
            if (compression_mode == 2)
                _recorder = std::make_shared<recorder>(path, dev);
            else
                _recorder = std::make_shared<recorder>(path, dev, compression_mode == 0);

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
            RS2_LOG_SEVERITY_INFO,
            RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR };
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

    int device_model::draw_playback_controls(ux_window& window, ImFont* font, viewer_model& viewer)
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

        const bool supports_playback_step = current_playback_status == RS2_PLAYBACK_STATUS_PAUSED;

        ImGui::PushFont(font);

        //////////////////// Step Backwards Button ////////////////////
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + space_width);
        std::string label = to_string() << textual_icons::step_backward << "##Step Backward " << id;
        if (ImGui::ButtonEx(label.c_str(), button_dim, supports_playback_step ? 0 : ImGuiButtonFlags_Disabled))
        {
            int fps = 0;
            for (auto&& s : viewer.streams)
            {
                if (s.second.profile.fps() > fps)
                    fps = s.second.profile.fps();
            }
            auto curr_frame = p.get_position();
            uint64_t step = uint64_t( 1000.0 / (float)fps * 1e6 );
            p.seek(std::chrono::nanoseconds(curr_frame - step));
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
                    syncer->on_frame = []{};
                    for (auto&& s : subdevices)
                    {
                        s->on_frame = []{};
                        if (s->streaming)
                            s->resume();
                    }
                    
                    p.resume();
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
                viewer.paused = true;
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
            int fps = 0;
            for (auto&& s : viewer.streams)
            {
                if (s.second.profile.fps() > fps)
                    fps = s.second.profile.fps();
            }
            auto curr_frame = p.get_position();
            uint64_t step = uint64_t( 1000.0 / (float)fps * 1e6 );
            p.seek(std::chrono::nanoseconds(curr_frame + step));
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
        draw_info_icon(window, font, button_dim);
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

    int device_model::draw_playback_panel(ux_window& window, ImFont* font, viewer_model& view)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0, 0xae, 0xff, 255));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);


        auto pos = ImGui::GetCursorPos();
        auto controls_height = draw_playback_controls(window, font, view);
        float seek_bar_left_alignment = 4.f;
        ImGui::SetCursorPos({ pos.x + seek_bar_left_alignment, pos.y + controls_height });
        ImGui::PushFont(font);
        auto seek_bar_height = draw_seek_bar();
        ImGui::PopFont();
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
            catch (const std::exception& ex)
            {
                error_message = ex.what();
            }

            ImGui::TreePop();
        }

        ImGui::PopStyleColor();
        return was_set;
    }

    void device_model::draw_info_icon(ux_window& window, ImFont* font, const ImVec2& size)
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
            window.link_hovered();
        }
        ImGui::PopStyleColor(2);
    }

    void device_model::begin_update_unsigned(viewer_model& viewer, std::string& error_message)
    {
        try
        {
            std::vector<uint8_t> data;
            auto ret = file_dialog_open(open_file, "Unsigned Firmware Image\0*.bin\0", NULL, NULL);
            if (ret)
            {
                std::ifstream file(ret, std::ios::binary | std::ios::in);
                if (file.good())
                {
                    data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
                }
                else
                {
                    error_message = to_string() << "Could not open file '" << ret << "'";
                    return;
                }
            }

            else return; // Aborted by the user

            auto manager = std::make_shared<firmware_update_manager>(*this, dev, viewer.ctx, data, false);

            auto n = std::make_shared<fw_update_notification_model>(
                "Manual Update requested", manager, true);
            n->forced = true;
            viewer.not_model.add_notification(n);

            for (auto&& n : related_notifications)
                if (dynamic_cast<fw_update_notification_model*>(n.get()))
                    n->dismiss(false);

            manager->start(n);
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

    void device_model::begin_update(std::vector<uint8_t> data, viewer_model& viewer, std::string& error_message)
    {
        try
        {
            if (data.size() == 0)
            {
                auto ret = file_dialog_open(open_file, "Signed Firmware Image\0*.bin\0", NULL, NULL);
                if (ret)
                {
                    std::ifstream file(ret, std::ios::binary | std::ios::in);
                    if (file.good())
                    {
                        data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
                    }
                    else
                    {
                        error_message = to_string() << "Could not open file '" << ret << "'";
                        return;
                    }
                }
                else return; // Aborted by the user
            }
            
            auto manager = std::make_shared<firmware_update_manager>(*this, dev, viewer.ctx, data, true);

            auto n = std::make_shared<fw_update_notification_model>(
                "Manual Update requested", manager, true);
            viewer.not_model.add_notification(n);
            n->forced = true;

            for (auto&& n : related_notifications)
                n->dismiss(false);

            manager->start(n);
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


    bool subdevice_model::supports_on_chip_calib()
    {
        bool is_d400 = s->supports(RS2_CAMERA_INFO_PRODUCT_LINE) ?
            std::string(s->get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400" : false;

        std::string fw_version = s->supports(RS2_CAMERA_INFO_FIRMWARE_VERSION) ?
            s->get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) : "";

        bool supported_fw = s->supports(RS2_CAMERA_INFO_FIRMWARE_VERSION) ?
            is_upgradeable("05.11.12.0", fw_version) : false;

        return s->is<rs2::depth_sensor>() && is_d400 && supported_fw;
        // TODO: Once auto-calib makes it into the API, switch to querying camera info
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
                int recording_setting = config_file::instance().get(configurations::record::file_save_mode);
                std::string path = "";
                std::string default_path = config_file::instance().get(configurations::record::default_path);
                if (!ends_with(default_path, "/") && !ends_with(default_path, "\\")) default_path += "/";
                std::string default_filename = rs2::get_timestamped_file_name() + ".bag";
                if (recording_setting == 0)
                {
                    path = default_path + default_filename;
                }
                else
                {
                    if (const char* ret = file_dialog_open(file_dialog_mode::save_file, "ROS-bag\0*.bag\0", 
                                            default_path.c_str(), default_filename.c_str()))
                    {
                        path = ret;
                        if (!ends_with(to_lower(path), ".bag")) path += ".bag";
                    }
                }
                
                if (path != "") start_recording(path, error_message);
            }
        }
        if (ImGui::IsItemHovered())
        {
            std::string record_button_hover_text = (!is_streaming ? "Start streaming to enable recording" : (is_recording ? "Stop Recording" : "Start Recording"));
            ImGui::SetTooltip("%s", record_button_hover_text.c_str());
            if (is_streaming) window.link_hovered();
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
        draw_info_icon(window, window.get_font(), device_panel_icons_size);
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
            window.link_hovered();
        }
        ImGui::PopFont();
        ImGui::PushFont(window.get_font());
        static bool keep_showing_advanced_mode_modal = false;
        if (ImGui::BeginPopup(label.c_str()))
        {
            bool something_to_show = false;
            ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
            if (auto tm2_extensions = dev.as<rs2::tm2>())
            {
                something_to_show = true;
                try
                {
                    if (!tm2_extensions.is_loopback_enabled() && ImGui::Selectable("Enable loopback...", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        if (const char* ret = file_dialog_open(file_dialog_mode::open_file, "ROS-bag\0*.bag\0", NULL, NULL))
                        {
                            tm2_extensions.enable_loopback(ret);
                        }
                    }
                    if (tm2_extensions.is_loopback_enabled() && ImGui::Selectable("Disable loopback...", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        tm2_extensions.disable_loopback();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        if (is_streaming)
                            ImGui::SetTooltip("Stop streaming to use loopback functionality");
                        else
                            ImGui::SetTooltip("Enter the device to loopback mode (inject frames from file to FW)");
                    }

                    if (auto tm_sensor = dev.first<pose_sensor>())
                    {
                        if (ImGui::Selectable("Export Localization map"))
                        {
                            if (auto target_path = file_dialog_open(save_file, "Tracking device Localization map (RAW)\0*.map\0", NULL, NULL))
                            {
                                error_message = safe_call([&]()
                                {
                                    std::stringstream ss;
                                    ss << "Exporting localization map to " << target_path << " ... ";
                                    viewer.not_model.add_log(ss.str());
                                    bin_file_from_bytes(target_path, tm_sensor.export_localization_map());
                                    ss.clear();
                                    ss << "completed";
                                    viewer.not_model.add_log(ss.str());
                                });
                            }
                        }

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Retrieve the localization map from device");
                        }

                        if (ImGui::Selectable("Import Localization map", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                        {
                            if (auto source_path = file_dialog_open(open_file, "Tracking device Localization map (RAW)\0*.map\0", NULL, NULL))
                            {
                                error_message = safe_call([&]()
                                {
                                    std::stringstream ss;
                                    ss << "Importing localization map from " << source_path << " ... ";
                                    tm_sensor.import_localization_map(bytes_from_bin_file(source_path));
                                    ss << "completed";
                                    viewer.not_model.add_log(ss.str());
                                });
                            }
                        }

                        if (ImGui::IsItemHovered())
                        {
                            if (is_streaming)
                                ImGui::SetTooltip("Stop streaming to Import localization map");
                            else
                                ImGui::SetTooltip("Load localization map from host to device");
                        }
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

                    ImGui::Separator();
                }

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

                if (dev.is<rs2::updatable>() || dev.is<rs2::update_device>())
                {
                    if (ImGui::Selectable("Update Firmware..."))
                    {
                        begin_update({}, viewer, error_message);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Install official signed firmware from file to the device");

                    if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && is_recommended_fw_available(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE))) 
                    {
                        if (ImGui::Selectable("Install Recommended Firmware "))
                        {
                            auto sensors = dev.query_sensors();
                            auto product_line_str = "";
                            if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE))
                                product_line_str = dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE);
                            if (sensors.size() && sensors.front().supports(RS2_CAMERA_INFO_PRODUCT_LINE))
                                product_line_str = sensors.front().get_info(RS2_CAMERA_INFO_PRODUCT_LINE);
                            int product_line = parse_product_line(product_line_str);

                            auto table = create_default_fw_table();

                            begin_update(table[product_line], viewer, error_message);
                        }
                    }

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Install default recommended firmware for this device");
                }

                bool is_locked = true;
                if (dev.supports(RS2_CAMERA_INFO_CAMERA_LOCKED))
                    is_locked = std::string(dev.get_info(RS2_CAMERA_INFO_CAMERA_LOCKED)) == "YES" ? true : false;

                if (dev.is<rs2::updatable>() && !is_locked)
                {
                    if (ImGui::Selectable("Update Unsigned Firmware..."))
                    {
                        begin_update_unsigned(viewer, error_message);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Install non official unsigned firmware from file to the device");
                }
            }

            bool has_autocalib = false;
            for (auto&& sub : subdevices)
            {
                if (sub->supports_on_chip_calib() && !has_autocalib)
                {
                    something_to_show = true;
                    if (ImGui::Selectable("On-Chip Calibration"))
                    {
                        try
                        {
                            auto manager = std::make_shared<on_chip_calib_manager>(viewer, sub, *this, dev);
                            auto n = std::make_shared<autocalib_notification_model>(
                                "", manager, false);

                            viewer.not_model.add_notification(n);
                            n->forced = true;
                            n->update_state = autocalib_notification_model::RS2_CALIB_STATE_SELF_INPUT;

                            for (auto&& n : related_notifications)
                                if (dynamic_cast<autocalib_notification_model*>(n.get()))
                                    n->dismiss(false);
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
                        ImGui::SetTooltip(  "This will improve the depth noise.\n"
                                            "Point at a scene that normally would have > 50 %% valid depth pixels,\n"
                                            "then press calibrate."
                                            "The health-check will be calculated.\n"
                                            "If >0.25 we recommend applying the new calibration.\n"
                                            "\"White wall\" mode should only be used when pointing at a flat white wall with projector on");

                    if (ImGui::Selectable("Tare Calibration"))
                    {
                        try
                        {
                            auto manager = std::make_shared<on_chip_calib_manager>(viewer, sub, *this, dev);
                            auto n = std::make_shared<autocalib_notification_model>(
                                "", manager, false);

                            viewer.not_model.add_notification(n);
                            n->forced = true;
                            n->update_state = autocalib_notification_model::RS2_CALIB_STATE_TARE_INPUT;

                            for (auto&& n : related_notifications)
                                if (dynamic_cast<autocalib_notification_model*>(n.get()))
                                    n->dismiss(false);
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
                        ImGui::SetTooltip(  "Tare calibration is used to adjust camera absolute distance to flat target.\n" 
                                            "User needs to enter the known ground truth");

                    has_autocalib = true;
                }
            }
            if (!has_autocalib)
            {
                bool selected = false;
                something_to_show = true;
                ImGui::Selectable("On-Chip Calibration", &selected, ImGuiSelectableFlags_Disabled);
                ImGui::Selectable("Tare Calibration", &selected, ImGuiSelectableFlags_Disabled);
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

        ImGui::PushStyleColor(ImGuiCol_Text, record_button_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, record_button_color);
        ImGui::ButtonEx(is_recording ? "Stop" : "Record", device_panel_icons_size, (!is_streaming ? ImGuiButtonFlags_Disabled : 0));
        if (ImGui::IsItemHovered() && is_streaming) window.link_hovered();
        ImGui::PopStyleColor(2);
        
        ImGui::SameLine();  ImGui::ButtonEx("Sync", device_panel_icons_size, ImGuiButtonFlags_Disabled);

        auto info_button_color = show_device_info ? light_blue : light_grey;
        ImGui::PushStyleColor(ImGuiCol_Text, info_button_color);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, info_button_color);
        ImGui::SameLine(); ImGui::ButtonEx("Info", device_panel_icons_size);
        if (ImGui::IsItemHovered()) window.link_hovered();
        ImGui::PopStyleColor(2);

        ImGui::SameLine(); ImGui::ButtonEx("More", device_panel_icons_size);
        if (ImGui::IsItemHovered()) window.link_hovered();
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(7);
        ImGui::PopFont();

        return device_panel_height;
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

            bool found = false;
            for (int i = 0; i < RS2_FORMAT_COUNT; i++)
            {
                auto format = (rs2_format)i;
                if (ends_with(rs2_format_to_string(format), formatstr))
                {
                    requested_streams[std::make_pair(RS2_STREAM_INFRARED, 1)].format = format;
                    found = true;
                }
            }

            if (!found)
            {
                if (formatstr == "R8L8")
                {
                    requested_streams[std::make_pair(RS2_STREAM_INFRARED, 1)].format = RS2_FORMAT_Y8;
                    requested_streams[std::make_pair(RS2_STREAM_INFRARED, 2)].format = RS2_FORMAT_Y8;
                }
                else
                {
                    throw std::runtime_error(to_string() << "Unsupported stream-ir-format: " << formatstr);
                }
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
                    size_t format_id = 0;
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
                    sub->ui.selected_format_id[uid] = int(format_id);

                    //Find fps
                    int requested_fps = kvp.second.fps;
                    size_t fps_id = 0;
                    for (; fps_id < sub->shared_fps_values.size(); fps_id++)
                    {
                        if (sub->shared_fps_values[fps_id] == requested_fps)
                            break;
                    }
                    if (fps_id == sub->shared_fps_values.size())
                    {
                        throw std::runtime_error(to_string() << "No match found for requested fps: " << requested_fps);
                    }
                    sub->ui.selected_shared_fps_id = int(fps_id);

                    //Find Resolution
                    std::pair<int, int> requested_res{ kvp.second.width,kvp.second.height };
                    size_t res_id = 0;
                    for (; res_id < sub->res_values.size(); res_id++)
                    {
                        if (sub->res_values[res_id] == requested_res)
                            break;
                    }
                    if (res_id == sub->res_values.size())
                    {
                        throw std::runtime_error(to_string() << "No match found for requested resolution: " << requested_res.first << "x" << requested_res.second);
                    }
                    sub->ui.selected_res_id = int(res_id);
                }
            }
        }
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
        auto serializable = dev.as<serializable_device>();

        const auto load_json = [&, serializable](const std::string f) {
            std::ifstream file(f);
            if (!file.good())
            {
                //Failed to read file, removing it from the available ones
                advanced_mode_settings_file_names.erase(f);
                selected_file_preset.clear();
                throw std::runtime_error(to_string() << "Failed to read configuration file:\n\"" << f << "\"\nRemoving it from presets.");
            }
            std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            if (serializable)
            {
                serializable.load_json(str);
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

        const auto save_to_json = [&, serializable](std::string full_filename)
        {
            if (!ends_with(to_lower(full_filename), ".json")) full_filename += ".json";
            std::ofstream outfile(full_filename);
            json saved_configuraion;
            if (serializable)
            {
                saved_configuraion = json::parse(serializable.serialize_json());
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
                    ImGui::Text("Preset: ");
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
                            *opt_model.invalidate_flag = true;

                            auto advanced = dev.as<advanced_mode>();
                            if (advanced)
                                if (!advanced.is_enabled())
                                    keep_showing_popup = true;

                            if(!keep_showing_popup)
                            {
                                if (selected < static_cast<int>(labels.size() - files_labels.size()))
                                {
                                    //Known preset was chosen
                                    auto new_val = opt_model.range.min + opt_model.range.step * selected;
                                    model.add_log(to_string() << "Setting " << opt_model.opt << " to "
                                        << opt_model.value << " (" << labels[selected] << ")");

                                    opt_model.endpoint->set_option(opt_model.opt, new_val);

                                    // Only apply preset to GUI if set_option was succesful
                                    selected_file_preset = "";
                                    opt_model.value = new_val;
                                    is_clicked = true;
                                }
                                else
                                {
                                    //File was chosen
                                    auto file = selected - static_cast<int>(labels.size() - files_labels.size());
                                    if(file < 0 || file >= full_files_names.size())
                                        throw std::runtime_error("not a valid format");
                                    auto f = full_files_names[file];
                                    error_message = safe_call([&]() { load_json(f); });
                                    selected_file_preset = f;
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
        const int buttons_flags = serializable ? 0 : ImGuiButtonFlags_Disabled;
        static bool require_advanced_mode_enable_prompt = false;
        auto advanced_dev = dev.as<advanced_mode>();
        auto is_advanced_device = false;
        auto is_advanced_mode_enabled = false;
        if (advanced_dev)
        {
            is_advanced_device = true;
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
            if (serializable && (!is_advanced_device || is_advanced_mode_enabled))
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
            if (serializable && (!is_advanced_device || is_advanced_mode_enabled))
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

        for (size_t i = 0; i < curr_values.size(); i++)
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

    bool rs2::device_model::is_streaming() const
    {
        return std::any_of(subdevices.begin(), subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm)
        {
            return sm->streaming;
        });
    }

    void device_model::draw_controls(float panel_width, float panel_height,
        ux_window& window,
        std::string& error_message,
        device_model*& device_to_remove,
        viewer_model& viewer, float windows_width,
        std::vector<std::function<void()>>& draw_later,
        bool load_json_if_streaming,
        json_loading_func json_loading,
        bool draw_device_outline)
    {
        ////////////////////////////////////////
        // draw device header
        ////////////////////////////////////////
        const bool is_playback_device = dev.is<playback>();
        bool is_ip_device = dev.supports(RS2_CAMERA_INFO_IP_ADDRESS);
        auto header_h = panel_height;
        if (is_playback_device || is_ip_device) header_h += 15;

        ImColor device_header_background_color = title_color;
        const float left_space = 3.f;
        const float upper_space = 3.f;

        bool update_read_only_options = _update_readonly_options_timer;

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
        if(dev.supports(RS2_CAMERA_INFO_NAME))
            ss << dev.get_info(RS2_CAMERA_INFO_NAME);
        if(is_ip_device)
        {
            ImGui::Text(" %s", ss.str().substr(0, ss.str().find("\n IP Device")).c_str());

            ImGui::PushFont(window.get_font());
            ImGui::Text("\tNetwork Device at %s", dev.get_info(RS2_CAMERA_INFO_IP_ADDRESS));
            ImGui::PopFont();
        }
        else
        {
            ImGui::Text(" %s", ss.str().c_str());

            if (dev.supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
            {
                std::string desc = dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
                ss.str("");
                ss << "   " << textual_icons::usb_type << " " << desc;
                ImGui::SameLine();
                if (!starts_with(desc, "3.")) ImGui::PushStyleColor(ImGuiCol_Text, yellow);
                else ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::Text(" %s", ss.str().c_str());
                ImGui::PopStyleColor();
                ss.str("");
                ss << "The camera was detected by the OS as connected to a USB " << desc << " port";
                ImGui::PushFont(window.get_font());
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(" %s", ss.str().c_str());
                ImGui::PopStyleColor();
                ImGui::PopFont();
            }
        }
            
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

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Remove selected device from current view\n(can be restored by clicking Add Source)");
                window.link_hovered();
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
        auto serialize = dev.is<serializable_device>();
        if (serialize)
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
            auto playback_panel_height = draw_playback_panel(window, window.get_font(), viewer);
            ImGui::SetCursorPos({ playback_panel_pos.x, playback_panel_pos.y + playback_panel_height });
        }

        bool is_streaming = std::any_of(subdevices.begin(), subdevices.end(), [](const std::shared_ptr<subdevice_model>& sm)
        {
            return sm->streaming;
        });

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

            ImGui::SetCursorPos({ rc.x + 225, rc.y - 107 });
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

                        std::vector<stream_profile> profiles;
                        auto is_comb_supported = sub->is_selected_combination_supported();
                        bool can_stream = false;
                        if (is_comb_supported)
                            can_stream = true;
                        else
                        {
                            profiles = sub->get_supported_profiles();
                            if (!profiles.empty())
                                can_stream = true;
                            else
                            {
                                std::string text = to_string() << "  " << textual_icons::toggle_off << "\noff   ";
                                ImGui::TextDisabled("%s", text.c_str());
                                if (std::any_of(sub->stream_enabled.begin(), sub->stream_enabled.end(), [](std::pair<int, bool> const& s) { return s.second; }))
                                {
                                    if (ImGui::IsItemHovered())
                                    {
                                        // ImGui::SetTooltip("Selected configuration (FPS, Resolution) is not supported");
                                        ImGui::SetTooltip("Selected value is not supported");
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
                        if (can_stream)
                        {
                            if (ImGui::Button(label.c_str(), { 30,30 }))
                            {
                                if (profiles.empty()) // profiles might be already filled
                                    profiles = sub->get_selected_profiles();
                                try
                                {
                                    if (!dev_syncer)
                                        dev_syncer = viewer.syncer->create_syncer();

                                    std::string friendly_name = sub->s->get_info(RS2_CAMERA_INFO_NAME);
                                    if (!viewer.zo_sensors.load() &&
                                            ((friendly_name.find("Tracking") != std::string::npos) ||
                                            (friendly_name.find("Motion") != std::string::npos)))
                                    {
                                        viewer.synchronization_enable = false;
                                    }
                                    _update_readonly_options_timer.signal();
                                    sub->play(profiles, viewer, dev_syncer);
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
                                window.link_hovered();
                                ImGui::SetTooltip("Start streaming data from this sensor");
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
                                stop_recording = true;
                                _update_readonly_options_timer.signal();
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            window.link_hovered();
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

                static const std::vector<rs2_option> drawing_order = serialize ?
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
                        for (auto&& i:sub->s->get_supported_options())
                        {
                            auto opt = static_cast<rs2_option>(i);
                            if (skip_option(opt)) continue;
                            if (std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
                            {
                                if (serialize && opt == RS2_OPTION_VISUAL_PRESET)
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
                        sub->_options_invalidated = true;
                        selected_file_preset.clear();
                    }
                }

                if (sub->s->is<depth_sensor>()) {
                    for (auto&& pb : sub->const_effects)
                    {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                        label = to_string() << pb->get_name() << "##" << id;
                        if (ImGui::TreeNode(label.c_str()))
                        {
                            for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                            {
                                auto opt = static_cast<rs2_option>(i);
                                if (skip_option(opt)) continue;
                                pb->get_option(opt).draw_option(
                                    dev.is<playback>() || update_read_only_options,
                                    false, error_message, viewer.not_model);
                            }

                            ImGui::TreePop();
                        }
                    }
                }

                if (sub->post_processing.size() > 0)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    const ImVec2 pos = ImGui::GetCursorPos();

                    draw_later.push_back([windows_width, &window, sub, pos, &viewer, this]() {
                        if (!sub->streaming) ImGui::SetCursorPos({ windows_width - 34 , pos.y - 3 });
                        else ImGui::SetCursorPos({ windows_width - 34, pos.y - 3 });

                        try
                        {

                            ImGui::PushFont(window.get_font());

                            ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                            if (!sub->post_processing_enabled)
                            {
                                std::string label = to_string() << " " << textual_icons::toggle_off << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << ",post";

                                ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                                if (ImGui::Button(label.c_str(), { 30,24 }))
                                {
                                    if (sub->zero_order_artifact_fix && sub->zero_order_artifact_fix->is_enabled())
                                        sub->verify_zero_order_conditions();
                                    sub->post_processing_enabled = true;
                                    config_file::instance().set(get_device_sensor_name(sub.get()).c_str(),
                                        sub->post_processing_enabled);
                                    for( auto&& pb : sub->post_processing )
                                    {
                                        if( !pb->visible )
                                            continue;
                                        if( pb->is_enabled() )
                                            pb->processing_block_enable_disable( true );
                                    }
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Enable post-processing filters");
                                    window.link_hovered();
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
                                    config_file::instance().set(get_device_sensor_name(sub.get()).c_str(),
                                        sub->post_processing_enabled);
                                    for( auto&& pb : sub->post_processing )
                                    {
                                        if( !pb->visible )
                                            continue;
                                        if( pb->is_enabled() )
                                            pb->processing_block_enable_disable( false );
                                    }
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Disable post-processing filters");
                                    window.link_hovered();
                                }
                            }
                            ImGui::PopStyleColor(5);
                            ImGui::PopFont();
                        }
                        catch (...)
                        {
                            ImGui::PopStyleColor(5);
                            ImGui::PopFont();
                            throw;
                        }
                    });

                    label = to_string() << "Post-Processing##" << id;
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        for (auto&& pb : sub->post_processing)
                        {
                            if (!pb->visible) continue;

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                            const ImVec2 pos = ImGui::GetCursorPos();
                            const ImVec2 abs_pos = ImGui::GetCursorScreenPos();

                            draw_later.push_back([windows_width, &window, sub, pos, &viewer, this, pb]() {
                                if (!sub->streaming || !sub->post_processing_enabled) ImGui::SetCursorPos({ windows_width - 35, pos.y -3 });
                                else
                                    ImGui::SetCursorPos({ windows_width - 35, pos.y - 3 });

                                try
                                {
                                    ImGui::PushFont(window.get_font());

                                    ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                                    if (!sub->post_processing_enabled)
                                    {
                                        if (!pb->is_enabled())
                                        {
                                            std::string label = to_string() << " " << textual_icons::toggle_off << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();

                                            ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);
                                            ImGui::ButtonEx(label.c_str(), { 25,24 }, ImGuiButtonFlags_Disabled);
                                        }
                                        else
                                        {
                                            std::string label = to_string() << " " << textual_icons::toggle_on << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();
                                            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);
                                            ImGui::ButtonEx(label.c_str(), { 25,24 }, ImGuiButtonFlags_Disabled);
                                        }
                                    }
                                    else
                                    {
                                        if (!pb->is_enabled())
                                        {
                                            std::string label = to_string() << " " << textual_icons::toggle_off << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();

                                            ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                                            if (ImGui::Button(label.c_str(), { 25,24 }))
                                            {
                                                if (pb->get_block()->is<zero_order_invalidation>())
                                                    sub->verify_zero_order_conditions();
                                                pb->enable( true );
                                                pb->save_to_config_file();
                                            }
                                            if (ImGui::IsItemHovered())
                                            {
                                                label = to_string() << "Enable " << pb->get_name() << " post-processing filter";
                                                ImGui::SetTooltip("%s", label.c_str());
                                                window.link_hovered();
                                            }
                                        }
                                        else
                                        {
                                            std::string label = to_string() << " " << textual_icons::toggle_on << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();
                                            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                                                if (ImGui::Button(label.c_str(), { 25,24 }))
                                                {
                                                    pb->enable( false );
                                                    pb->save_to_config_file();
                                                }
                                                if (ImGui::IsItemHovered())
                                                {
                                                    label = to_string() << "Disable " << pb->get_name() << " post-processing filter";
                                                    ImGui::SetTooltip("%s", label.c_str());
                                                window.link_hovered();
                                            }
                                        }
                                    }

                                    ImGui::PopStyleColor(5);
                                    ImGui::PopFont();
                                }
                                catch (...)
                                {
                                    ImGui::PopStyleColor(5);
                                    ImGui::PopFont();
                                    throw;
                                }
                            });

                            label = to_string() << pb->get_name() << "##" << id;
                            if (ImGui::TreeNode(label.c_str()))
                            {
                                for (auto&& opt : pb->get_option_list())
                                {
                                    if (skip_option(opt)) continue;
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

    void device_model::handle_hardware_events(const std::string& serialized_data)
    {
        //TODO: Move under hour glass
    }

    device_changes::device_changes(rs2::context& ctx)
    {
        _changes.emplace(rs2::device_list{}, ctx.query_devices(RS2_PRODUCT_LINE_ANY));
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

    // Aggregate the trajectory path
    void tm2_model::update_model_trajectory(const pose_frame& pose, bool track)
    {
        static bool prev_track = track;
        if (!_trajectory_tracking)
            return;

        if (track)
        {
            // Reset the waypoints on stream resume
            if (!prev_track)
                reset_trajectory();

            rs2_pose pose_data = const_cast<pose_frame&>(pose).get_pose_data();
            auto t = tm2_pose_to_world_transformation(pose_data);
            float model[4][4];
            t.to_column_major((float*)model);

            rs2_vector translation{ t.mat[0][3], t.mat[1][3], t.mat[2][3] };
            tracked_point p{ translation , pose_data.tracker_confidence }; //TODO: Osnat - use tracker_confidence or mapper_confidence ?
            // register the new waypoint
            add_to_trajectory(p);
        }

        prev_track = track;
    }

    void tm2_model::draw_trajectory(bool is_trajectory_button_pressed)
    {
        if (!is_trajectory_button_pressed)
        {
            record_trajectory(false);
            reset_trajectory();
            return;
        }

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
}
