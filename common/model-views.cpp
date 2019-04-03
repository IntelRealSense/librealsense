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
#include <imgui_internal.h>

#define ARCBALL_CAMERA_IMPLEMENTATION
#include <arcball_camera.h>

#include "os.h"

constexpr const char* recommended_fw_url = "https://downloadcenter.intel.com/download/27522/Latest-Firmware-for-Intel-RealSense-D400-Product-Family?v=t";
constexpr const char* store_url = "https://click.intel.com/realsense.html";

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

namespace rs2
{
    namespace models
    {
        struct short3
        {
            uint16_t x, y, z;
        };

        #include <res/d435.h>
        #include <res/d415.h>
        #include <res/sr300.h>
        #include <res/t265.h>
    }

    typedef void(*load_function)(std::vector<rs2::float3>&,
        std::vector<rs2::float3>&, std::vector<models::short3>&);

    obj_mesh load_model(load_function f)
    {
        obj_mesh res;
        std::vector<models::short3> idx;
        f(res.positions, res.normals, idx);
        for (auto i : idx)
            res.indexes.push_back({ i.x, i.y, i.z });
        return res;
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

    void open_issue(const std::vector<device_model>& devices)
    {
        std::stringstream ss;

        rs2_error* e = nullptr;

        ss << "| | |\n";
        ss << "|---|---|\n";
        ss << "|**librealsense**|" << api_version_to_string(rs2_get_api_version(&e)) << (is_debug() ? " DEBUG" : " RELEASE") << "|\n";
        ss << "|**OS**|" << get_os_name() << "|\n";

        for (auto& dm : devices)
        {
            for (auto& kvp : dm.infos)
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

    void export_to_ply(const std::string& fname, notifications_model& ns, points p, video_frame texture, bool notify)
    {
        std::thread([&ns, p, texture, fname, notify]() mutable {
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
                model.add_notification({ to_string() << "Could not refresh read-only option " << endpoint->get_option_name(opt) << ": " << e.what(),
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
        : _owner(owner), _name(name), _block(block), _invoker(invoker), enabled(enable)
    {
        std::stringstream ss;
        ss << "##" << ((owner) ? owner->dev.get_info(RS2_CAMERA_INFO_NAME) : _name)
            << "/" << ((owner) ? (*owner->s).get_info(RS2_CAMERA_INFO_NAME) : "_")
            << "/" << (long long)this;

        if (_owner)
            _full_name = get_device_sensor_name(_owner) + "." + _name;
        else
            _full_name = _name;

        enabled = restore_processing_block(_full_name.c_str(),
                                           block, enabled);

        populate_options(ss.str().c_str(), owner, owner ? &owner->_options_invalidated : nullptr, error_message);
    }

    void processing_block_model::save_to_config_file()
    {
        save_processing_block_to_config_file(_full_name.c_str(), _block, enabled);
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
        std::shared_ptr<sensor> s, std::string& error_message)
        : s(s), dev(dev), tm2(), ui(), last_valid_ui(),
        streaming(false), _pause(false),
        depth_colorizer(std::make_shared<rs2::colorizer>()),
        yuy2rgb(std::make_shared<rs2::yuy_decoder>())
    {
        restore_processing_block("colorizer", depth_colorizer);
        restore_processing_block("yuy2rgb", yuy2rgb);

        std::stringstream ss;
        ss << configurations::viewer::post_processing 
           << "." << dev.get_info(RS2_CAMERA_INFO_NAME)
           << "." << s->get_info(RS2_CAMERA_INFO_NAME);
        auto key = ss.str();

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

        for (auto&& f : s->get_recommended_filters())
        {
            auto shared_filter = std::make_shared<filter>(f);
            auto model = std::make_shared<processing_block_model>(
                this, shared_filter->get_info(RS2_CAMERA_INFO_NAME), shared_filter,
                [=](rs2::frame f) { return shared_filter->process(f); }, error_message);

            if (shared_filter->is<disparity_transform>())
                model->visible = false;

            if (shared_filter->is<zero_order_invalidation>())
                zero_order_artifact_fix = model;

            if (shared_filter->is<hole_filling_filter>())
                model->enabled = false;

            post_processing.push_back(model);
        }

        auto colorizer = std::make_shared<processing_block_model>(
            this, "Depth Visualization", depth_colorizer,
            [=](rs2::frame f) { return depth_colorizer->colorize(f); }, error_message);
        const_effects.push_back(colorizer);

        ss.str("");
        ss << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
            << "/" << s->get_info(RS2_CAMERA_INFO_NAME)
            << "/" << (long long)this;
        populate_options(options_metadata, ss.str().c_str(), this, s, &_options_invalidated, error_message);

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
            auto resolution_constrain = usb2 ? std::make_pair(640, 480) : std::make_pair(1280, 720);

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
        if(zero_order_artifact_fix && zero_order_artifact_fix->enabled && 
            (f.get_profile().stream_type() == RS2_STREAM_DEPTH || f.get_profile().stream_type() == RS2_STREAM_INFRARED || f.get_profile().stream_type() == RS2_STREAM_CONFIDENCE))
            return true;
        if (!viewer.is_3d_view || viewer.is_3d_depth_source(f) || viewer.is_3d_texture_source(f))
            return true;

        return false;
    }

    void subdevice_model::play(const std::vector<stream_profile>& profiles, viewer_model& viewer, std::shared_ptr<rs2::asynchronous_syncer> syncer)
    {
        if (post_processing_enabled && zero_order_artifact_fix && zero_order_artifact_fix->enabled)
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
    {}

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

    void viewer_model::render_pose(rs2::rect stream_rect, float buttons_heights, ImGuiWindowFlags flags)
    {
        int num_of_pose_buttons = 3; // trajectory, grid, info

        ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + buttons_heights });
        ImGui::SetNextWindowSize({ stream_rect.w, buttons_heights });
        std::string pose_label = to_string() << "header of 3dviewer - pose";
        ImGui::Begin(pose_label.c_str(), nullptr, flags);

        // Draw selection buttons on the pose header, the buttons are global to all the streaming devices
        ImGui::SetCursorPos({ stream_rect.w - 32 * num_of_pose_buttons - 5, 0 });

        bool color_icon = pose_info_object_button.is_pressed(); //draw trajectory is on - color the icon
        if (color_icon)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
        }

        // Draw info object button (is not synchronized with the info buttons in the 2D view)
        if (ImGui::Button(pose_info_object_button.get_icon().c_str(), { 24, buttons_heights }))
        {
            show_pose_info_3d = !show_pose_info_3d;
            pose_info_object_button.toggle_button();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", pose_info_object_button.get_tooltip().c_str());
        }
        if (color_icon)
        {
            ImGui::PopStyleColor(2);
        }

        // Draw grid object button
        ImGui::SameLine();

        if (ImGui::Button(grid_object_button.get_icon().c_str(), { 24, buttons_heights }))
        {
            ImGui::OpenPopup("Grid Configuration");
        }

        if (ImGui::BeginPopupModal("Grid Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysUseWindowPadding))
        {
            for (auto&& s : streams)
            {
                if (s.second.is_stream_visible() && s.second.profile.stream_type() == RS2_STREAM_POSE)
                {
                    ImGui::Text(" Unit Scale:");
                    ImGui::Text("     1 unit =");
                    ImGui::SameLine();

                    ImGui::PushItemWidth(100);
                    float currentStep = s.second.texture->currentGrid.step;
                    if (ImGui::InputFloat("##grid_step", &currentStep, 0.1f, 1.0f, 2))
                    {
                        if (currentStep >= 0.01)
                        {
                            s.second.texture->currentGrid.set(currentStep);
                        }
                    }
                    ImGui::PopItemWidth();

                    ImGui::SameLine();

                    ImGui::PushItemWidth(80);
                    int currentGridUnit = s.second.texture->currentGrid.unit;
                    if (ImGui::Combo("##grid_scale_combo", &currentGridUnit, "Meter\0Feet\0\0"))
                    {
                        s.second.texture->currentGrid.set((grid_step_unit)currentGridUnit);
                    }
                    ImGui::PopItemWidth();


                    ImGui::Separator();

                    int boxHorizontalLength = s.second.texture->currentGrid.boxHorizontalLength;
                    int boxVerticalLength = s.second.texture->currentGrid.boxVerticalLength;
                    int boxVerticaAlignment = s.second.texture->currentGrid.boxVerticalAlignment;

                    ImGui::Text(" Display");

                    ImGui::Columns(2, 0, false);
                    ImGui::SetColumnOffset(1, 100);

                    ImGui::Text("     Horizontal:");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(148);
                    if (ImGui::SliderIntWithSteps("##boxHorizontalLength", &boxHorizontalLength, 0, 100, 2))
                    {
                        s.second.texture->currentGrid.boxHorizontalLength = boxHorizontalLength;
                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("     Vertical:  ");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(148);
                    if (ImGui::SliderIntWithSteps("##boxVerticalLength", &boxVerticalLength, 1, 99, 2))
                    {
                        s.second.texture->currentGrid.boxVerticalLength = boxVerticalLength;
                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Text("     Vertical Alignment:  ");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(148);
                    if (ImGui::SliderIntWithSteps("##boxVerticalAlignment", &boxVerticaAlignment, -100, 100, 1))
                    {
                        s.second.texture->currentGrid.boxVerticalAlignment = boxVerticaAlignment;
                    }
                    ImGui::PopItemWidth();

                    ImGui::Columns();

                    if (ImGui::Button("OK", ImVec2(80, 0)))
                    {
                        s.second.texture->previousGrid = s.second.texture->currentGrid;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(80, 0)))
                    {
                        s.second.texture->currentGrid.set(s.second.texture->previousGrid);
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Reset", ImVec2(80, 0)))
                    {
                        pose_grid resetGrid;
                        s.second.texture->currentGrid.set(resetGrid);
                        s.second.texture->previousGrid = s.second.texture->currentGrid;
                        ImGui::CloseCurrentPopup();
                    }
                    break;
                }
            }

            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", grid_object_button.get_tooltip().c_str());
        }

        ImGui::SameLine();

        color_icon = trajectory_button.is_pressed(); //draw trajectory is on - color the icon
        if (color_icon)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
        }
        if (ImGui::Button(trajectory_button.get_icon().c_str(), { 24, buttons_heights }))
        {
            trajectory_button.toggle_button();
            for (auto&& s : streams)
            {
                if (s.second.profile.stream_type() == RS2_STREAM_POSE)
                    streams[s.second.profile.unique_id()].dev->tm2.record_trajectory(trajectory_button.is_pressed());
            }
        }
        if (color_icon)
        {
            ImGui::PopStyleColor(2);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", trajectory_button.get_tooltip().c_str());
        }

        ImGui::End();
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
                s.second.profile.stream_type() == RS2_STREAM_DEPTH)
            {
                if (selected_depth_source_uid == -1)
                {
                    if (streams_origin.find(s.second.profile.unique_id()) != streams_origin.end() &&
                        streams.find(streams_origin[s.second.profile.unique_id()]) != streams.end())
                    {
                        selected_depth_source_uid = streams_origin[s.second.profile.unique_id()];
                    }   
                }
                if (streams_origin.find(s.second.profile.unique_id()) != streams_origin.end() && streams_origin[s.second.profile.unique_id()] == selected_depth_source_uid)
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
                (s.second.profile.stream_type() == RS2_STREAM_COLOR ||
                 s.second.profile.stream_type() == RS2_STREAM_INFRARED ||
                 s.second.profile.stream_type() == RS2_STREAM_DEPTH ||
                 s.second.profile.stream_type() == RS2_STREAM_FISHEYE))
            {
                if (selected_tex_source_uid == -1 && selected_depth_source_uid != -1)
                {
                    if (streams_origin.find(s.second.profile.unique_id()) != streams_origin.end() &&
                        streams.find(streams_origin[s.second.profile.unique_id()]) != streams.end())
                    {
                        selected_tex_source_uid = streams_origin[s.second.profile.unique_id()];
                    }                         
                }
                if ((streams_origin.find(s.second.profile.unique_id()) != streams_origin.end() &&streams_origin[s.second.profile.unique_id()] == selected_tex_source_uid))
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
            if(streams.find(selected_tex_source_uid)!= streams.end())
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
            ImGuiWindowFlags_NoBringToFrontOnFocus |
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
            if (streams.find(tex_sources[selected_tex_source]) != streams.end())
                selected_tex_source_uid = tex_sources[selected_tex_source];

            ImGui::PopItemWidth();

            ImGui::SameLine();

            // Occlusion control for RGB UV-Map uses option's description as label
            // Position is dynamically adjusted to avoid overlapping on resize
            if (streams.find(selected_tex_source_uid) != streams.end())
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
                if (selected_tex_source_uid >= 0 && streams.find(selected_tex_source_uid) != streams.end())
                {
                    tex = streams[selected_tex_source_uid].texture->get_last_frame(true);
                    if (tex) ppf.update_texture(tex);
                }

                std::string fname(ret);
                if (!ends_with(to_lower(fname), ".ply")) fname += ".ply";
                export_to_ply(fname.c_str(), not_model, last_points, last_texture->get_last_frame());
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
        bool pose_render = false;

        for (auto&& s : streams)
        {
            if (s.second.is_stream_visible() &&
                s.second.profile.stream_type() == RS2_STREAM_POSE)
            {
                pose_render = true;
                break;
            }
        }

        if (pose_render)
        {
            render_pose(stream_rect, buttons_heights, flags);
        }

        auto total_top_bar_height = top_bar_height * (1 + pose_render); // may include single bar or additional bar for pose

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

        ImGui::SetNextWindowPos({ stream_rect.x + stream_rect.w - 215, stream_rect.y + total_top_bar_height + 5 });
        ImGui::SetNextWindowSize({ 260, 65 });
        ImGui::Begin("3D Info box", nullptr, flags);

        ImGui::Columns(2, 0, false);
        ImGui::SetColumnOffset(1, 60);

        ImGui::Text("Rotate:");
        ImGui::NextColumn();
        ImGui::Text("Left Mouse Button");
        ImGui::NextColumn();

        ImGui::Text("Pan:");
        ImGui::NextColumn();
        ImGui::Text("Middle Mouse Button");
        ImGui::NextColumn();

        ImGui::Text("Zoom:");
        ImGui::NextColumn();
        ImGui::Text("Mouse Wheel");
        ImGui::NextColumn();

        ImGui::Columns();

        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);


        ImGui::PopFont();
    }

    void viewer_model::update_configuration()
    {
        continue_with_ui_not_aligned = config_file::instance().get_or_default(
            configurations::viewer::continue_with_ui_not_aligned, false);
        is_3d_view = config_file::instance().get_or_default(
            configurations::viewer::is_3d_view, false);

        auto min_severity = (rs2_log_severity)config_file::instance().get_or_default(
            configurations::viewer::log_severity, 2);

        if (config_file::instance().get_or_default(
            configurations::viewer::log_to_console, false))
        {
            rs2::log_to_console(min_severity);
        }
        if (config_file::instance().get_or_default(
            configurations::viewer::log_to_file, false))
        {
            std::string filename = config_file::instance().get(
                configurations::viewer::log_filename);

            rs2::log_to_file(min_severity, filename.c_str());
        }
    }

    viewer_model::viewer_model()
            : ppf(*this), 
              synchronization_enable(true)
    {
        syncer = std::make_shared<syncer_model>();
        reset_camera();
        rs2_error* e = nullptr;
        not_model.add_log(to_string() << "librealsense version: " << api_version_to_string(rs2_get_api_version(&e)) << "\n");
    
        update_configuration();

        camera_mesh.push_back(load_model(models::uncompress_d415_obj));
        camera_mesh.push_back(load_model(models::uncompress_d435_obj));
        camera_mesh.push_back(load_model(models::uncompress_sr300_obj));
        camera_mesh.push_back(load_model(models::uncompress_t265_obj));

        for (auto&& mesh : camera_mesh)
            for (auto& xyz : mesh.positions)
            {
                xyz = xyz / 1000.f;
                xyz.x *= -1;
                xyz.y *= -1;
            }
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
        if (RS2_STREAM_POSE == profile.stream_type()) ++num_of_buttons; // Grid button

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoFocusOnAppearing | 
            ImGuiWindowFlags_NoBringToFrontOnFocus;

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


        if (RS2_STREAM_POSE == profile.stream_type())
        {
            label = to_string() << textual_icons::grid << "##grid " << profile.unique_id();
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                auto newGrid = this->texture->currentGrid;
                float switchToStep = 0;

                switch (this->texture->currentGrid.unit)
                {
                    case GRID_STEP_UNIT_METER:
                        newGrid.set(1.f);
                        newGrid.set(GRID_STEP_UNIT_FEET);
                        break;
                    case GRID_STEP_UNIT_FEET:
                        newGrid.set(1.f);
                        newGrid.set(GRID_STEP_UNIT_METER);
                        break;
                }

                this->texture->currentGrid.set(newGrid);
            }

            if (ImGui::IsItemHovered())
            {
                grid_step_unit currentGridUnit = this->texture->currentGrid.unit;
                std::string switchToUnit = "";
                float switchToStep = 0;
                switch (currentGridUnit)
                {
                    case GRID_STEP_UNIT_METER:
                        switchToUnit = "Feet";
                        switchToStep = 1.0f;
                        break;
                    case GRID_STEP_UNIT_FEET:
                        switchToUnit = "Meter";
                        switchToStep = 1.f;
                        break;
                }

                ImGui::SetTooltip("Switch to %0.2f %s grid", switchToStep, switchToUnit.c_str());
            }

            ImGui::SameLine();
        }

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
            auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

            ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(9, 11, 13, 100));

            static const auto height_info_rect = 10.f;
            static const auto y_offset_info_rect = 5.f;
            static const auto x_offset_info_rect = 12.0f;
            auto width_info_rect = stream_rect.w - x_offset_info_rect;

            curr_info_rect = rect{ stream_rect.x + x_offset_info_rect,
                                   stream_rect.y + y_offset_info_rect,
                                   width_info_rect,
                                   height_info_rect };
            ImGui::SetNextWindowPos({ curr_info_rect.x, curr_info_rect.y });
            ImGui::SetNextWindowSize({ curr_info_rect.w, curr_info_rect.h });
            std::string label = to_string() << "Stream Info of " << profile.unique_id();
            ImGui::Begin(label.c_str(), nullptr, flags);

            std::string res;
            if (profile.as<rs2::video_stream_profile>())
                res = to_string() << size.x << "x" << size.y << ",  ";
            label = to_string() << res << truncate_string(rs2_format_to_string(profile.format()),9) << ", ";
            ImGui::Text("%s", label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s","Stream Resolution, Format");
            }

            ImGui::SameLine();

            label = to_string() << "FPS: " << std::setprecision(2) << std::setw(7) <<  std::fixed << fps.get_fps();
            ImGui::Text("%s", label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", "FPS is calculated based on timestamps and not viewer time");
            }

            ImGui::SameLine();

            label = to_string() << " Frame: " << std::left <<  frame_number;
            ImGui::Text("%s", label.c_str());

            ImGui::SameLine(290);

            label = to_string() << "Time: " << std::left << std::fixed << std::setprecision(1) << timestamp;
            ImGui::Text("%s", label.c_str());
            if (ImGui::IsItemHovered())
            {
                if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(450.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, red);
                    ImGui::TextUnformatted("Timestamp: SystemTime (Hardware Timestamp unavailable.\nPlease refer to frame_metadata.md for mode information)");
                    ImGui::PopStyleColor();
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                else
                {
                    ImGui::SetTooltip("Timestamp: Hardware clock");
                }
            }

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
                ss << ", *p: 0x" << std::hex << static_cast<int>(round(val));
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
                ImGuiWindowFlags_NoFocusOnAppearing |
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

    void stream_model::show_stream_imu(ImFont* font, const rect &stream_rect, const  rs2_vector& axis)
    {
        const auto precision = 3;
        rs2_stream stream_type = profile.stream_type();

        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(9, 11, 13, 100));

        float y_offset = 0;
        if (show_stream_details)
        {
            y_offset += 30;
        }

        ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y+y_offset });
        ImGui::SetNextWindowSize({ stream_rect.w, stream_rect.h- y_offset });
        std::string label = to_string() << "IMU Stream Info of " << profile.unique_id();
        ImGui::Begin(label.c_str(), nullptr, flags);

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

            ImGui::SetCursorPos({ rc.x + 27 + motion.nameExtraSpace, rc.y + 1 });
            std::string label = to_string() << "##" << motion.name.c_str();
            std::string coordinate = to_string() << std::fixed << std::setprecision(precision) << std::showpos << motion.coordinate;
            ImGui::InputText(label.c_str(), (char*)coordinate.c_str(), coordinate.size() + 1, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

            ImGui::SetCursorPos({ rc.x + 80 + motion.nameExtraSpace, rc.y + 4 });
            ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(255, 255, 255, 100, true));
            ImGui::Text("(%s)", motion.units.c_str());

            ImGui::PopStyleColor(3);
            ImGui::SetCursorPos({ rc.x, rc.y + line_h });
        }

        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);
    }

    void stream_model::show_stream_pose(ImFont* font, const rect &stream_rect, const  rs2_pose& pose_frame, rs2_stream stream_type, bool fullScreen, float y_offset)
    {
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(9, 11, 13, 100));


        ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y+ y_offset });
        ImGui::SetNextWindowSize({ stream_rect.w, std::min(180.0f, stream_rect.h) });
        std::string label = to_string() << "Pose Stream Info of " << profile.unique_id();
        ImGui::Begin(label.c_str(), nullptr, flags);
        
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
        std::string unit = this->texture->currentGrid.getUnitName();

