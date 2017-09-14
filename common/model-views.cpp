// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "model-views.h"

#include <librealsense2/rs_advanced_mode.hpp>

#include <regex>
#include <thread>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#include <noc_file_dialog.h>

#define ARCBALL_CAMERA_IMPLEMENTATION
#include <arcball_camera.h>


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



namespace rs2
{
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

    void export_to_ply(notifications_model& ns, points points, video_frame texture)
    {
        const char *ret;
        ret = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "Polygon File Format (PLY)\0*.ply\0", NULL, NULL);
        if (!ret) return; // user cancelled dialog. Don't save image.
        std::string fname(ret);
        if (!ends_with(to_lower(fname), ".ply")) fname += ".ply";

        std::thread([&ns, points, texture, fname]() {
            std::string texfname(fname);
            texfname += ".png";

            const auto vertices = points.get_vertices();
            const auto texcoords = points.get_texture_coordinates();
            const auto tex = reinterpret_cast<const uint8_t*>(texture.get_data());
            std::vector<vertex> new_vertices;
            //std::vector<texture_coordinate> new_texcoords;
            std::vector<std::tuple<uint8_t,uint8_t,uint8_t>> new_tex;
            new_vertices.reserve(points.size());
            //new_texcoords.reserve(points.size());
            new_tex.reserve(points.size());

            for (int i = 0; i < points.size(); ++i)
                if (std::abs(vertices[i].x) >= 1e-6 || std::abs(vertices[i].y) >= 1e-6 || std::abs(vertices[i].z) >= 1e-6)
                {
                    new_vertices.push_back(vertices[i]);
                    if (texture)
                    {
                        //new_texcoords.push_back(texcoords[i]);
                        auto color = get_texcolor(texture, texcoords[i]);
                        new_tex.push_back(color);
                    }

                }

            std::ofstream out(fname);
            out << "ply\n";
            out << "format binary_little_endian 1.0\n" /*"format ascii 1.0\n"*/;
            out << "comment pointcloud saved from Realsense Viewer\n";
            //if (texture) out << "comment TextureFile " << get_file_name(texfname) << "\n";
            out << "element vertex " << new_vertices.size() << "\n";
            out << "property float" << sizeof(float) * 8 << " x\n";
            out << "property float" << sizeof(float) * 8 << " y\n";
            out << "property float" << sizeof(float) * 8 << " z\n";
            if (texture)
            {
                //out << "property float" << sizeof(float) * 8 << " u\n";
                //out << "property float" << sizeof(float) * 8 << " v\n";
                out << "property uchar red\n";
                out << "property uchar green\n";
                out << "property uchar blue\n";
            }
            out << "end_header\n";
            out.close();

            out.open(fname, std::ios_base::app | std::ios_base::binary);
            for (int i = 0; i < new_vertices.size(); ++i)
            {
                // we assume little endian architecture on your device
                out.write(reinterpret_cast<const char*>(&(new_vertices[i].x)), sizeof(float));
                out.write(reinterpret_cast<const char*>(&(new_vertices[i].y)), sizeof(float));
                out.write(reinterpret_cast<const char*>(&(new_vertices[i].z)), sizeof(float));
//                out << new_vertices[i].x << ' ' << new_vertices[i].y << ' ' << new_vertices[i].z;
                if (texture)
                {
                    //out.write(reinterpret_cast<const char*>(&(new_texcoords[i].u)), sizeof(float));
                    //out.write(reinterpret_cast<const char*>(&(new_texcoords[i].v)), sizeof(float));
                    out.write(reinterpret_cast<const char*>(&(std::get<0>(new_tex[i]))), sizeof(uint8_t));
                    out.write(reinterpret_cast<const char*>(&(std::get<1>(new_tex[i]))), sizeof(uint8_t));
                    out.write(reinterpret_cast<const char*>(&(std::get<2>(new_tex[i]))), sizeof(uint8_t));
//                    out << std::hex << ' ' << std::get<0>(new_tex[i]) << ' ' << std::get<1>(new_tex[i]) << ' ' << std::get<2>(new_tex[i]);
                }
//                out << '\n';
            }

            /* save texture to texfname */
            //if (texture) stbi_write_png(texfname.data(), texture.get_width(), texture.get_height(), texture.get_bytes_per_pixel(), texture.get_data(), texture.get_width() * texture.get_bytes_per_pixel());

            ns.add_notification({ to_string() << "Finished saving 3D view " << (texture ? "to " : "without texture to ") << fname,
                std::chrono::duration_cast<std::chrono::duration<double,std::micro>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
                RS2_LOG_SEVERITY_INFO,
                RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
        }).detach();
    }


    std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec)
    {
        std::vector<const char*> res;
        for (auto&& s : vec) res.push_back(s.c_str());
        return res;
    }

