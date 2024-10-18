// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "stream-model.h"
#include "subdevice-model.h"
#include "viewer.h"
#include "os.h"
#include <imgui_internal.h>


namespace rs2
{
    stream_model::stream_model()
        : texture(std::unique_ptr<texture_buffer>(new texture_buffer())),
        _stream_not_alive(std::chrono::milliseconds(1500)),
        _stabilized_reflectivity(10)
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

    void draw_rect( const rect & r, int line_width, bool draw_cross )
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

        if( draw_cross )
        {
            glLineStipple( 1, 0x0808 );
            glEnable( GL_LINE_STIPPLE );
            glBegin( GL_LINES );
            glVertex2f( r.x, r.y + r.h / 2 );
            glVertex2f( r.x + r.w, r.y + r.h / 2 );
            glEnd();
            glBegin( GL_LINES );
            glVertex2f( r.x + r.w / 2, r.y );
            glVertex2f( r.x + r.w / 2, r.y + r.h );
            glEnd();
        }

        glPopAttrib();
    }

    bool stream_model::is_stream_visible() const
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

    void stream_model::begin_stream(std::shared_ptr<subdevice_model> d, rs2::stream_profile p, const viewer_model& viewer)
    {
        dev = d;
        original_profile = p;

        profile = p;
        texture->colorize = d->depth_colorizer;
        texture->yuy2rgb = d->yuy2rgb;
        texture->y411 = d->y411;

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

        try
        {
            auto ds = d->dev.first< depth_sensor >();
            if( viewer._support_ir_reflectivity
                && ds.supports( RS2_OPTION_ENABLE_IR_REFLECTIVITY )
                && ds.supports( RS2_OPTION_ENABLE_MAX_USABLE_RANGE )
                && ( ( p.stream_type() == RS2_STREAM_INFRARED )
                     || ( p.stream_type() == RS2_STREAM_DEPTH ) ) )
            {
                _reflectivity = std::unique_ptr< reflectivity >( new reflectivity() );
            }
        }
        catch(...) {};

    }

    bool stream_model::draw_reflectivity( int x,
                                          int y,
                                          rs2::depth_sensor ds,
                                          const std::map< int, stream_model > & streams,
                                          std::stringstream & ss,
                                          bool same_line )
    {
        bool reflectivity_valid = false;

        static const int MAX_PIXEL_MOVEMENT_TOLERANCE = 0;

        if( std::abs( _prev_mouse_pos_x - x ) > MAX_PIXEL_MOVEMENT_TOLERANCE
            || std::abs( _prev_mouse_pos_y - y ) > MAX_PIXEL_MOVEMENT_TOLERANCE )
        {
            _reflectivity->reset_history();
            _stabilized_reflectivity.clear();
            _prev_mouse_pos_x = x;
            _prev_mouse_pos_y = y;
        }

        // Get IR sample for getting current reflectivity
        auto ir_stream
            = std::find_if( streams.cbegin(),
                            streams.cend(),
                            []( const std::pair< const int, stream_model > & stream ) {
                                return stream.second.profile.stream_type() == RS2_STREAM_INFRARED;
                            } );

        // Get depth sample for adding to reflectivity history
        auto depth_stream
            = std::find_if( streams.cbegin(),
                            streams.cend(),
                            []( const std::pair< const int, stream_model > & stream ) {
                                return stream.second.profile.stream_type() == RS2_STREAM_DEPTH;
                            } );

        if ((ir_stream != streams.end()) && (depth_stream != streams.end()))
        {
            auto depth_val = 0.0f;
            auto ir_val = 0.0f;
            depth_stream->second.texture->try_pick( x, y, &depth_val );
            ir_stream->second.texture->try_pick( x, y, &ir_val );

            _reflectivity->add_depth_sample( depth_val, x, y );  // Add depth sample to the history

            float noise_est = ds.get_option( RS2_OPTION_NOISE_ESTIMATION );
            auto mur_sensor = ds.as< max_usable_range_sensor >();
            if( mur_sensor )
            {
                auto max_usable_range = mur_sensor.get_max_usable_depth_range();
                reflectivity_valid = true;
                std::string ref_str = "N/A";
                try
                {
                    if (_reflectivity->is_history_full())
                    {
                        auto pixel_ref
                            = _reflectivity->get_reflectivity(noise_est, max_usable_range, ir_val);
                        _stabilized_reflectivity.add(pixel_ref);
                        auto stabilized_pixel_ref = _stabilized_reflectivity.get( 0.75f ); // We use 75% stability for preventing spikes
                        ref_str = rsutils::string::from() << std::dec << round( stabilized_pixel_ref * 100 ) << "%";
                    }
                    else
                    {
                        // Show dots when calculating ,dots count [3-10]
                        int dots_count = static_cast<int>(_reflectivity->get_samples_ratio() * 7);
                        ref_str = "calculating...";
                        ref_str += std::string(dots_count, '.');
                    }
                }
                catch( ... )
                {
                }

                if( same_line )
                    ss << ", Reflectivity: " << ref_str;
                else
                    ss << "\nReflectivity: " << ref_str;
            }
        }

        return reflectivity_valid;
    }

    void stream_model::update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message)
    {
        if (dev->roi_checked)
        {
            auto&& sensor = dev->s;
            // Case 1: Starting Dragging of the ROI rect
            // Pre-condition: not capturing already + mouse is down + we are inside stream rect
            if (!capturing_roi && mouse.mouse_down[0] && stream_rect.contains(mouse.cursor))
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

                // Case 3: We are in middle of dragging (capturing) and mouse was released
                if( ! mouse.mouse_down[0] )
                {
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

        std::string label = rsutils::string::from() << "Stream of " << profile.unique_id();

        ImGui::GetWindowDrawList()->AddRectFilled({ stream_rect.x, stream_rect.y - top_bar_height },
            { stream_rect.x + stream_rect.w, stream_rect.y }, ImColor(sensor_bg));

        if( ! dev )
            throw std::runtime_error( "device is not set for the stream" );

        int offset = 5;
        if (dev->_is_being_recorded) offset += 23;
        auto p = dev->dev.as<playback>();
        if (dev->is_paused() || (p && p.current_status() == RS2_PLAYBACK_STATUS_PAUSED)) offset += 23;

        ImGui::SetCursorScreenPos({ stream_rect.x + 4 + offset, stream_rect.y - top_bar_height + 7 });

        std::string tooltip;
        if (dev->dev.supports(RS2_CAMERA_INFO_NAME) &&
            dev->dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER) &&
            dev->s->supports(RS2_CAMERA_INFO_NAME))
        {
            std::string dev_name = dev->dev.get_info(RS2_CAMERA_INFO_NAME);
            std::string dev_serial = dev->dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
            std::string sensor_name = dev->s->get_info(RS2_CAMERA_INFO_NAME);
            std::string stream_name = rs2_stream_to_string(profile.stream_type());
            std::string stream_index_str;

            // Show stream index on IR streams
            if (profile.stream_type() == RS2_STREAM_INFRARED)
            {
                int stream_index = profile.stream_index();
                stream_index_str = rsutils::string::from() << " #" << stream_index;
            }

            tooltip = rsutils::string::from() << dev_name << " s.n:" << dev_serial << " | " << sensor_name << ", " << stream_name << stream_index_str << " stream";
            const auto approx_char_width = 12;
            if (stream_rect.w - 32 * num_of_buttons >= (dev_name.size() + dev_serial.size() + sensor_name.size() + stream_name.size() + stream_index_str.size()) * approx_char_width)
                label = tooltip;
            else
            {
                // Use only the SKU type for compact representation and use only the last three digits for S.N
                auto short_name = split_string(dev_name, ' ').back();
                auto short_sn = dev_serial;
                short_sn.erase(0, dev_serial.size() - 5).replace(0, 2, "..");

                auto label_length = stream_rect.w - 32 * num_of_buttons;
                

                if (label_length >= (short_name.size() + dev_serial.size() + sensor_name.size() + stream_name.size() + stream_index_str.size()) * approx_char_width)
                    label = rsutils::string::from() << short_name << " s.n:" << dev_serial << " | " << sensor_name << " " << stream_name << stream_index_str << " stream";
                else if (label_length >= (short_name.size() + short_sn.size() + stream_name.size() + stream_index_str.size()) * approx_char_width)
                    label = rsutils::string::from() << short_name << " s.n:" << short_sn << " " << stream_name << stream_index_str << " stream";
                else if (label_length >= (short_name.size() + stream_index_str.size()) * approx_char_width)
                    label = rsutils::string::from() << short_name << " " << stream_name << stream_index_str << " stream";
                else if (label_length >= short_name.size() * approx_char_width)
                    label = rsutils::string::from() << short_name << " " << stream_name;
                else
                    label = "";
            }
        }
        else
        {
            label = rsutils::string::from() << "Unknown " << rs2_stream_to_string(profile.stream_type()) << " stream";
            tooltip = label;
        }

        ImGui::PushTextWrapPos(stream_rect.x + stream_rect.w - 32 * num_of_buttons - 5);
        ImGui::Text("%s", label.c_str());
        if (tooltip != label && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip.c_str());
        ImGui::PopTextWrapPos();

        ImGui::SetCursorScreenPos({ stream_rect.x + stream_rect.w - 32 * num_of_buttons, stream_rect.y - top_bar_height });


        label = rsutils::string::from() << textual_icons::metadata << "##Metadata" << profile.unique_id();
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
            label = rsutils::string::from() << textual_icons::bar_chart << "##Color map";
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
            label = rsutils::string::from() << textual_icons::play << "##Resume " << profile.unique_id();
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
            label = rsutils::string::from() << textual_icons::pause << "##Pause " << profile.unique_id();
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

        label = rsutils::string::from() << textual_icons::camera << "##Snapshot " << profile.unique_id();
        if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
        {
            auto filename = file_dialog_open(save_file, "Portable Network Graphics (PNG)\0*.png\0", nullptr, nullptr);

            if (filename)
            {
                snapshot_frame(filename, viewer);
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save snapshot");
        }
        ImGui::SameLine();

        label = rsutils::string::from() << textual_icons::info_circle << "##Info " << profile.unique_id();
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
                label = rsutils::string::from() << textual_icons::window_maximize << "##Maximize " << profile.unique_id();

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

                label = rsutils::string::from() << textual_icons::window_restore << "##Restore " << profile.unique_id();

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
            label = rsutils::string::from() << textual_icons::times << "##Stop " << profile.unique_id();
            if (ImGui::Button(label.c_str(), { 24, top_bar_height }))
            {
                dev->stop(viewer.not_model);
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
        float tail_w = curr_info_rect.w - 20;
        float min_w = ImGui::CalcTextSize("0").x;
        auto ctx = ImGui::GetCurrentContext();
        float space_w = ctx->Style.ItemSpacing.x;

        if (show_stream_details && !show_metadata)
        {
            if (_info_height.get() > line_y + ImGui::GetTextLineHeight() - curr_info_rect.y)
            {
                ImGui::SetCursorScreenPos({ curr_info_rect.x + 10, line_y });

                if (timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
                    ImGui::PushStyleColor(ImGuiCol_Text, redish);

                label = rsutils::string::from() << "Time: " << std::left << std::fixed << std::setprecision(1) << timestamp << " ";

                if( timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME )
                    label = rsutils::string::from() << textual_icons::exclamation_triangle << label;

                tail_w -= ImGui::CalcTextSize(label.c_str()).x;
                if (tail_w > min_w)
                {
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
                    tail_w -= space_w;
                }

                if (tail_w > min_w)
                {
                    label = rsutils::string::from() << " Frame: " << std::left << frame_number;
                    tail_w -= ImGui::CalcTextSize(label.c_str()).x;
                    if (tail_w > min_w)
                        ImGui::Text("%s", label.c_str());

                    ImGui::SameLine();
                    tail_w -= space_w;
                }

                if (tail_w > min_w)
                {
                    std::string res;
                    if (profile.as<rs2::video_stream_profile>())
                        res = rsutils::string::from() << size.x << "x" << size.y << ",  ";
                    label = rsutils::string::from() << res << truncate_string(rs2_format_to_string(profile.format()), 9) << ", ";
                    tail_w -= ImGui::CalcTextSize(label.c_str()).x;
                    if (tail_w > min_w)
                        ImGui::Text("%s", label.c_str());
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "Stream Resolution, Format");
                    }

                    ImGui::SameLine();
                    tail_w -= space_w;
                }

                if (tail_w > min_w)
                {
                    label = rsutils::string::from() << "FPS: " << std::setprecision(2) << std::setw(7) << std::fixed << fps.get_fps();
                    tail_w -= ImGui::CalcTextSize(label.c_str()).x;
                    if (tail_w > min_w)
                        ImGui::Text("%s", label.c_str());
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "FPS is calculated based on timestamps and not viewer time");
                    }
                }

                line_y += ImGui::GetTextLineHeight() + 5;
            }
        }

        if (show_metadata)
            stream_model::draw_stream_metadata(timestamp, timestamp_domain, frame_number, profile, original_size, stream_rect);

        ImGui::PopStyleColor(5);
    }

    void stream_model::create_stream_details( std::vector< attribute >& stream_details,
                                              const double timestamp,
                                              const rs2_timestamp_domain timestamp_domain,
                                              const unsigned long long frame_number,
                                              const stream_profile profile,
                                              const rs2::float2 original_size )
    {
        stream_details.push_back( { "Frame Timestamp",
                                    rsutils::string::from() << std::fixed << std::setprecision( 1 ) << timestamp,
                                    "Frame Timestamp is normalized represetation of when the frame was taken.\n"
                                    "It's a property of every frame, so when exact creation time is not provided by "
                                    "the hardware, an approximation will be used.\n"
                                    "Clock Domain fields helps to interpret the meaning of timestamp\n"
                                    "Timestamp is measured in milliseconds, and is allowed to roll-over (reset to "
                                    "zero) in some situations" } );
        stream_details.push_back(
            { "Clock Domain",
              rsutils::string::from() << rs2_timestamp_domain_to_string( timestamp_domain ),
              "Clock Domain describes the format of Timestamp field. It can be one of the following:\n"
              "1. System Time - When no hardware timestamp is available, system time of arrival will be used.\n"
              "                 System time benefits from being comparable between device, but suffers from not being "
              "able to approximate latency.\n"
              "2. Hardware Clock - Hardware timestamp is attached to the frame by the device, and is consistent "
              "accross device sensors.\n"
              "                    Hardware timestamp encodes precisely when frame was captured, but cannot be "
              "compared across devices\n"
              "3. Global Time - Global time is provided when the device can both offer hardware timestamp and "
              "implements Global Timestamp Protocol.\n"
              "                 Global timestamps encode exact time of capture and at the same time are comparable "
              "accross devices." } );
        stream_details.push_back(
            { "Frame Number",
              rsutils::string::from() << frame_number,
              "Frame Number is a rolling ID assigned to frames.\n"
              "Most devices do not guarantee consequitive frames to have conseuquitive frame numbers\n"
              "But it is true most of the time" } );

        if( profile.as< rs2::video_stream_profile >() )
        {
            stream_details.push_back( { "Hardware Size",
                                        rsutils::string::from() << original_size.x << " x " << original_size.y,
                                        "Hardware size is the original frame resolution we got from the sensor, before "
                                        "applying post processing filters." } );

            stream_details.push_back( { "Display Size",
                                        rsutils::string::from() << size.x << " x " << size.y,
                                        "When Post-Processing is enabled, the actual display size of the frame may "
                                        "differ from original capture size" } );
        }
        stream_details.push_back(
            { "Pixel Format", rsutils::string::from() << rs2_format_to_string( profile.format() ), "" } );

        stream_details.push_back(
            { "Hardware FPS",
              rsutils::string::from() << std::setprecision( 2 ) << std::fixed << fps.get_fps(),
              "Hardware FPS captures the number of frames per second produced by the device.\n"
              "It is possible and likely that not all of these frames will make it to the application." } );

        stream_details.push_back(
            { "Viewer FPS",
              rsutils::string::from() << std::setprecision( 2 ) << std::fixed << view_fps.get_fps(),
              "Viewer FPS captures how many frames the application manages to render.\n"
              "Frame drops can occur for variety of reasons." } );

        stream_details.push_back( { "", "", "" } );
    }

    void stream_model::draw_stream_metadata( const double timestamp,
                                            const rs2_timestamp_domain timestamp_domain,
                                            const unsigned long long frame_number,
                                            stream_profile profile,
                                            rs2::float2 original_size,
                                            const rect &stream_rect )
    {
        std::vector< attribute > stream_details;

        create_stream_details( stream_details, timestamp, timestamp_domain, frame_number, profile, original_size );

        const std::string no_md = "no md";

        if( timestamp_domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME )
        {
            stream_details.push_back( { no_md, "", ""} );
        }

        std::map< rs2_frame_metadata_value, std::string > descriptions = {
            { RS2_FRAME_METADATA_FRAME_COUNTER, "A sequential index managed per-stream. Integer value" },
            { RS2_FRAME_METADATA_FRAME_TIMESTAMP,
              "Timestamp set by device clock when data readout and transmit commence. Units are device dependent" },
            { RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
              "Timestamp of the middle of sensor's exposure calculated by device. usec" },
            { RS2_FRAME_METADATA_ACTUAL_EXPOSURE,
              "Sensor's exposure width. When Auto Exposure (AE) is on the value is controlled by firmware. usec" },
            { RS2_FRAME_METADATA_GAIN_LEVEL,
              "A relative value increasing which will increase the Sensor's gain factor.\n"
              "When AE is set On, the value is controlled by firmware. Integer value" },
            { RS2_FRAME_METADATA_AUTO_EXPOSURE, "Auto Exposure Mode indicator. Zero corresponds to AE switched off. " },
            { RS2_FRAME_METADATA_WHITE_BALANCE, "White Balance setting as a color temperature. Kelvin degrees" },
            { RS2_FRAME_METADATA_TIME_OF_ARRIVAL, "Time of arrival in system clock" },
            { RS2_FRAME_METADATA_TEMPERATURE,
              "Temperature of the device, measured at the time of the frame capture. Celsius degrees " },
            { RS2_FRAME_METADATA_BACKEND_TIMESTAMP, "Timestamp get from uvc driver. usec" },
            { RS2_FRAME_METADATA_ACTUAL_FPS, "Hardware FPS * 1000 =\n"
              "1000000 * (frame-number - prev-frame-number) / (timestamp - prev-timestamp)" },
            { RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE,
              "Laser power mode. Zero corresponds to Laser power switched off and one for switched on." },
            { RS2_FRAME_METADATA_EXPOSURE_PRIORITY,
              "Exposure priority. When enabled Auto-exposure algorithm is allowed to reduce requested FPS to "
              "sufficiently increase exposure time (an get enough light)" },
            { RS2_FRAME_METADATA_POWER_LINE_FREQUENCY,
              "Power Line Frequency for anti-flickering Off/50Hz/60Hz/Auto. " },
        };

        for( auto i = 0; i < RS2_FRAME_METADATA_COUNT; i++ )
        {
            auto && kvp = frame_md.md_attributes[i];
            if( kvp.first )
            {
                auto val = (rs2_frame_metadata_value)i;
                std::string name = rs2_frame_metadata_to_string( val );
                std::string desc;
                if( descriptions.find( val ) != descriptions.end() )
                    desc = descriptions[val];
                stream_details.push_back( { name, format_value(val, kvp.second), desc } );
            }
        }

        float max_text_width = 0.f;

        for (auto&& kvp : stream_details) {
            max_text_width = std::max(max_text_width, ImGui::CalcTextSize(kvp.name.c_str()).x);
        }

        // Set cursor to metadata start position
        ImGui::SetCursorScreenPos( { stream_rect.x, stream_rect.y } );

        // Creating layer for metadata
        std::string metadata_layer_id = rsutils::string::from() << "##Metadata-" << profile.unique_id();
        ImGui::BeginChild( metadata_layer_id.c_str(), ImVec2( stream_rect.w + 2, stream_rect.h ) );
        auto screen_pos = ImGui::GetCursorScreenPos( );
        const float space_between_columns = 20.f;
        const float space_between_lines = 4.f;
        const float space_from_left = 10.f;

        float line_y = ImGui::GetCursorScreenPos().y;

        for( auto && at : stream_details )
        {
            ImGui::SetCursorScreenPos( { screen_pos.x + space_from_left, line_y } ); // create space from left for metadata labels column

            ImGui::PushStyleColor( ImGuiCol_FrameBg, transparent );
            ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, light_blue );

            if ( at.name == "" ) {
                line_y += ImGui::GetTextLineHeight() + space_between_lines; // Create space separation between stream details and metatada
            }
            else if( at.name == no_md )
            {
                std::vector<std::string> warning_lines = { "Per-frame metadata is not enabled at the OS level!", "Please follow the installation guide for the details." };
                ImGui::PushStyleColor( ImGuiCol_Text, redish );

                // This loop print 2 lines of the warning. This is work around solution because InputText can't show text after new line character.
                for (int i = 0; i < warning_lines.size(); i++) {
                    auto warning_size = ImGui::CalcTextSize(warning_lines[i].c_str());

                    // Draw a smooth rectangle background for a warning.
                    for (auto i = 3; i > 0; i--)
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            { screen_pos.x + space_from_left - i, line_y - i },
                            { screen_pos.x + space_from_left + warning_size.x + i + 10, line_y + warning_size.y + i },
                            ImColor( alpha( sensor_bg, 0.1f ) ) );

                    ImGui::SetCursorScreenPos( { screen_pos.x + space_from_left, line_y } ); // create space from left for metadata labels column.
                    ImGui::PushItemWidth(warning_size.x + 5);

                    std::string metadata_id = rsutils::string::from() << "##" << at.name << "-" << profile.unique_id();
                    ImGui::InputText( metadata_id.c_str(),
                                      (char *)warning_lines[i].c_str(),
                                      warning_lines[i].size(),
                                      ImGuiInputTextFlags_ReadOnly );

                    line_y += ImGui::GetTextLineHeight() + space_between_lines; // Move down to the next line
                }

                ImGui::PopStyleColor(); // remove redish text color.
            }
            else
            {
                std::string text = rsutils::string::from() << at.name << ":";
                auto label_size = ImGui::CalcTextSize(text.c_str());
            
                // Draw a smooth rectangle background for label
                // When we draw multiple rectangles with a shorter pixel on w & h is darker the inside rectangle
                for (auto i = 3; i > 0; i--)
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        { screen_pos.x + space_from_left - i, line_y - i },
                        { screen_pos.x + space_from_left + label_size.x + i + 10, line_y + label_size.y + i },
                        ImColor( alpha( sensor_bg, 0.1f ) ) );

                ImGui::PushItemWidth(label_size.x + 5);  // Set input text width for label.

                std::string label_id = rsutils::string::from() << "##" << at.name << "-" << profile.unique_id();
                ImGui::InputText( label_id.c_str(),
                                  (char *)text.c_str(),
                                  text.size(),
                                  ImGuiInputTextFlags_ReadOnly );
                
                ImGui::PopItemWidth();
                
                if( at.description != "" )
                {
                    if( ImGui::IsItemHovered() )
                    {
                        ImGui::SetTooltip( "%s", at.description.c_str() );
                    }
                }

                text = at.value;
                auto value_size = ImGui::CalcTextSize(text.c_str());
                
                // Draw a smooth rectangle background for label value.
                for (auto i = 3; i > 0; i--)
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        { screen_pos.x + space_from_left + max_text_width + space_between_columns - i, line_y - i },
                        { screen_pos.x + space_from_left + value_size.x + max_text_width + space_between_columns + i + 10, line_y + value_size.y + i },
                        ImColor( alpha( sensor_bg, 0.1f ) ) );
                
                ImGui::SetCursorScreenPos( { screen_pos.x + space_from_left + max_text_width + space_between_columns, line_y } );

                ImGui::PushItemWidth(value_size.x + 5);  // Set input text width for label value.

                std::string value_id = rsutils::string::from() << "##" << at.name << "-" << at.value << "-" << profile.unique_id();
                ImGui::InputText( value_id.c_str(),
                                  (char *)text.c_str(),
                                  text.size(),
                                  ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                
                ImGui::PopItemWidth();

                line_y += ImGui::GetTextLineHeight() + space_between_lines; // Move down to the next line
            }

            ImGui::PopStyleColor( 2 ); // pop transparent ImGuiCol_FrameBg, pop light_blue ImGuiCol_TextSelectedBg.
        }

        ImGui::EndChild();
    }

    std::string stream_model::format_value(rs2_frame_metadata_value& md_val, rs2_metadata_type& attribute_val) const
    {
        if (should_show_in_hex(md_val))
            return rsutils::string::from() << "0x" << std::hex << attribute_val; // return value as hex
        return rsutils::string::from() << attribute_val;
    }

    bool stream_model::should_show_in_hex(rs2_frame_metadata_value& md_val) const
    {
        // place in the SET metadata types you wish to display in HEX format
        static std::unordered_set< int > show_in_hex;

        if (show_in_hex.find(md_val) != show_in_hex.end())
            return true;
        return false;
    }

    void stream_model::show_stream_footer(ImFont* font, const rect &stream_rect, const mouse_info& mouse, const std::map<int, stream_model> &streams, viewer_model& viewer)
    {
        auto non_visual_stream = (profile.stream_type() == RS2_STREAM_GYRO)
            || (profile.stream_type() == RS2_STREAM_ACCEL)
            || (profile.stream_type() == RS2_STREAM_GPIO)
            || (profile.stream_type() == RS2_STREAM_POSE);

        // This scope contains a cursor behavior on visual stream with no metadata on
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
                ss << " 0x" << std::hex << static_cast< int >( round( val ) );
            }

            bool show_max_range = false;
            bool show_reflectivity = false;

            if (texture->get_last_frame().is<depth_frame>())
            {
                // Draw pixel distance
                auto meters = texture->get_last_frame().as<depth_frame>().get_distance(x, y);

                if (viewer.metric_system)
                {
                    // depth is displayed in mm when distance is below 20 cm and gets back to meters when above 30 cm
                    static bool display_in_mm = false;
                    if (!display_in_mm && meters > 0.f && meters < 0.2f)
                    {
                        display_in_mm = true;
                    }
                    else if (display_in_mm && meters > 0.3f)
                    {
                        display_in_mm = false;
                    }
                    if (display_in_mm)
                        ss << std::dec << " = " << std::setprecision(3) << meters * 1000 << " millimeters";
                    else
                        ss << std::dec << " = " << std::setprecision(3) << meters << " meters";
                }
                else
                    ss << std::dec << " = " << std::setprecision(3) << meters / FEET_TO_METER << " feet";

                // Draw maximum usable depth range
                auto ds = sensor_from_frame(texture->get_last_frame())->as<depth_sensor>();
                if (!viewer.is_option_skipped(RS2_OPTION_ENABLE_MAX_USABLE_RANGE))
                {
                    if (ds.supports(RS2_OPTION_ENABLE_MAX_USABLE_RANGE) &&
                        (ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE) == 1.0f))
                    {
                        auto mur_sensor = ds.as<max_usable_range_sensor>();
                        if (mur_sensor)
                        {
                            show_max_range = true;
                            auto max_usable_range = mur_sensor.get_max_usable_depth_range();
                            const float MIN_RANGE = 1.5f;
                            const float MAX_RANGE = 9.0f;
                            // display maximum usable range in range 1.5-9 [m] at 1.5 [m] resolution (rounded)
                            auto max_usable_range_limited = std::min(std::max(max_usable_range, MIN_RANGE), MAX_RANGE);

                            //round to 1.5 [m]
                            auto max_usable_range_rounded = static_cast<int>(max_usable_range_limited / 1.5f) * 1.5f;

                            if (viewer.metric_system)
                                ss << std::dec << "\nMax usable range: " << std::setprecision(1) << max_usable_range_rounded << " meters";
                            else
                                ss << std::dec << "\nMax usable range: " << std::setprecision(1) << max_usable_range_rounded / FEET_TO_METER << " feet";
                        }
                    }
                }

                // Draw IR reflectivity on depth frame
                if (_reflectivity)
                {
                    if (ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY) == 1.0f)
                    {
                        rect roi_for_reflectivity{
                            (float)dev->algo_roi.min_x,
                            (float)dev->algo_roi.min_y,
                            (float)( dev->algo_roi.max_x - dev->algo_roi.min_x ),
                            (float)( dev->algo_roi.max_y - dev->algo_roi.min_y ) };

                        auto normalized_roi = roi_for_reflectivity
                                                  .normalize( _normalized_zoom.unnormalize( get_original_stream_bounds() ) )
                                                  .unnormalize( stream_rect )
                                                  .cut_by( stream_rect );

                        if ((0.2f == roi_percentage) && normalized_roi.contains(mouse.cursor))
                        {
                            // Add reflectivity information on frame, if max usable range is displayed, display reflectivity on the same line
                            show_reflectivity = draw_reflectivity(x, y, ds, streams, ss, show_max_range);
                        }
                    }
                }
            }

            // Draw IR reflectivity on IR frame
            if (_reflectivity)
            {
                bool lf_exist = texture->get_last_frame();
                if (lf_exist)
                {
                    auto ds = sensor_from_frame(texture->get_last_frame())->as<depth_sensor>();
                    if (ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ) == 1.0f )
                    {
                        bool lf_exist = texture->get_last_frame();
                        if (is_stream_alive() && texture->get_last_frame().get_profile().stream_type() == RS2_STREAM_INFRARED)
                        {
                            rect roi_for_reflectivity{
                                (float)dev->algo_roi.min_x,
                                (float)dev->algo_roi.min_y,
                                (float)(dev->algo_roi.max_x - dev->algo_roi.min_x),
                                (float)(dev->algo_roi.max_y - dev->algo_roi.min_y) };

                            auto normalized_roi = roi_for_reflectivity
                                                      .normalize( _normalized_zoom.unnormalize( get_original_stream_bounds() ) )
                                                      .unnormalize( stream_rect )
                                                      .cut_by( stream_rect );

                            if ((0.2f == roi_percentage) && normalized_roi.contains(mouse.cursor))
                            {
                                show_reflectivity = draw_reflectivity(x, y, ds, streams, ss, show_max_range);
                            }
                        }
                    }
                }
            }

            std::string msg(ss.str().c_str());

            ImGui_ScopePushFont(font);

            // adjust windows size to the message length
            auto new_line_start_idx = msg.find_first_of('\n');
            int footer_vertical_size = 35;
            auto width = float(msg.size() * 8);

            // adjust width according to the longest line
            if (show_max_range || show_reflectivity)
            {
                footer_vertical_size = 50;
                auto first_line_size = msg.find_first_of('\n') + 1;
                auto second_line_size = msg.substr(new_line_start_idx).size();
                width = std::max(first_line_size, second_line_size) * 8.0f;
            }

            auto align = 20;
            width += align - (int)width % align;

            ImVec2 pos{ stream_rect.x + 5, stream_rect.y + stream_rect.h - footer_vertical_size };
            ImGui::GetWindowDrawList()->AddRectFilled({ pos.x, pos.y },
                { pos.x + width, pos.y + footer_vertical_size - 5 }, ImColor(dark_sensor_bg));

            ImGui::SetCursorScreenPos({ pos.x + 10, pos.y + 5 });

            std::string label = rsutils::string::from() << "Footer for stream of " << profile.unique_id();

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

            ImGui::Text("%s", msg.c_str());
            ImGui::PopStyleColor(2);
        }
        else
        {
            if (_reflectivity)
            {
                _reflectivity->reset_history();
                _stabilized_reflectivity.clear();
            }
        }
    }

    // This function contains a cursor behavior on IMU stream with no metadata on
    float stream_model::show_stream_imu( ImFont * font,
                                         const rect & stream_rect,
                                         const rs2_vector & axis,
                                         const mouse_info & mouse,
                                         char const * const units,
                                         char const * const title,
                                         float y_offset )
    {
        float total_h = 0.f;
        if (stream_rect.contains(mouse.cursor) && !show_metadata)
        {
            const auto precision = 3;

            ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

            ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);

            if (show_stream_details)
            {
                y_offset += 30;
            }

            std::string label = rsutils::string::from() << "IMU Stream Info of " << profile.unique_id();

            int const line_h = 18;

            ImVec2 pos{ stream_rect.x, stream_rect.y + y_offset };
            ImGui::SetCursorScreenPos({ pos.x + 5, pos.y + 5 });
            if( title )
            {
                auto rc = ImGui::GetCursorPos();
                ImGui::SetCursorPos( { rc.x + 12, rc.y + 4 } );
                ImGui::PushStyleColor( ImGuiCol_Text, from_rgba( 255, 255, 255, 255, true ) );
                ImGui::Text( "%s", title );
                ImGui::PopStyleColor( 1 );
                ImGui::SetCursorPos( { rc.x, rc.y + line_h } );
                total_h += line_h;
            }

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

            std::vector<motion_data> motion_vector = { { "X", axis.x, units, "Vector X", from_rgba(233, 0, 0, 255, true) , from_rgba(233, 0, 0, 255, true), 0},
                                                    { "Y", axis.y, units, "Vector Y", from_rgba(0, 255, 0, 255, true) , from_rgba(2, 100, 2, 255, true), 0},
                                                    { "Z", axis.z, units, "Vector Z", from_rgba(85, 89, 245, 255, true) , from_rgba(0, 0, 245, 255, true), 0},
                                                    { "N", norm, "Norm", "||V|| = SQRT(X^2 + Y^2 + Z^2)",from_rgba(255, 255, 255, 255, true) , from_rgba(255, 255, 255, 255, true), 0} };

            for (auto&& motion : motion_vector)
            {
                auto rc = ImGui::GetCursorPos();
                ImGui::SetCursorPos({ rc.x + 12, rc.y + 4 });
                ImGui::PushStyleColor(ImGuiCol_Text, motion.colorFg);
                ImGui::Text("%s:", motion.name.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", motion.toolTip.c_str());
                }
                ImGui::PopStyleColor(1);

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, motion.colorBg);

                ImGui::PushItemWidth(100);
                ImGui::SetCursorPos({ rc.x + 27 + motion.nameExtraSpace, rc.y + 1 });
                std::string label = rsutils::string::from() << "##" << profile.unique_id() << "." << rc.y << " " << motion.name.c_str();
                std::string coordinate = rsutils::string::from() << std::fixed << std::setprecision(precision) << std::showpos << motion.coordinate;
                ImGui::InputText(label.c_str(), (char*)coordinate.c_str(), coordinate.size() + 1, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();

                ImGui::SetCursorPos({ rc.x + 80 + motion.nameExtraSpace, rc.y + 4 });
                ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(255, 255, 255, 100, true));
                ImGui::Text("(%s)", motion.units.c_str());

                ImGui::PopStyleColor(3);
                ImGui::SetCursorPos({ rc.x, rc.y + line_h });
                total_h += line_h;
            }

            ImGui::PopStyleColor(5);
        }
        return total_h;
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

        std::string label = rsutils::string::from() << "Pose Stream Info of " << profile.unique_id();

        ImVec2 pos{ stream_rect.x, stream_rect.y + y_offset };
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
                ImGui::SetTooltip("%s", pose.toolTip.c_str());
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

            ImGui::SetCursorPos({ rc.x + 100 + (fullScreen ? pose.nameExtraSpace : 0), rc.y + 1 });
            std::string label = rsutils::string::from() << "##" << profile.unique_id() << " " << pose.name.c_str();
            std::string data = "";

            if (pose.strData.empty())
            {
                data = "[";
                std::string comma = "";
                unsigned int i = 0;
                while ((i < 4) && (pose.floatData[i] != FLT_MAX))
                {

                    data += rsutils::string::from() << std::fixed << std::setprecision(pose.precision) << (pose.signedNumber ? std::showpos : std::noshowpos) << comma << pose.floatData[i];
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
                ImGui::SetCursorPos({ rc.x + 300 + (fullScreen ? pose.nameExtraSpace : 0), rc.y + 4 });
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
        if (loc != std::string::npos)
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

        auto last_frame = texture->get_last_frame( false );
        auto original_frame = last_frame.as< video_frame >();

        // For Depth-originated streams also provide a copy of the raw data accompanied by sensor-specific metadata
        if (original_frame && val_in_range(original_frame.get_profile().stream_type(), { RS2_STREAM_DEPTH , RS2_STREAM_INFRARED }))
        {
            stream_desc = rs2_stream_to_string(original_frame.get_profile().stream_type());

            //Capture raw frame
            auto filename = filename_base + "_" + stream_desc + ".raw";
            if (save_frame_raw_data(filename, original_frame))
                ss << "Raw data is captured into " << filename << std::endl;
            else
                viewer.not_model->add_notification({ rsutils::string::from() << "Failed to save frame raw data  " << filename,
                    RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });

            // And the frame's attributes
            filename = filename_base + "_" + stream_desc + "_metadata.csv";

            try
            {
                if (frame_metadata_to_csv(filename, original_frame))
                    ss << "The frame attributes are saved into " << filename;
                else
                    viewer.not_model->add_notification({ rsutils::string::from() << "Failed to save frame metadata file " << filename,
                        RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
            catch (std::exception& e)
            {
                viewer.not_model->add_notification({ rsutils::string::from() << e.what(),
                    RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
        }

        auto motion = last_frame.as< motion_frame >();
        if( motion )
        {
            stream_desc = rs2_stream_to_string( motion.get_profile().stream_type() );

            // And the frame's attributes
            auto filename = filename_base + "_" + stream_desc + ".csv";

            try
            {
                if( motion_data_to_csv( filename, motion ) )
                    ss << "The frame attributes are saved into\n" << filename;
                else
                    viewer.not_model->add_notification(
                        { rsutils::string::from() << "Failed to save frame file " << filename,
                          RS2_LOG_SEVERITY_INFO,
                          RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR } );
            }
            catch( std::exception & e )
            {
                viewer.not_model->add_notification( { rsutils::string::from() << e.what(),
                                                      RS2_LOG_SEVERITY_INFO,
                                                      RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR } );
            }
        }

        auto pose = last_frame.as< pose_frame >();
        if( pose )
        {
            stream_desc = rs2_stream_to_string( pose.get_profile().stream_type() );

            // And the frame's attributes
            auto filename = filename_base + "_" + stream_desc + ".csv";

            try
            {
                if( pose_data_to_csv( filename, pose ) )
                    ss << "The frame attributes are saved into\n" << filename;
                else
                    viewer.not_model->add_notification(
                        { rsutils::string::from() << "Failed to save frame file " << filename,
                          RS2_LOG_SEVERITY_INFO,
                          RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR } );
            }
            catch( std::exception & e )
            {
                viewer.not_model->add_notification( { rsutils::string::from() << e.what(),
                                                      RS2_LOG_SEVERITY_INFO,
                                                      RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR } );
            }
        }
        if (ss.str().size())
            viewer.not_model->add_notification(notification_data{
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
        // Allow mouse scrolling for zoom when not displaying scrollable metadata
        if (stream_rect.contains(g.cursor) && !show_metadata)
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

        if( dev )
        {
            _normalized_zoom = get_normalized_zoom( stream_rect, g, is_middle_clicked, zoom_val );
            texture->show(stream_rect, 1.f, _normalized_zoom);

            if( dev->show_algo_roi )
            {
                rect r{ float(dev->algo_roi.min_x), float(dev->algo_roi.min_y),
                        float(dev->algo_roi.max_x - dev->algo_roi.min_x),
                        float(dev->algo_roi.max_y - dev->algo_roi.min_y) };

                r = r.normalize(_normalized_zoom.unnormalize(get_original_stream_bounds())).unnormalize(stream_rect).cut_by(stream_rect);
                glColor3f(yellow.x, yellow.y, yellow.z);
                draw_rect(r, 2, true);

                std::string message = "Metrics Region of Interest";
                auto msg_width = stb_easy_font_width((char*)message.c_str());
                if (msg_width < r.w)
                    draw_text(static_cast<int>(r.x + r.w / 2 - msg_width / 2), static_cast<int>(r.y + 10), message.c_str());

                glColor3f(1.f, 1.f, 1.f);
                roi_percentage = dev->roi_percentage;
            }

            update_ae_roi_rect(stream_rect, g, error_message);
        }
        texture->show_preview(stream_rect, _normalized_zoom);

        if (is_middle_clicked)
            _middle_pos = g.cursor;
    }
}