        if (this->texture->currentGrid.unit == GRID_STEP_UNIT_FEET)
        {
            velocity.x *= feetTranslator; velocity.y *= feetTranslator; velocity.z *= feetTranslator;
            acceleration.x *= feetTranslator; acceleration.y *= feetTranslator; acceleration.z *= feetTranslator;
            translation.x *= feetTranslator; translation.y *= feetTranslator; translation.z *= feetTranslator;
        }

        std::vector<pose_data> pose_vector = {
            { "Grid Scale",{ this->texture->currentGrid.step, FLT_MAX , FLT_MAX , FLT_MAX }, "", 2, false,  "(" + unit + ")", "Grid scale in " + unit, 50, true, false, true},
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
            std::string label = to_string() << "##" << pose.name.c_str();
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

        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);
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

            try
            {
                if (frame_metadata_to_csv(filename, original_frame))
                    ss << "The frame attributes are saved into " << filename;
                else
                    viewer.not_model.add_notification({ to_string() << "Failed to save frame metadata file " << filename,
                        0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
            catch (std::exception& e)
            {
                viewer.not_model.add_notification({ to_string() << e.what(),
                    0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
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

    device_model::device_model(device& dev, std::string& error_message, viewer_model& viewer)
        : dev(dev),
          syncer(viewer.syncer),
           _update_readonly_options_timer(std::chrono::seconds(6))
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
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    void rs2::viewer_model::popup_if_ui_not_aligned(ImFont* font_14)
    {
        constexpr const char* graphics_updated_driver = "https://downloadcenter.intel.com/download/27266/Graphics-Intel-Graphics-Driver-for-Windows-15-60-?product=80939";

        if (continue_with_ui_not_aligned)
            return;

        std::string error_message = to_string() <<
            "The application has detected possible UI alignment issue,            \n" <<
            "sometimes caused by outdated graphics card drivers.\n" <<
            "For Intel Integrated Graphics driver \n";

        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar;

        ImGui_ScopePushFont(font_14);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

        std::string name = to_string() << "  " << textual_icons::exclamation_triangle << " UI Offset Detected";


        ImGui::OpenPopup(name.c_str());

        if (ImGui::BeginPopupModal(name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            ImGui::Text("%s", error_message.c_str());
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Button, transparent);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

            ImGui::SetCursorPos({ 190, 42 });
            if (ImGui::Button("click here", { 150, 60 }))
            {
                open_url(graphics_updated_driver);
            }

            ImGui::PopStyleColor(5);

            static bool dont_show_again = false;

            if (ImGui::Button(" Ignore & Continue ", ImVec2(150, 0)))
            {
                continue_with_ui_not_aligned = true;
                if (dont_show_again)
                {
                    config_file::instance().set(
                        configurations::viewer::continue_with_ui_not_aligned, 
                        continue_with_ui_not_aligned);
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            ImGui::Checkbox("Don't show this again", &dont_show_again);

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
                if (pp->enabled)
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
        if (render_thread_active.exchange(true) == false)
        {
            viewer.syncer->start();
            render_thread = std::thread([&](){post_processing_filters::render_loop();});
        }
    }

    void post_processing_filters::stop()
    {
        if (render_thread_active.exchange(false) == true)
        {
            viewer.syncer->stop();
            render_thread.join();
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

        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::SetNextWindowPos({ float(x), float(y) });
        ImGui::SetNextWindowSize({ 250.f, 70.f });
        ImGui::Begin("nostreaming_popup", nullptr, flags);

        ImGui::PushFont(font_18);
        ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0x70, 0x8f, 0xa8, 0xff));
        ImGui::Text("Connect a RealSense Camera\nor Add Source");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorPos({ 0, 43 });
        ImGui::PushStyleColor(ImGuiCol_Button, dark_window_background);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dark_window_background);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, dark_window_background);
        ImGui::PushStyleColor(ImGuiCol_Text, button_color + 0.25f);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, button_color + 0.55f);
        
        ImGui::PopStyleColor(5);
        ImGui::End();
        ImGui::PopStyleColor();
    }

    // Generate streams layout, creates a grid-like layout with factor amount of columns
    std::map<int, rect> generate_layout(const rect& r,
        int top_bar_height, size_t factor,
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
            for (size_t f = 1; f <= active_streams.size(); f++)
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
        std::shared_ptr<texture_buffer> texture_frame = nullptr;
        points p;
        frame f{};

        std::map<int, frame> last_frames;
        try
        {
            size_t index = 0;
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
                        if (frame.is<points>() && !paused)  // find and store the 3d points frame for later use
                        {
                            p = frame.as<points>();
                            continue;
                        }

                        if (frame.is<pose_frame>())  // Aggregate the trajectory in pause mode to make the path consistent
                        {
                            auto dev = streams[frame.get_profile().unique_id()].dev;
                            if (dev)
                            {
                                dev->tm2.update_model_trajectory(frame.as<pose_frame>(), !paused);
                            }
                        }

                        auto texture = upload_frame(std::move(frame));

                        if ((selected_tex_source_uid == -1 && frame.get_profile().format() == RS2_FORMAT_Z16) || 
                            frame.get_profile().format() != RS2_FORMAT_ANY && is_3d_texture_source(frame))
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

        if (paused || s_model.dev->_is_being_recorded)
        {
            bottom_y_ruler -= 30;
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
        size_t last_index = 0;
        for (size_t i = 1; i < rgb_per_distance_vec.size(); ++i)
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
            float posX = stream_rect.x + stream_rect.w - 40;
            float posY = stream_rect.y + stream_rect.h - 40;

            if (stream_mv.dev->_is_being_recorded)
            {
                show_recording_icon(font2, static_cast<int>(posX), static_cast<int>(posY), stream_mv.profile.unique_id(), alpha);
                posX -= 23;
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
                show_paused_icon(font2, static_cast<int>(posX), static_cast<int>(posY), stream_mv.profile.unique_id());

            stream_mv.show_stream_header(font1, stream_rect, *this);
            stream_mv.show_stream_footer(font1, stream_rect, mouse);

            auto stream_type = stream_mv.profile.stream_type();

            if (streams[stream].is_stream_visible())
            {
                switch (stream_type)
                {
                    case RS2_STREAM_GYRO: /* Fall Through */
                    case RS2_STREAM_ACCEL:
                    {
                        auto motion = streams[stream].texture->get_last_frame().as<motion_frame>();
                        if (motion.get())
                        {
                            auto axis = motion.get_motion_data();
                            stream_mv.show_stream_imu(font1, stream_rect, axis);
                        }
                        break;
                    }
                    case RS2_STREAM_POSE:
                    {
                        if (streams[stream].show_stream_details)
                        {
                            auto pose = streams[stream].texture->get_last_frame().as<pose_frame>();
                            if (pose.get())
                            {
                                auto pose_data = pose.get_pose_data();
                                stream_mv.show_stream_pose(font1, stream_rect, pose_data, stream_type, (*this).fullscreen, 30.0f);
                            }
                        }
  
                        break;
                    }
                }
            }

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
                if(RS2_FORMAT_RGB8 == textured_frame.get_profile().format())
                {
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
        }

        // Metadata overlay windows shall be drawn after textures to preserve z-buffer functionality
        for (auto &&kvp : layout)
        {
            if (streams[kvp.first].metadata_displayed)
                streams[kvp.first].show_metadata(mouse);
        }
    }

    void draw_3d_grid(pose_grid grid)
    {
        uint32_t boxHorizontalLength = grid.boxHorizontalLength;
        uint32_t boxVerticalLength = grid.boxVerticalLength;
        int32_t boxVerticalAlignment = grid.boxVerticalAlignment;
        float step = grid.getPixelStep();

        uint32_t horizontal = boxHorizontalLength / 2;
        uint32_t vertical = (boxVerticalLength-1) / 2;

        glPushAttrib(GL_CURRENT_BIT);
        glPushAttrib(GL_LINE_BIT);
        glLineWidth(1.f);
        glColor4f(0.4f, 0.4f, 0.4f, 1.f);
        glBegin(GL_LINES);

        // Render the box grid
        float currentYStep = boxVerticalAlignment * step;

        for (uint8_t y = 0; y <= (boxVerticalLength-1); y++)
        {
            float currentXZStep = 0;
            for (uint8_t xz = 0; xz <= boxHorizontalLength; xz++)
            {
                glVertex3f(currentXZStep - (horizontal * step), currentYStep - (vertical * step), (-1.0f) * horizontal * step);
                glVertex3f(currentXZStep - (horizontal * step), currentYStep - (vertical * step), horizontal * step);
                glVertex3f((-1.0f)* horizontal * (step), currentYStep - (vertical * step), ((-1.0f)*horizontal * step) + currentXZStep);
                glVertex3f(horizontal * step, currentYStep - (vertical * step), (((-1.0f) * horizontal) * step) + currentXZStep);
                currentXZStep += step;
            }
            currentYStep += step;
        }

        float currentZStep = 0;
        for (uint8_t z = 0; z <= boxHorizontalLength; z++)
        {
            float currentXStep = 0;
            for (uint8_t x = 0; x <= boxHorizontalLength; x++)
            {
                glVertex3f(currentXStep - (horizontal * step), (boxVerticalAlignment * step) + (-1.0f)*(vertical * step), (((-1.0f) * horizontal) * step) + currentZStep);
                glVertex3f(currentXStep - (horizontal * step), (boxVerticalAlignment * step) + (vertical * step), (((-1.0f) * horizontal) * step) + currentZStep);

                currentXStep += step;
            }
            currentZStep += step;
        }
        glEnd();
        glPopAttrib(); // color
        glPopAttrib(); // line width
    }

     void viewer_model::render_camera_mesh(int id)
    {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glBegin(GL_TRIANGLES);
        auto& mesh = camera_mesh[id];
        for (auto& i : mesh.indexes)
        {
            auto v0 = mesh.positions[i.x];
            auto v1 = mesh.positions[i.y];
            auto v2 = mesh.positions[i.z];
            glVertex3fv(&v0.x);
            glVertex3fv(&v1.x);
            glVertex3fv(&v2.x);
            glColor4f(0.036f, 0.044f, 0.051f, 0.3f);
        }
        glEnd();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
 void viewer_model::render_3d_view(const rect& viewer_rect,
        std::shared_ptr<texture_buffer> texture, rs2::points points, ImFont *font1)
    {
        auto top_bar_height = 32.f;

        if (points)
        {
            last_points = points;
        }
        if (texture)
        {
            last_texture = texture;
        }

        glViewport(static_cast<GLint>(viewer_rect.x), static_cast<GLint>(viewer_rect.y),
            static_cast<GLsizei>(viewer_rect.w), static_cast<GLsizei>(viewer_rect.h));

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        matrix4 perspective_mat = create_perspective_projection_matrix(viewer_rect.w, viewer_rect.h, 60, 0.001f, 100.f);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf((float*)perspective_mat.mat);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(view);

        matrix4 view_mat;
        memcpy(&view_mat, view, sizeof(matrix4));

        glDisable(GL_TEXTURE_2D);

        glEnable(GL_DEPTH_TEST);

        auto r1 = matrix4::identity();
        auto r2 = matrix4::identity();

        if (draw_plane)
        {
            glPushAttrib(GL_LINE_BIT);
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
            glPopAttrib();
        }

        auto x = static_cast<float>(-M_PI / 2);
        float _rx[4][4] = {
            { 1 , 0, 0, 0 },
            { 0, cos(x), -sin(x), 0 },
            { 0, sin(x), cos(x), 0 },
            { 0, 0, 0, 1 }
        };
        auto z = static_cast<float>(M_PI);
        float _rz[4][4] = {
            { cos(z), -sin(z),0, 0 },
            { sin(z), cos(z), 0, 0 },
            { 0 , 0, 1, 0 },
            { 0, 0, 0, 1 }
        };
        rs2::matrix4 rx(_rx);
        rs2::matrix4 rz(_rz);

        int stream_num = 0; // counter to locate the pose info window correctly (currently works for up to 3 streaming devices)
        pose_frame pose = frame{};
        for (auto&& stream : streams)
        {
            if (stream.second.profile.stream_type() == RS2_STREAM_POSE)
            {
                auto f = stream.second.texture->get_last_frame();
                if (!f.is<pose_frame>())
                {
                    continue;
                }

                draw_3d_grid(stream.second.texture->currentGrid);

                pose = f;
                rs2_pose pose_data = pose.get_pose_data();
                if (show_pose_info_3d)
                {
                    auto stream_type = stream.second.profile.stream_type();
                    auto stream_rect = viewer_rect;
                    stream_rect.x += 460 * stream_num;
                    stream_rect.y += 2 * top_bar_height + 5;
                    stream.second.show_stream_pose(font1, stream_rect, pose_data, stream_type, true, 0);
                }

                auto t = tm2_pose_to_world_transformation(pose_data);
                float model[4][4];
                t.to_column_major((float*)model);
                auto m = model;

                r1 = m * rx;
                r2 = rz * m * rx;

                // set the pose transformation as the model matrix to draw the axis
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadMatrixf(view);

                glMultMatrixf((float*)_rx);

                streams[f.get_profile().unique_id()].dev->tm2.draw_trajectory(trajectory_button.is_pressed());

                // remove model matrix from the rest of the render
                glPopMatrix();

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadMatrixf(r2 * view_mat);
                render_camera_mesh(t265_mesh_id);
                glPopMatrix();

                stream_num++;
            }
        }

        if (!pose)
        {
             // Render "floor" grid
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

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadMatrixf(r1 * view_mat);
            texture_buffer::draw_axes(0.4f, 1);
            glPopMatrix();
        }

        glColor4f(1.f, 1.f, 1.f, 1.f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(r1 * view_mat);
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
            glEnable(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadMatrixf(r2 * view_mat);

            auto vertices = last_points.get_vertices();
            auto tex_coords = last_points.get_texture_coordinates();

            glBindTexture(GL_TEXTURE_2D, tex);

            if (render_quads)
            {
                glBegin(GL_QUADS);

                const auto threshold = 0.05f;
                auto width = vf_profile.width();
                auto height = vf_profile.height();
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
            else
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

            glPopMatrix();

            glDisable(GL_TEXTURE_2D);

            if (streams.find(selected_depth_source_uid) != streams.end())
            {
                auto source_frame = streams[selected_depth_source_uid].texture->get_last_frame();

                int index = -1;

                auto dev = streams[selected_depth_source_uid].dev->dev;
                if (dev.supports(RS2_CAMERA_INFO_NAME))
                {
                    auto dev_name = dev.get_info(RS2_CAMERA_INFO_NAME);
                    if (starts_with(dev_name, "Intel RealSense D415"))  index = 0;
                    if (starts_with(dev_name, "Intel RealSense D435") ||
                        starts_with(dev_name, "Intel RealSense USB2"))  index = 1;
                    if (starts_with(dev_name, "Intel RealSense SR300")) index = 2;
                    if (starts_with(dev_name, "Intel RealSense T26"))   index = 3;
                };

                if (source_frame && index >= 0)
                {
                    glMatrixMode(GL_MODELVIEW);
                    glPushMatrix();
                    glLoadMatrixf(r2 * view_mat);
                    render_camera_mesh(index);
                    glPopMatrix();
                }
            }
        }

        glPopMatrix();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();

        glDisable(GL_DEPTH_TEST);

        if (ImGui::IsKeyPressed('R') || ImGui::IsKeyPressed('r'))
        {
            reset_camera();
        }
    }


    void viewer_model::show_top_bar(ux_window& window, const rect& viewer_rect, const std::vector<device_model>& devices)
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

        int buttons = window.is_fullscreen() ? 4 : 3;

        ImGui::SetCursorPosX(window.width() - panel_width - panel_y * (buttons));
        ImGui::PushStyleColor(ImGuiCol_Text, is_3d_view ? light_grey : light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, is_3d_view ? light_grey : light_blue);
        if (ImGui::Button("2D", { panel_y, panel_y })) 
        {
            is_3d_view = false;
            config_file::instance().set(configurations::viewer::is_3d_view, is_3d_view);
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        ImGui::SetCursorPosX(window.width() - panel_width - panel_y * (buttons - 1));
        auto pos1 = ImGui::GetCursorScreenPos();

        ImGui::PushStyleColor(ImGuiCol_Text, !is_3d_view ? light_grey : light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, !is_3d_view ? light_grey : light_blue);
        if (ImGui::Button("3D", { panel_y,panel_y }))
        {
            is_3d_view = true;
            config_file::instance().set(configurations::viewer::is_3d_view, is_3d_view);
            update_3d_camera(window, viewer_rect, true);
        }

        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        ImGui::SetCursorPosX(window.width() - panel_width - panel_y * (buttons - 2));

        static bool settings_open = false;
        ImGui::PushStyleColor(ImGuiCol_Text, !settings_open ? light_grey : light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, !settings_open ? light_grey : light_blue);
        
        if (ImGui::Button(u8"\uf013\uf0d7", { panel_y,panel_y }))
        {
            ImGui::OpenPopup("More Options");
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", "More Options...");
        }

        if (window.is_fullscreen())
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(window.width() - panel_width - panel_y * (buttons - 3));

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_color);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_red);
            if (ImGui::Button(textual_icons::exit, { panel_y,panel_y }))
            {
                exit(0);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Exit the App");
                window.link_hovered();
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::PopFont();

        ImGui::PushStyleColor(ImGuiCol_Text, black);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        ImGui::PushFont(window.get_font());

        auto settings = "Settings";
        auto about = "About";
        bool open_settings_popup = false;
        bool open_about_popup = false;

        ImGui::SetNextWindowPos({ window.width() - 100, panel_y });
        ImGui::SetNextWindowSize({ 100, 90 });

        if (ImGui::BeginPopup("More Options"))
        {
            settings_open = true;

            if (ImGui::Selectable("Report Issue"))
            {
                open_issue(devices);
            }

            if (ImGui::Selectable("Intel Store"))
            {
                open_url(store_url);
            }

            if (ImGui::Selectable(settings))
            {
                open_settings_popup = true;
            }
            
            ImGui::Separator();

            if (ImGui::Selectable(about))
            {
                open_about_popup = true;
            }

            ImGui::EndPopup();
        }
        else
        {
            settings_open = false;
        }

        static config_file temp_cfg;
        static bool reload_required = false;
        static bool refresh_required = false;

        static int tab = 0;

        if (open_settings_popup) 
        {
            temp_cfg = config_file::instance();
            ImGui::OpenPopup(settings);   
            reload_required = false;    
            refresh_required = false;
            tab = config_file::instance().get_or_default(configurations::viewer::settings_tab, 0);             
        }

        {
            float w  = window.width()  * 0.6f;
            float h  = window.height() * 0.6f;
            float x0 = window.width()  * 0.2f;
            float y0 = window.height() * 0.2f;
            ImGui::SetNextWindowPos({ x0, y0 });
            ImGui::SetNextWindowSize({ w, h });

            flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

            ImGui_ScopePushFont(window.get_font());
            ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

            if (ImGui::BeginPopupModal(settings, nullptr, flags))
            {
                ImGui::SetCursorScreenPos({ (float)(x0 + w / 2 - 220), (float)(y0 + 27) });
                ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                ImGui::PushFont(window.get_large_font());

                ImGui::PushStyleColor(ImGuiCol_Text, tab != 0 ? light_grey : light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, tab != 0 ? light_grey : light_blue);
                if (ImGui::Button("Playback & Record", { 170, 30})) 
                {
                    tab = 0;
                    config_file::instance().set(configurations::viewer::settings_tab, tab);
                    temp_cfg.set(configurations::viewer::settings_tab, tab);
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, tab != 1 ? light_grey : light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, tab != 1 ? light_grey : light_blue);
                if (ImGui::Button("Performance", { 150, 30})) 
                {
                    tab = 1;
                    config_file::instance().set(configurations::viewer::settings_tab, tab);
                    temp_cfg.set(configurations::viewer::settings_tab, tab);
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, tab != 2 ? light_grey : light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, tab != 2 ? light_grey : light_blue);
                if (ImGui::Button("General", { 100, 30})) 
                {
                    tab = 2;
                    config_file::instance().set(configurations::viewer::settings_tab, tab);
                    temp_cfg.set(configurations::viewer::settings_tab, tab);
                }
                ImGui::PopStyleColor(2);
                //ImGui::SameLine();

                ImGui::PopFont();
                ImGui::PopStyleColor(2); // button color

                ImGui::SetCursorScreenPos({ (float)(x0 + 15), (float)(y0 + 65) });
                ImGui::Separator();

                if (tab == 0)
                {
                    int recording_setting = temp_cfg.get(configurations::record::file_save_mode);
                    ImGui::Text("When starting a new recording:");
                    if (ImGui::RadioButton("Select filename automatically", recording_setting == 0))
                    {
                        recording_setting = 0;
                        temp_cfg.set(configurations::record::file_save_mode, recording_setting);
                    }
                    if (ImGui::RadioButton("Ask me every time", recording_setting == 1))
                    {
                        recording_setting = 1;
                        temp_cfg.set(configurations::record::file_save_mode, recording_setting);
                    }
                    ImGui::Text("Default recording folder: ");
                    ImGui::SameLine();
                    static char path[256];
                    memset(path, 0, 256);
                    std::string path_str = temp_cfg.get(configurations::record::default_path);
                    memcpy(path, path_str.c_str(), std::min(255, (int)path_str.size()));

                    if (ImGui::InputText("##default_record_path", path, 255))
                    {
                        path_str = path;
                        temp_cfg.set(configurations::record::default_path, path_str);
                    }

                    ImGui::Separator();

                    ImGui::Text("ROS-bag Compression:");
                    int recording_compression = temp_cfg.get(configurations::record::compression_mode);
                    if (ImGui::RadioButton("Always Compress (might cause frame drops)", recording_compression == 0))
                    {
                        recording_compression = 0;
                        temp_cfg.set(configurations::record::compression_mode, recording_compression);
                    }
                    if (ImGui::RadioButton("Never Compress (larger .bag file size)", recording_compression == 1))
                    {
                        recording_compression = 1;
                        temp_cfg.set(configurations::record::compression_mode, recording_compression);
                    }
                    if (ImGui::RadioButton("Use device defaults", recording_compression == 2))
                    {
                        recording_compression = 2;
                        temp_cfg.set(configurations::record::compression_mode, recording_compression);
                    }
                }

                if (tab == 1)
                {
                    int font_samples = temp_cfg.get(configurations::performance::font_oversample);
                    ImGui::Text("Font Samples: "); 
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Increased font samples produce nicer text, but require more GPU memory, sometimes resulting in boxes instead of font characters");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(80);
                    if (ImGui::SliderInt("##font_samples", &font_samples, 1, 8))
                    {
                        reload_required = true;
                        temp_cfg.set(configurations::performance::font_oversample, font_samples);
                    }
                    ImGui::PopItemWidth();

                    bool msaa = temp_cfg.get(configurations::performance::enable_msaa);
                    if (ImGui::Checkbox("Enable Multisample Anti-Aliasing (MSAA)", &msaa))
                    {
                        reload_required = true;
                        temp_cfg.set(configurations::performance::enable_msaa, msaa);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("MSAA will improve the rendering quality of edges at expense of greater GPU memory utilisation.");

                    if (msaa)
                    {
                        int samples = temp_cfg.get(configurations::performance::msaa_samples);
                        ImGui::Text("MSAA Samples: "); ImGui::SameLine();
                        ImGui::PushItemWidth(160);
                        if (ImGui::SliderInt("##samples", &samples, 2, 16))
                        {
                            reload_required = true;
                            temp_cfg.set(configurations::performance::msaa_samples, samples);
                        }
                        ImGui::PopItemWidth();
                    }

                    bool show_fps = temp_cfg.get(configurations::performance::show_fps);
                    if (ImGui::Checkbox("Show Application FPS (rendering FPS)", &show_fps))
                    {
                        reload_required = true;
                        temp_cfg.set(configurations::performance::show_fps, show_fps);
                    }

                    bool vsync = temp_cfg.get(configurations::performance::vsync);
                    if (ImGui::Checkbox("Enable VSync", &vsync))
                    {
                        reload_required = true;
                        temp_cfg.set(configurations::performance::vsync, vsync);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Vertical sync will try to synchronize application framerate to the monitor refresh-rate (usually limiting the framerate to 60)");

                    bool fullscreen = temp_cfg.get(configurations::window::is_fullscreen);
                    if (ImGui::Checkbox("Fullscreen (F8)", &fullscreen))
                    {
                        reload_required = true;
                        temp_cfg.set(configurations::window::is_fullscreen, fullscreen);
                    }
                }

                if (tab == 2)
                {
                    ImGui::Text("librealsense has built-in logging capabilities.");
                    ImGui::Text("Logs may contain API calls, timing of frames, OS error messages and file-system links, but no actual frame content.");

                    bool log_to_console = temp_cfg.get(configurations::viewer::log_to_console);
                    if (ImGui::Checkbox("Output librealsense log to console", &log_to_console))
                    {
                        temp_cfg.set(configurations::viewer::log_to_console, log_to_console);
                    }
                    bool log_to_file = temp_cfg.get(configurations::viewer::log_to_file);
                    if (ImGui::Checkbox("Output librealsense log to file", &log_to_file))
                    {
                        temp_cfg.set(configurations::viewer::log_to_file, log_to_file);
                    }
                    if (log_to_file)
                    {
                        ImGui::Text("Log file name:");
                        ImGui::SameLine();
                        static char logpath[256];
                        memset(logpath, 0, 256);
                        std::string path_str = temp_cfg.get(configurations::viewer::log_filename);
                        memcpy(logpath, path_str.c_str(), std::min(255, (int)path_str.size()));

                        if (ImGui::InputText("##default_log_path", logpath, 255))
                        {
                            path_str = logpath;
                            temp_cfg.set(configurations::viewer::log_filename, path_str);
                        }
                    }
                    if (log_to_console || log_to_file)
                    {
                        int new_severity = temp_cfg.get(configurations::viewer::log_severity);

                        std::vector<std::string> severities;
                        for (int i = 0; i < RS2_LOG_SEVERITY_COUNT; i++)
                            severities.push_back(rs2_log_severity_to_string((rs2_log_severity)i));

                        ImGui::Text("Minimal log severity:");
                        ImGui::SameLine();

                        ImGui::PushItemWidth(150);
                        if (draw_combo_box("##log_severity", severities, new_severity))
                        {
                            temp_cfg.set(configurations::viewer::log_severity, new_severity);
                        }
                        ImGui::PopItemWidth();
                    }


                    ImGui::Separator();

                    ImGui::Text("RealSense tools settings capture the state of UI, and not of the hardware:");

                    if (ImGui::Button(" Restore Defaults "))
                    {
                        reload_required = true;
                        temp_cfg = config_file();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(" Export Settings "))
                    {
                        auto ret = file_dialog_open(save_file, "JavaScript Object Notation (JSON)\0*.json\0", NULL, NULL);
                        if (ret)
                        {
                            try
                            {
                                std::string filename = ret;
                                filename = to_lower(filename);
                                if (!ends_with(filename, ".json")) filename += ".json";
                                temp_cfg.save(filename.c_str());
                            }
                            catch (...){}
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(" Import Settings "))
                    {
                        auto ret = file_dialog_open(open_file, "JavaScript Object Notation (JSON)\0*.json\0", NULL, NULL);
                        if (ret)
                        {
                            try
                            {
                                config_file file(ret);
                                temp_cfg = file;
                                reload_required = true;
                            }
                            catch (...){}
                        }
                    }
                }

                ImGui::Separator();

                ImGui::GetWindowDrawList()->AddRectFilled({ (float)x0, (float)(y0 + h - 60) },
                    { (float)(x0 + w), (float)(y0 + h) }, ImColor(sensor_bg));

                ImGui::SetCursorScreenPos({ (float)(x0 + 15), (float)(y0 + h - 60) });
                if (reload_required)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                    ImGui::Text(u8"\uf071 The application will be restarted in order for new settings to take effect");
                    ImGui::PopStyleColor();
                }

                auto apply = [&](){
                    config_file::instance() = temp_cfg;
                    if (reload_required) window.reload();
                    else if (refresh_required) window.refresh();
                    update_configuration();
                };
        
                ImGui::SetCursorScreenPos({ (float)(x0 + w / 2 - 190), (float)(y0 + h - 30) });
                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                    apply();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Save settings and close");
                }
                ImGui::SameLine();

                auto configs_same = temp_cfg == config_file::instance();
                ImGui::PushStyleColor(ImGuiCol_Text, configs_same ? light_grey : light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, configs_same ? light_grey : light_blue);
                if (ImGui::Button("Apply", ImVec2(120, 0)))
                {
                    apply();
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Save settings");
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Close window without saving any changes to the settings");
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
        }

        if (open_about_popup) 
        {
            ImGui::OpenPopup(about);                    
        }

        {
            float w = 590.f;
            float h = 300.f;
            float x0 = (window.width() - w) / 2.f;
            float y0 = (window.height() - h) / 2.f;
            ImGui::SetNextWindowPos({ x0, y0 });
            ImGui::SetNextWindowSize({ w, h });

            flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

            ImGui_ScopePushFont(window.get_font());
            ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1);

            if (ImGui::BeginPopupModal(about, nullptr, flags))
            {
                ImGui::Image((void*)(intptr_t)window.get_splash().get_gl_handle(), 
                             ImVec2(w - 30, 100), {0.20f, 0.38f}, {0.80f, 0.56f});

                auto realsense_pos = ImGui::GetCursorPos();
                ImGui::Text("Intel RealSense is a suite of depth-sensing and motion-tracking technologies.");

                ImGui::Text("librealsense is an open-source cross-platform SDK for working with RealSense devices.");

                ImGui::Text("Full source code is available at"); ImGui::SameLine();
                auto github_pos = ImGui::GetCursorPos();
                ImGui::Text("github.com/IntelRealSense/librealsense.");
                
                ImGui::Text("This software is distributed under the"); ImGui::SameLine();
                auto license_pos = ImGui::GetCursorPos();
                ImGui::Text("Apache License, Version 2.0.");

                ImGui::Text("RealSense is a registered trademark of Intel Corporation.");
                
                ImGui::Text("Copyright 2018 Intel Corporation.");

                ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_Text, light_blue);

                ImGui::SetCursorPos({ realsense_pos.x - 4, realsense_pos.y - 3 });

                hyperlink(window, "Intel RealSense", "https://realsense.intel.com/");

                ImGui::SetCursorPos({ github_pos.x - 4, github_pos.y - 3 });
                hyperlink(window, "github.com/IntelRealSense/librealsense", "https://github.com/IntelRealSense/librealsense/");

                ImGui::SetCursorPos({ license_pos.x - 4, license_pos.y - 3 });

                hyperlink(window, "Apache License, Version 2.0", "https://raw.githubusercontent.com/IntelRealSense/librealsense/master/LICENSE");

                ImGui::PopStyleColor(4);

        
                ImGui::SetCursorScreenPos({ (float)(x0 + w / 2 - 60), (float)(y0 + h - 30) });
                if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(7);

        ImGui::GetWindowDrawList()->AddLine({ pos1.x, pos1.y + 10 }, { pos1.x,pos1.y + panel_y - 10 }, ImColor(light_grey));


        ImGui::SameLine();
        ImGui::SetCursorPosX(window.width() - panel_width - panel_y);

        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void viewer_model::update_3d_camera(
        ux_window& win,
        const rect& viewer_rect, bool force)
    {
        mouse_info& mouse = win.get_mouse();
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
            // Whenever the mouse reaches the end of the window
            // and jump back to the start, it will add to the overflow
            // counter, so by adding the overflow value
            // we essentially emulate an infinite display
            auto cx = mouse.cursor.x + overflow.x;
            auto px = mouse.prev_cursor.x + overflow.x;
            auto cy = mouse.cursor.y + overflow.y;
            auto py = mouse.prev_cursor.y + overflow.y;

            // Limit how much user mouse can jump between frames
            // This can work poorly when the app FPS is really terrible (< 10)
            // but overall works resonably well
            const auto MAX_MOUSE_JUMP = 200;
            if (std::abs(cx - px) < MAX_MOUSE_JUMP && 
                std::abs(cy - py) < MAX_MOUSE_JUMP )
                arcball_camera_update(
                (float*)&pos, (float*)&target, (float*)&up, view,
                sec_since_update,
                0.2f, // zoom per tick
                -0.1f, // pan speed
                3.0f, // rotation multiplier
                static_cast<int>(viewer_rect.w), static_cast<int>(viewer_rect.h), // screen (window) size
                static_cast<int>(px), static_cast<int>(cx),
                static_cast<int>(py), static_cast<int>(cy),
                (ImGui::GetIO().MouseDown[2] || ImGui::GetIO().MouseDown[1]) ? 1 : 0,
                ImGui::GetIO().MouseDown[0] ? 1 : 0,
                mouse.mouse_wheel,
                0);

            // If we are pressing mouse button
            // inside the 3D viewport
            // we should remember that we
            // are in the middle of manipulation
            // and adjust when mouse leaves the area
            if (ImGui::GetIO().MouseDown[0] || 
                ImGui::GetIO().MouseDown[1] || 
                ImGui::GetIO().MouseDown[2])
            {
                manipulating = true;
            }
        }

        auto rect = viewer_rect;
        rect.w -= 10;

        // If we started manipulating the camera
        // and left the viewport
        if (manipulating && !rect.contains(mouse.cursor))
        {
            // If mouse is no longer pressed,
            // abort the manipulation
            if (!ImGui::GetIO().MouseDown[0] && 
                !ImGui::GetIO().MouseDown[1] && 
                !ImGui::GetIO().MouseDown[2])
            {
                manipulating = false;
                overflow = float2{ 0.f, 0.f };
            }
            else
            {
                // Wrap-around the mouse in X direction
                auto startx = (mouse.cursor.x - rect.x);
                if (startx < 0) 
                {
                    overflow.x -= rect.w;
                    startx += rect.w;
                }
                if (startx > rect.w) 
                {
                    overflow.x += rect.w;
                    startx -= rect.w;
                }
                startx += rect.x;

                // Wrap-around the mouse in Y direction
                auto starty = (mouse.cursor.y - rect.y);
                if (starty < 0) 
                {
                    overflow.y -= rect.h;
                    starty += rect.h;
                }
                if (starty > rect.h)
                {
                    overflow.y += rect.h;
                    starty -= rect.h;
                }
                starty += rect.y;

                // Set new cursor position
                glfwSetCursorPos(win, startx, starty);
            }
        }
        else overflow = float2{ 0.f, 0.f };

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
        auto profile = f.get_profile().as<video_stream_profile>();
        auto index = profile.unique_id();
        auto mapped_index = streams_origin[index];

        if (!is_rasterizeable(profile.format()))
            return false;

        if (index == selected_tex_source_uid || mapped_index == selected_tex_source_uid || selected_tex_source_uid == -1)
            return true;
        return false;
    }

    std::vector<frame> rs2::viewer_model::get_frames(frame frame)
    {
        std::vector<rs2::frame> res;

        if (auto set = frame.as<frameset>())
            for (auto&& f : set)
                res.push_back(f);

        else
            res.push_back(frame);

        return res;
    }

    frame viewer_model::get_3d_depth_source(frame f)
    {
        auto frames = get_frames(f);

        for (auto&& f : frames)
        {
            if (is_3d_depth_source(f))
                return f;
        }
        return nullptr;
    }

    frame rs2::viewer_model::get_3d_texture_source(frame f)
    {
        auto frames = get_frames(f);

        for (auto&& f : frames)
        {
            if (is_3d_texture_source(f))
                return f;
        }
        return nullptr;
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

    std::shared_ptr<texture_buffer> viewer_model::upload_frame(frame&& f)
    {
        if (f.get_profile().stream_type() == RS2_STREAM_DEPTH)
            ppf.depth_stream_active = true;

        auto index = f.get_profile().unique_id();

        std::lock_guard<std::mutex> lock(streams_mutex);
        if (streams.find(streams_origin[index]) != streams.end())
            return streams[streams_origin[index]].upload_frame(std::move(f));
        else return nullptr;
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

        std::string label = to_string() << textual_icons::step_backward << "##Step Backwards " << id;

        if (pause_required) 
        {
            p.pause();
            for (auto&& s : subdevices)
            {
                if (s->streaming)
                    s->pause();
                s->on_frame = []{};
            }
            syncer->on_frame = []{};
            pause_required = false;
        }

        // TODO: Figure out how to properly step-back
        ImGui::ButtonEx(label.c_str(), button_dim, ImGuiButtonFlags_Disabled);

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
            pause_required = false;
            auto action = [this]() {
                    pause_required = true;
                };
            for (auto& s : subdevices)
            {
                s->on_frame = action;
            }
            syncer->on_frame = action;

            p.resume();
            for (auto&& s : subdevices)
            {
                if (s->streaming)
                    s->resume();
            }
            viewer.paused = false;
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
                        if (ImGui::Selectable("Export Localization map", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                        {
                            if (auto target_path = file_dialog_open(save_file, "Tracking device Localization map (RAW)\0*.*\0", NULL, NULL))
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
                            if (is_streaming)
                                ImGui::SetTooltip("Stop streaming to Export localization map");
                            else
                                ImGui::SetTooltip("Retrieve the localization map from device");
                        }

                        if (ImGui::Selectable("Import Localization map", false, is_streaming ? ImGuiSelectableFlags_Disabled : 0))
                        {
                            if (auto source_path = file_dialog_open(save_file, "Tracking device Localization map (RAW)\0*.*\0", NULL, NULL))
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
                            auto advanced = dev.as<advanced_mode>();
                            if (advanced)
                            {
                                if (advanced.is_enabled())
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
        auto header_h = panel_height;
        if (is_playback_device) header_h += 15;

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
        ss << dev.get_info(RS2_CAMERA_INFO_NAME);
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
            auto playback_panel_height = draw_playback_panel(window, window.get_font(), viewer);
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

            ImGui::SetCursorPos({ rc.x + 225, rc.y - 107 });

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
                                    if(!dev_syncer)
                                        dev_syncer = viewer.syncer->create_syncer();

                                    std::string friendly_name = sub->s->get_info(RS2_CAMERA_INFO_NAME);
                                    if ((friendly_name.find("Tracking") != std::string::npos) ||
                                        (friendly_name.find("Motion") != std::string::npos))
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
                            if (skip_option(opt)) continue;
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
                        sub->_options_invalidated = true;
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
                            if (skip_option(opt)) continue;
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
                                    if (sub->zero_order_artifact_fix && sub->zero_order_artifact_fix->enabled)
                                        sub->verify_zero_order_conditions();
                                    sub->post_processing_enabled = true;
                                    config_file::instance().set(get_device_sensor_name(sub.get()).c_str(),
                                        sub->post_processing_enabled);
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
                                        if (!pb->enabled)
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
                                        if (!pb->enabled)
                                        {
                                            std::string label = to_string() << " " << textual_icons::toggle_off << "##" << id << "," << sub->s->get_info(RS2_CAMERA_INFO_NAME) << "," << pb->get_name();

                                            ImGui::PushStyleColor(ImGuiCol_Text, redish);
                                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                                            if (ImGui::Button(label.c_str(), { 25,24 }))
                                            {
                                                if (pb->get_block()->is<zero_order_invalidation>())
                                                    sub->verify_zero_order_conditions();
                                                pb->enabled = true;
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
                                                    pb->enabled = false;
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

    void viewer_model::draw_viewport(const rect& viewer_rect, 
        ux_window& window, int devices, std::string& error_message, 
        std::shared_ptr<texture_buffer> texture, points points)
    {
        static bool first = true;
        if (first)
        {
            update_3d_camera(window, viewer_rect, true);
            first = false;
        }

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

            update_3d_camera(window, viewer_rect);

            rect window_size{ 0, 0, (float)window.width(), (float)window.height() };
            rect fb_size{ 0, 0, (float)window.framebuf_width(), (float)window.framebuf_height() };
            rect new_rect = viewer_rect.normalize(window_size).unnormalize(fb_size);

            render_3d_view(new_rect, texture, points, window.get_font());
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
                        if (auto p = s.second.dev->dev.as<playback>())
                        {
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

            if (pending_notifications.size() > (size_t)MAX_SIZE)
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
    void tm2_model::draw_controller_pose_object()
    {
        const float sphere_radius = 0.02f;
        const float controller_height = 0.2f;
        //TODO: Draw controller holder as cylinder
        texture_buffer::draw_circle(1, 0, 0, 0, 1, 0, sphere_radius, { 0.0, controller_height + sphere_radius, 0.0 }, 1.0f);
        texture_buffer::draw_circle(0, 1, 0, 0, 0, 1, sphere_radius, { 0.0, controller_height + sphere_radius, 0.0 }, 1.0f);
        texture_buffer::draw_circle(1, 0, 0, 0, 0, 1, sphere_radius, { 0.0, controller_height + sphere_radius, 0.0 }, 1.0f);
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