    bool option_model::draw(std::string& error_message)
    {
        auto res = false;
        if (supported)
        {
            auto desc = endpoint.get_option_description(opt);

            if (is_checkbox())
            {
                auto bool_value = value > 0.0f;
                if (ImGui::Checkbox(label.c_str(), &bool_value))
                {
                    res = true;
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
                            ImGui::ProgressBar(value / 100.f, vec, std::to_string((int)value).c_str());
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
                                endpoint.set_option(opt, value);
                                *invalidate_flag = true;
                                res = true;
                            }
                        }
                        else
                        {
                            if (ImGui::SliderFloat(id.c_str(), &value,
                                range.min, range.max, "%.4f"))
                            {
                                endpoint.set_option(opt, value);
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
                        if (abs(i - value) < 0.001f) selected = counter;
                        labels.push_back(endpoint.get_option_value_description(opt, i));
                    }
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });

                    try
                    {
                        if (ImGui::Combo(id.c_str(), &selected, labels.data(),
                            static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * selected;
                            endpoint.set_option(opt, value);
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
                    button_label = label.substr(index+1);
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
            supported =  endpoint.supports(opt);
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
            read_only = endpoint.is_option_read_only(opt);
        }
        catch (const error& e)
        {
            error_message = error_to_string(e);
        }
    }

    void option_model::update_all_feilds(std::string& error_message, notifications_model& model)
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
            if (read_only){
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
            if (endpoint.get_option_value_description(opt, i) == nullptr)
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

    void viewer_model::draw_histogram_options(float depth_units, const subdevice_model& sensor)
    {
        /*std::vector<int> depth_streams_ids;
        for (auto&& s : streams)
        {
            if (s.second.dev.get() == &sensor && s.second.profile.stream_type() == RS2_STREAM_DEPTH)
            {
                depth_streams_ids.push_back(s.second.profile.unique_id());
            }
        }

        if (!depth_streams_ids.size()) return;

        auto&& first_depth_stream = streams[depth_streams_ids.front()];
        bool equalize = first_depth_stream.texture->equalize;

        if (ImGui::Checkbox("Histogram Equalization", &equalize))
        {
            for (auto id : depth_streams_ids)
            {
                streams[id].texture->equalize = equalize;
                streams[id].texture->min_depth = 0;
                streams[id].texture->max_depth = 6 / depth_units;
            }
        }

        if (!equalize)
        {
            auto val = first_depth_stream.texture->min_depth * depth_units;
            if (ImGui::SliderFloat("##Near (m)", &val, 0, 16))
            {
                for (auto id : depth_streams_ids)
                {
                    streams[id].texture->min_depth = val / depth_units;
                }
            }
            val = first_depth_stream.texture->max_depth * depth_units;
            if (ImGui::SliderFloat("##Far  (m)", &val, 0, 16))
            {
                for (auto id : depth_streams_ids)
                {
                    streams[id].texture->max_depth = val / depth_units;
                }
            }
            for (auto id : depth_streams_ids)
            {
                if (streams[id].texture->min_depth > streams[id].texture->max_depth)
                {
                    std::swap(streams[id].texture->max_depth, streams[id].texture->min_depth);
                }
            }
        }*/
    }

    subdevice_model::subdevice_model(device& dev, sensor& s, std::string& error_message)
        : s(s), dev(dev), streaming(false),
          selected_shared_fps_id(0), _pause(false)
    {
        try
        {
            if (s.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
                auto_exposure_enabled = s.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) > 0;
        }
        catch(...)
        {

        }

        try
        {
            if (s.supports(RS2_OPTION_DEPTH_UNITS))
                depth_units = s.get_option(RS2_OPTION_DEPTH_UNITS);
        }
        catch(...)
        {

        }

        // For Realtec sensors
        rgb_rotation_btn = (val_in_range(std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID)),
                            { std::string("0AD3") ,std::string("0B07") }) &&
                            val_in_range(std::string(s.get_info(RS2_CAMERA_INFO_NAME)), { std::string("RGB Camera") }));

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
            metadata.endpoint = s;
            metadata.label = rs2_option_to_string(opt) + std::string("##") + ss.str();
            metadata.invalidate_flag = &options_invalidated;
            metadata.dev = this;

            metadata.supported = s.supports(opt);
            if (metadata.supported)
            {
                try
                {
                    metadata.range = s.get_option_range(opt);
                    metadata.read_only = s.is_option_read_only(opt);
                    if (!metadata.read_only)
                        metadata.value = s.get_option(opt);
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
            auto uvc_profiles = s.get_stream_profiles();
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
                for (auto format : { RS2_FORMAT_RGB8,
                                     RS2_FORMAT_Z16,
                                     RS2_FORMAT_Y8,
                                     RS2_FORMAT_MOTION_XYZ32F } )
                {
                    if (get_default_selection_index(format_array.second, format, &selection_index))
                    {
                        selected_format_id[format_array.first] = selection_index;
                        break;
                    }
                }
            }

            get_default_selection_index(res_values, std::make_pair(0, 0), &selection_index);
            selected_res_id = selection_index;

            while (selected_res_id >= 0 && !is_selected_combination_supported()) selected_res_id--;
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

    void subdevice_model::draw_stream_selection()
    {
        std::string label = to_string() << "Stream Selection Columns##" << dev.get_info(RS2_CAMERA_INFO_NAME)
            << s.get_info(RS2_CAMERA_INFO_NAME);

        auto streaming_tooltip = [&](){
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
            << s.get_info(RS2_CAMERA_INFO_NAME) << " resolution";
        if (streaming)
        {
            ImGui::Text("%s", res_chars[selected_res_id]);
            streaming_tooltip();
        }
        else
        {
            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
            ImGui::Combo(label.c_str(), &selected_res_id, res_chars.data(),
                static_cast<int>(res_chars.size()));
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();
        }
        ImGui::SetCursorPosX(col0);

        // FPS
        if (show_single_fps_list)
        {
            auto fps_chars = get_string_pointers(shared_fpses);
            ImGui::Text("Frame Rate (FPS):");
            streaming_tooltip();
            ImGui::SameLine(); ImGui::SetCursorPosX(col1);

            label = to_string() << "##" << dev.get_info(RS2_CAMERA_INFO_NAME)
                << s.get_info(RS2_CAMERA_INFO_NAME) << " fps";

            if (streaming)
            {
                ImGui::Text("%s", fps_chars[selected_shared_fps_id]);
                streaming_tooltip();
            }
            else
            {
                ImGui::PushItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
                ImGui::Combo(label.c_str(), &selected_shared_fps_id, fps_chars.data(),
                    static_cast<int>(fps_chars.size()));
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();
            }

            ImGui::SetCursorPosX(col0);
        }

        if (!streaming)
        {
            ImGui::Text("Available Streams:");
//            ImGui::NextColumn();
//            ImGui::NextColumn();
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
                    << s.get_info(RS2_CAMERA_INFO_NAME)
                    << " " << f.first << " format";

                if (!show_single_fps_list)
                {
                    ImGui::Text("Format:");
                    streaming_tooltip();
                    ImGui::SameLine(); ImGui::SetCursorPosX(col1);
                }

                if (streaming)
                {
                    ImGui::Text("%s", formats_chars[selected_format_id[f.first]]);
                    streaming_tooltip();
                }
                else
                {
                    ImGui::PushItemWidth(-1);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
                    ImGui::Combo(label.c_str(), &selected_format_id[f.first], formats_chars.data(),
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

                    label = to_string() << s.get_info(RS2_CAMERA_INFO_NAME)
                        << s.get_info(RS2_CAMERA_INFO_NAME)
                        << f.first << " fps";

                    if (streaming)
                    {
                        ImGui::Text("%s", fps_chars[selected_fps_id[f.first]]);
                        streaming_tooltip();
                    }
                    else
                    {
                        ImGui::PushItemWidth(-1);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });
                        ImGui::Combo(label.c_str(), &selected_fps_id[f.first], fps_chars.data(),
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

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    }

    bool subdevice_model::is_selected_combination_supported()
    {
        std::vector<stream_profile> results;

        for (auto&& f : formats)
        {
            auto stream = f.first;
            if (stream_enabled[stream] && res_values.size() > 0)
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
                auto width = res_values[selected_res_id].first;
                auto height = res_values[selected_res_id].second;
                auto format = format_values[stream][selected_format_id[stream]];

                auto fps = 0;
                if (show_single_fps_list)
                    fps = shared_fps_values[selected_shared_fps_id];
                else
                    fps = fps_values_per_stream[stream][selected_fps_id[stream]];



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

    void subdevice_model::stop()
    {
        streaming = false;
        _pause = false;

        s.stop();

        queues.foreach([&](frame_queue& q)
        {
            frame f;
            while (q.poll_for_frame(&f));
        });

        s.close();
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

    void subdevice_model::play(const std::vector<stream_profile>& profiles)
    {
        s.open(profiles);
        try {
            s.start([&](frame f){
                auto index = f.get_profile().unique_id();
                queues.at(index).enqueue(std::move(f));
            });
        }
        catch (...)
        {
            s.close();
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
            opt_md.update_all_feilds(error_message, notifications);

            if (next_option == RS2_OPTION_ENABLE_AUTO_EXPOSURE)
            {
                auto old_ae_enabled = auto_exposure_enabled;
                auto_exposure_enabled = opt_md.value > 0;

                if (!old_ae_enabled && auto_exposure_enabled)
                {
                    try
                    {
                        if (s.is<roi_sensor>())
                        {
                            auto r = s.as<roi_sensor>().get_region_of_interest();
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
            if(std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
            {
                draw_option(opt, update_read_only_options, error_message, notifications);
            }
        }
    }

    bool subdevice_model::draw_option(rs2_option opt, bool update_read_only_options,
                                      std::string& error_message, notifications_model& model)
    {
        auto&& metadata = options_metadata[opt];
        if (update_read_only_options)
        {
            metadata.update_supported(error_message);
            if (metadata.supported && streaming)
            {
                metadata.update_read_only_status(error_message);
                if (metadata.read_only)
                {
                    metadata.update_all_feilds(error_message, model);
                }
            }
        }
        return metadata.draw(error_message);
    }

    stream_model::stream_model()
        : texture ( std::unique_ptr<texture_buffer>(new texture_buffer()))
    {}

    void stream_model::upload_frame(frame&& f)
    {
        if (dev && dev->is_paused()) return;

        last_frame = std::chrono::high_resolution_clock::now();

        auto image = f.as<video_frame>();
        auto width = (image) ? image.get_width() : 640.f;
        auto height = (image) ? image.get_height() : 480.f;

        size = { static_cast<float>(width), static_cast<float>(height)};
        profile = f.get_profile();
        frame_number = f.get_frame_number();
        timestamp_domain = f.get_frame_timestamp_domain();
        timestamp = f.get_timestamp();
        fps.add_timestamp(f.get_timestamp(), f.get_frame_number());


        // populate frame metadata attributes
        for (auto i=0; i< RS2_FRAME_METADATA_COUNT; i++)
        {
            if (f.supports_frame_metadata((rs2_frame_metadata_value)i))
                frame_md.md_attributes[i] = std::make_pair(true,f.get_frame_metadata((rs2_frame_metadata_value)i));
            else
                frame_md.md_attributes[i].first = false;
        }

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

     void draw_rect(const rect& r)
    {
        glPushAttrib(GL_ENABLE_BIT);

        glLineWidth(1);

        glBegin(GL_LINE_STRIP);
        glVertex2i(r.x, r.y);
        glVertex2i(r.x, r.y + r.h);
        glVertex2i(r.x + r.w, r.y + r.h);
        glVertex2i(r.x + r.w, r.y);
        glVertex2i(r.x, r.y);
        glVertex2i(r.x, r.y + r.h);
        glVertex2i(r.x + r.w, r.y + r.h);
        glVertex2i(r.x + r.w, r.y);
        glVertex2i(r.x, r.y);
        glEnd();

        glPopAttrib();
    }

    float stream_model::get_stream_alpha()
    {
        if (dev && dev->is_paused()) return 1.f;

        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - last_frame;
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = smoothstep(static_cast<float>(ms),
            _min_timeout, _min_timeout + _frame_timeout);
        return 1.0f - t;
    }

    bool stream_model::is_stream_visible()
    {
        if (dev && 
            (dev->is_paused() || (dev->streaming && dev->dev.is<playback>())))
            {
                last_frame = std::chrono::high_resolution_clock::now();
                return true;
            }

        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - last_frame;
        auto ms = duration_cast<milliseconds>(diff).count();
        return ms <= _frame_timeout + _min_timeout;
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
                        if (sensor.is<roi_sensor>())
                        {
                            sensor.as<roi_sensor>().set_region_of_interest(roi);
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
                        if (sensor.is<roi_sensor>())
                        {
                            sensor.as<roi_sensor>().set_region_of_interest({ x_margin, y_margin,
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

        auto model = pc.get_points();

        const auto top_bar_height = 32.f;
        const auto num_of_buttons = 3;

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
                    selected_depth_source_uid = s.second.profile.unique_id();
                }
                if (s.second.profile.unique_id() == selected_depth_source_uid)
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

        if (depth_sources_str.size() > 0)
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
                        selected_depth_source_uid = s.second.profile.unique_id();
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
                s.second.texture->get_last_frame() &&
                s.second.profile.stream_type() != RS2_STREAM_DEPTH)
            {
                if (selected_tex_source_uid == -1)
                {
                    selected_tex_source_uid = s.second.profile.unique_id();
                }
                if (s.second.profile.unique_id() == selected_tex_source_uid)
                {
                    selected_tex_source = i;
                }

                tex_sources.push_back(s.second.profile.unique_id());

                auto dev_name = s.second.dev ? s.second.dev->dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";
                auto stream_name = rs2_stream_to_string(s.second.profile.stream_type());

                tex_sources_str.push_back(to_string() << dev_name << " " << stream_name);

                i++;
            }
        }

        // Only allow to change texture if we have something to put it on:
        if (tex_sources_str.size() > 0 && depth_sources_str.size() > 0)
        {
            ImGui::SetCursorPosY(7);
            ImGui::Text("Texture Source:"); ImGui::SameLine();


            ImGui::SetCursorPosY(7);
            ImGui::PushItemWidth(190);
            draw_combo_box("##Tex Source", tex_sources_str, selected_tex_source);

            i = 0;
            for (auto&& s : streams)
            {
                if (s.second.is_stream_visible() &&
                    s.second.texture->get_last_frame() &&
                    s.second.profile.stream_type() != RS2_STREAM_DEPTH)
                {
                    if (i == selected_tex_source)
                    {
                        selected_tex_source_uid = s.second.profile.unique_id();
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

        if (selected_depth_source_uid >= 0){
            auto depth = streams[selected_depth_source_uid].texture->get_last_frame();
            if (depth) pc.push_frame(depth);
        }

        frame tex;
        if (selected_tex_source_uid >= 0)
        {
            tex = streams[selected_tex_source_uid].texture->get_last_frame();
            if (tex) pc.update_texture(tex);
        }

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

        if (ImGui::Button(u8"\uf0c7", { 30, top_bar_height }))
        {
            export_to_ply(not_model, model, tex);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Export 3D model to PLY format");

        ImGui::SameLine();

        if (ImGui::Button(u8"\uf021", { 30, top_bar_height }))
        {
            reset_camera();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset View");

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
        ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(0x1b, 0x21, 0x25, 200));
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
        std::vector<int> streams_to_remove;
        for (auto&& kvp : streams)
        {
            if (!kvp.second.is_stream_visible() &&
                (!kvp.second.dev || (!kvp.second.dev->is_paused() && !kvp.second.dev->streaming)))
                streams_to_remove.push_back(kvp.first);
        }
        for (auto&& i : streams_to_remove) {
            streams.erase(i);
        }
    }

    void stream_model::show_stream_header(ImFont* font, rs2::rect stream_rect, viewer_model& viewer)
    {
        const auto top_bar_height = 32.f;
        auto num_of_buttons = 4;
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
            dev->s.supports(RS2_CAMERA_INFO_NAME))
        {
            std::string dev_name = dev->dev.get_info(RS2_CAMERA_INFO_NAME);
            std::string dev_serial = dev->dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
            std::string sensor_name = dev->s.get_info(RS2_CAMERA_INFO_NAME);
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
        if (dev->is_paused() ||  (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED))
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
            auto ret = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "Portable Network Graphics (PNG)\0*.png\0", NULL, NULL);

            if (ret)
            {
                std::string filename = ret;
                if (!ends_with(to_lower(filename), ".png")) filename += ".png";

                auto frame = texture->get_last_frame().as<video_frame>();
                if (frame)
                {
                    stbi_write_png(filename.data(), frame.get_width(), frame.get_height(), frame.get_bytes_per_pixel(), frame.get_data(), frame.get_width() * frame.get_bytes_per_pixel());

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

        label = to_string() << u8"\uf00d" << "##Stop " << profile.unique_id();
        if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
        {
            dev->stop();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Stop this sensor");
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

            ImGui::Columns(1);

            ImGui::End();
            ImGui::PopStyleColor(6);
            ImGui::PopStyleVar(2);
        }
    }

    void stream_model::show_stream_footer(rect stream_rect, mouse_info& mouse)
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
                                             .fit({0, 0, 40, 40})
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
        texture->show(stream_rect, get_stream_alpha(), _normalized_zoom);
        update_ae_roi_rect(stream_rect, g, error_message);
        texture->show_preview(stream_rect, _normalized_zoom);

        if (is_middle_clicked)
            _middle_pos = g.cursor;
    }

    void stream_model::show_metadata(const mouse_info& g)
    {
        auto flags = ImGuiWindowFlags_ShowBorders;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.3f, 0.3f, 0.3f, 0.5});
        ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0.f, 0.25f, 0.3f, 1 });
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.f, 0.3f, 0.8f, 1 });
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 });

        std::string label = to_string() << profile.stream_name() << " Stream Metadata";
        ImGui::Begin(label.c_str(), nullptr, flags);

        // Print all available frame metadata attributes
        for (size_t i=0; i < RS2_FRAME_METADATA_COUNT; i++ )
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
            auto model = std::make_shared<subdevice_model>(dev, sub, error_message);
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

                sub->play(profiles);

                for (auto&& profile : profiles)
                {
                    viewer.streams[profile.unique_id()].dev = sub;
                }
            }
        }
    }
    void device_model::draw_device_details(device& dev, context& ctx)
    {
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
                auto value = dev.get_info(info);
                ImGui::Text("%s", value);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", value);
                }
            }
        }
        if(dev.is<playback>())
        {
            auto p = dev.as<playback>();
            if (ImGui::SmallButton("Remove Device"))
            {
                for (auto&& subdevice : subdevices)
                {
                    subdevice->stop();
                }
                ctx.unload_device(p.file_name());
            }
            else
            {
                int64_t total_duration = p.get_duration().count();
                static int seek_pos = 0;
                static int64_t progress = 0;
                progress = p.get_position();

                double part = (1.0 * progress) / total_duration;
                seek_pos = static_cast<int>(std::max(0.0, std::min(part, 1.0)) * 100);

                if(seek_pos != 0 && p.current_status() == RS2_PLAYBACK_STATUS_STOPPED)
                {
                    seek_pos = 0;
                }
                int prev_seek_progress = seek_pos;

                ImGui::SeekSlider("Seek Bar", &seek_pos);
                if (prev_seek_progress != seek_pos)
                {
                    //Seek was dragged
                    auto duration_db =
                        std::chrono::duration_cast<std::chrono::duration<double,
                                                                         std::nano>>(p.get_duration());
                    auto single_percent = duration_db.count() / 100;
                    auto seek_time = std::chrono::duration<double, std::nano>(seek_pos * single_percent);
                    p.seek(std::chrono::duration_cast<std::chrono::nanoseconds>(seek_time));
                }
                    if (ImGui::CollapsingHeader("Playback Options"))
                    {
                        static bool is_paused = p.current_status() == RS2_PLAYBACK_STATUS_PAUSED;
                        if (!is_paused)
                        {
                            if(ImGui::Button("Pause"))
                            {
                                p.pause();
                                for (auto&& sub : subdevices)
                                {
                                    if (sub->streaming) sub->pause();
                                }
                                is_paused = !is_paused;
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Pause playback");
                            }
                        }
                        if (is_paused )
                        {
                            if(ImGui::Button("Resume"))
                            {
                                p.resume();
                                for (auto&& sub : subdevices)
                                {
                                    if (sub->streaming) sub->resume();
                                }
                                is_paused = !is_paused;
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Continue playback");
                            }
                        }
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
        auto log = not_model.get_log();
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::InputTextMultiline("##Log", const_cast<char*>(log.c_str()),
            log.size() + 1, { w, h - 20 }, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();
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
    void viewer_model::show_icon(ImFont* font_18, const char* label_str, const char* text, int x, int y, int id, const ImVec4& text_color)
    {
        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::PushFont(font_18);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::SetNextWindowPos({ (float)x, (float)y });
        ImGui::SetNextWindowSize({ 32.f, 32.f });
        std::string label = to_string() << label_str << id;
        ImGui::Begin(label.c_str(), nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        ImGui::Text("%s", text);
        ImGui::PopStyleColor();

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
    void async_pointclound_mapper::render_loop()
    {
        while (keep_calculating_pointcloud)
        {
            try
            {
                frame f;
                if (depth_frames_to_render.poll_for_frame(&f))
                {
                    if (f.get_frame_number() == last_frame_number &&
                        f.get_timestamp() <= last_timestamp &&
                        f.get_profile().unique_id() == last_stream_id)
                        continue;

                    resulting_3d_models.enqueue(pc.calculate(f));

                    last_frame_number = f.get_frame_number();
                    last_timestamp = f.get_timestamp();
                    last_stream_id = f.get_profile().unique_id();
                }
            }
            catch (...) {}

            // There is no practical reason to re-calculate the 3D model
            // at higher frequency then 100 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

        ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0x70,0x8f, 0xa8, 0xff));
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

    void viewer_model::render_3d_view(const rect& viewer_rect, float scale_factor)
    {
        glViewport(viewer_rect.x * scale_factor, viewer_rect.y * scale_factor, 
                   viewer_rect.w * scale_factor, viewer_rect.h * scale_factor);

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
            glVertex3f(i - 3, 1, 0);
            glVertex3f(i - 3, 1, 6);
            glVertex3f(-3, 1, i);
            glVertex3f(3, 1, i);
        }
        glEnd();

        texture_buffer::draw_axis(0.1, 1);

        glColor4f(1.f, 1.f, 1.f, 1.f);

        if (auto points = pc.get_points())
        {
            glPointSize((float)viewer_rect.w / points.get_profile().as<video_stream_profile>().width());

            if (selected_tex_source_uid >= 0)
            {
                auto tex = streams[selected_tex_source_uid].texture->get_gl_handle();
                glBindTexture(GL_TEXTURE_2D, tex);
                glEnable(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);
            }

            //glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);

            glBegin(GL_POINTS);

            auto vertices = points.get_vertices();
            auto tex_coords = points.get_texture_coordinates();

            for (int i = 0; i < points.size(); i++)
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

    void viewer_model::upload_frame(frame&& f)
    {
        auto index = f.get_profile().unique_id();
        streams[index].upload_frame(std::move(f));
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
                auto model = std::make_shared<subdevice_model>(*_recorder, sub, error_message);
                record_sensors.push_back(model);
            }
            live_subdevices = subdevices;
            subdevices.clear();
            subdevices.swap(record_sensors);
            for(int i=0; i< live_subdevices.size(); i++)
            {
                subdevices[i]->selected_res_id = live_subdevices[i]->selected_res_id;
                subdevices[i]->selected_shared_fps_id = live_subdevices[i]->selected_shared_fps_id;
                subdevices[i]->selected_format_id = live_subdevices[i]->selected_format_id;
                subdevices[i]->show_single_fps_list = live_subdevices[i]->show_single_fps_list;
                subdevices[i]->fpses_per_stream = live_subdevices[i]->fpses_per_stream;
                subdevices[i]->selected_fps_id = live_subdevices[i]->selected_fps_id;
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
        if(ImGui::Combo(label.c_str(), &playback_speed_index, "Speed: x0.25\0Speed: x0.5\0Speed: x1\0Speed: x1.5\0Speed: x2\0\0"))
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

    void device_model::draw_advanced_mode_tab(device& dev,
        std::vector<std::string>& restarting_info)
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
                        if (ImGui::Button("Disable Advanced Mode", ImVec2{ 226, 0 }))
                        {
                            //if (yes_no_dialog()) // TODO
                            //{
                            advanced.toggle_advanced_mode(false);
                            restarting_info = get_device_info(dev, false);
                            //}
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Disabling advanced mode will reset depth generation to factory settings\nThis will not affect calibration");
                        }
                        draw_advanced_mode_controls(advanced, amc, get_curr_advanced_controls);
                    }
                    else
                    {
                        if (ImGui::Button("Enable Advanced Mode", ImVec2{ 226, 0 }))
                        {
                            //if (yes_no_dialog()) // TODO
                            //{
                            advanced.toggle_advanced_mode(true);
                            restarting_info = get_device_info(dev, false);
                            //}
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Advanced mode is a persistent camera state unlocking calibration formats and depth generation controls\nYou can always reset the camera to factory defaults by disabling advanced mode");
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
          _category(category){}


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
        timestamp  = n.get_timestamp();
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
        height = float(lines * 30 + 20);
        ImGui::SetNextWindowSize({ float(315), float(height) });
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
            ss  << "Timestamp: "
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
}
