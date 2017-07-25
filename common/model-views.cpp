// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "model-views.h"

namespace rs2
{
    inline std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec)
    {
        std::vector<const char*> res;
        for (auto&& s : vec) res.push_back(s.c_str());
        return res;
    }

    void option_model::draw(std::string& error_message)
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
                        if (ImGui::SliderIntWithSteps(id.c_str(), &int_value,
                            static_cast<int>(range.min),
                            static_cast<int>(range.max),
                            static_cast<int>(range.step)))
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

            if (opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE && dev->auto_exposure_enabled && dev->streaming)
            {
                ImGui::SameLine(0, 60);

                if (!dev->roi_checked)
                {
                    std::string caption = to_string() << "Set ROI##" << label;
                    if (ImGui::Button(caption.c_str(), { 65, 0 }))
                    {
                        dev->roi_checked = true;
                    }
                }
                else
                {
                    std::string caption = to_string() << "Cancel##" << label;
                    if (ImGui::Button(caption.c_str(), { 65, 0 }))
                    {
                        dev->roi_checked = false;
                    }
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Select custom region of interest for the auto-exposure algorithm\nClick the button, then draw a rect on the frame");
            }
        }
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

    void option_model::update_read_only(std::string& error_message)
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

    void option_model::update_all(std::string& error_message)
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


    subdevice_model::subdevice_model(device& dev, sensor& s, std::string& error_message)
        : s(s), dev(dev), streaming(false), queues(RS2_STREAM_COUNT),
          selected_shared_fps_id(0), _pause(false)
    {
        for (auto& elem : queues)
        {
            elem = std::unique_ptr<frame_queue>(new frame_queue(5));
        }

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
            ss << dev.get_info(RS2_CAMERA_INFO_NAME)
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
            auto uvc_profiles = s.get_stream_modes();
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
        // Draw combo-box with all resolution options for this device
        auto res_chars = get_string_pointers(resolutions);
        ImGui::PushItemWidth(-1);
        ImGui::Text("Resolution:");
        ImGui::SameLine();
        std::string label = to_string() << dev.get_info(RS2_CAMERA_INFO_NAME)
            << s.get_info(RS2_CAMERA_INFO_NAME) << " resolution";
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
            label = to_string() << dev.get_info(RS2_CAMERA_INFO_NAME)
                << s.get_info(RS2_CAMERA_INFO_NAME) << " fps";

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

                label = to_string() << dev.get_info(RS2_CAMERA_INFO_NAME)
                    << s.get_info(RS2_CAMERA_INFO_NAME)
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

                    label = to_string() << s.get_info(RS2_CAMERA_INFO_NAME)
                        << s.get_info(RS2_CAMERA_INFO_NAME)
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

            if (streaming && rgb_rotation_btn && ImGui::Button("Flip Stream Orientation", ImVec2(160, 20)))
            {
                rotate_rgb_image(dev, res_values[selected_res_id].first);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Rotate Sensor 180 deg");
            }
        }
    }

    bool subdevice_model::is_selected_combination_supported()
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

    std::vector<stream_profile> subdevice_model::get_selected_profiles()
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

    void subdevice_model::stop()
    {
        streaming = false;
        _pause = false;

        s.stop();

        for (auto& elem : queues)
        {
            frame f;
            while (elem->poll_for_frame(&f));
        }

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
                auto stream_type = f.get_stream_type();
                queues[(int)stream_type]->enqueue(std::move(f));
            });
        }
        catch (...)
        {
            s.close();
            throw;
        }

        streaming = true;
    }

    void subdevice_model::update(std::string& error_message)
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
                                       bool update_read_only_options, std::string& error_message)
    {
        for (auto& opt : drawing_order)
        {
            draw_option(opt, update_read_only_options, error_message);
        }

        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            auto opt = static_cast<rs2_option>(i);
            if(std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
            {
                draw_option(opt, update_read_only_options, error_message);
            }
        }
    }

    void subdevice_model::draw_option(rs2_option opt, bool update_read_only_options,
                                      std::string& error_message)
    {
        auto&& metadata = options_metadata[opt];
        if (update_read_only_options)
        {
            metadata.update_supported(error_message);
            if (metadata.supported && streaming)
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

        texture->upload(f);
    }

    void stream_model::outline_rect(const rect& r)
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
        if (dev && dev->is_paused())
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
            static const float wheel_step = 0.1f;
            auto mouse_wheel_value = -ImGui::GetIO().MouseWheel;
            if (mouse_wheel_value > wheel_step)
                zoom_val += wheel_step;
            else if (mouse_wheel_value < -wheel_step)
                zoom_val -= wheel_step;
        }

        static const uint32_t middle_button = 2;
        auto is_middle_clicked = ImGui::GetIO().MouseDown[middle_button];

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

    void device_model::reset()
    {
        subdevices.resize(0);
        streams.clear();
        _recorder.reset();
    }

    device_model::device_model(device& dev, std::string& error_message)
    {
        for (auto&& sub : dev.query_sensors())
        {
            auto model = std::make_shared<subdevice_model>(dev, sub, error_message);
            subdevices.push_back(model);
        }
    }

    bool device_model::draw_combo_box(const std::vector<std::string>& device_names, int& new_index)
    {
        std::vector<const char*>  device_names_chars = get_string_pointers(device_names);

        ImGui::PushItemWidth(-1);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", device_names_chars[new_index]);
        }
        return ImGui::Combo("device_model", &new_index, device_names_chars.data(), static_cast<int>(device_names.size()));
        ImGui::PopItemWidth();
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
//                    if (ImGui::CollapsingHeader("Playback Options"))
//                    {
//                        static bool is_paused = false;
//                        if (!is_paused && ImGui::Button("Pause"))
//                        {
//                            p.pause();
//                            for (auto&& sub : model.subdevices)
//                            {
//                                if (sub->streaming) sub->pause();
//                            }
//                            is_paused = !is_paused;
//                        }
//                        if (ImGui::IsItemHovered())
//                        {
//                            ImGui::SetTooltip("Pause playback");
//                        }
//                        if (is_paused && ImGui::Button("Resume"))
//                        {
//                            p.resume();
//                            for (auto&& sub : model.subdevices)
//                            {
//                                if (sub->streaming) sub->resume();
//                            }
//                            is_paused = !is_paused;
//                        }
//                        if (ImGui::IsItemHovered())
//                        {
//                            ImGui::SetTooltip("Continue playback");
//                        }
//                    }
            }
        }
    }

    std::map<rs2_stream, rect> device_model::calc_layout(float x0, float y0, float width, float height)
    {
        std::set<rs2_stream> active_streams;
        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs2_stream>(i);
            if (streams[stream].is_stream_visible())
            {
                active_streams.insert(stream);
            }
        }

        if (fullscreen)
        {
            if (active_streams.count(selected_stream) == 0) fullscreen = false;
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

            auto it = active_streams.begin();
            for (auto x = 0; x < factor; x++)
            {
                for (auto y = 0; y < complement; y++)
                {
                    if (it == active_streams.end()) break;

                    rect r = { x0 + x * cell_width, y0 + y * cell_height,
                        cell_width, cell_height };
                    results[*it] = r;
                    it++;
                }
            }
        }

        return get_interpolated_layout(results);
    }

    void device_model::upload_frame(frame&& f)
    {
        auto stream_type = f.get_stream_type();
        streams[stream_type].upload_frame(std::move(f));
    }

    void device_model::start_recording(device& dev, const std::string& path, std::string& error_message)
    {
        if (_recorder != nullptr)
        {
            return; //already recording
        }
        
        _recorder = std::make_shared<recorder>(path, dev);
        live_subdevices = subdevices;
        subdevices.clear();
        for (auto&& sub : _recorder->query_sensors())
        {
            auto model = std::make_shared<subdevice_model>(dev, sub, error_message);
            subdevices.push_back(model);
        }
		assert(live_subdevices.size() == subdevices.size());
	    for(int i=0; i< live_subdevices.size(); i++)
	    {
			//TODO: Ziv, change this
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
    }

    void device_model::stop_recording()
    {
        subdevices.clear();
        subdevices = live_subdevices;
        live_subdevices.clear();
        //this->streams.clear();
        _recorder.reset();
    }

    void device_model::pause_record()
    {
        _recorder->pause();
    }

    void device_model::resume_record()
    {
        _recorder->resume();
    }

    std::map<rs2_stream, rect> device_model::get_interpolated_layout(const std::map<rs2_stream, rect>& l)
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

    double notification_model::get_age_in_ms()
    {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - created_time).count();
    }

    void notification_model::set_color_scheme(float t)
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

        auto ms = get_age_in_ms() / MAX_LIFETIME_MS;
        auto t = smoothstep(static_cast<float>(ms), 0.7f, 1.0f);

        set_color_scheme(t);
        ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 1, 1 - t });

        auto lines = static_cast<int>(std::count(message.begin(), message.end(), '\n') + 1);
        ImGui::SetNextWindowPos({ float(w - 430), float(y) });
        ImGui::SetNextWindowSize({ float(415), float(lines*50) });
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

    void notifications_model::add_notification(const notification_data& n)
    {
        std::lock_guard<std::mutex> lock(m); // need to protect the pending_notifications queue because the insertion of notifications
                                             // done from the notifications callback and proccesing and removing of old notifications done from the main thread

        notification_model m(n);
        m.index = index++;
        pending_notifications.push_back(m);

        if (pending_notifications.size() > MAX_SIZE)
            pending_notifications.erase(pending_notifications.begin());
    }

    void notifications_model::draw(int w, int h, notification_model& selected)
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

        selected.set_color_scheme(0.f);
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
}
