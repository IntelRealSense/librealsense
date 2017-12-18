#include <iomanip>
#include "depth-quality-model.h"
#include <librealsense2/rs_advanced_mode.hpp>
#include "model-views.h"

namespace rs2
{
    namespace depth_quality
    {
        tool_model::tool_model()
            : _update_readonly_options_timer(std::chrono::seconds(6)), _roi_percent(0.4f),
              _roi_located(std::chrono::seconds(4)),
              _too_close(std::chrono::seconds(4)),
              _too_far(std::chrono::seconds(4)),
              _skew_right(std::chrono::seconds(1)),
              _skew_left(std::chrono::seconds(1)),
              _skew_up(std::chrono::seconds(1)),
              _skew_down(std::chrono::seconds(1)),
              _angle_alert(std::chrono::seconds(4)),
              _min_dist(300.f), _max_dist(2000.f), _max_angle(10.f)
        {
            _viewer_model.is_3d_view = true;
            _viewer_model.allow_3d_source_change = false;
            _viewer_model.allow_stream_close = false;
            _viewer_model.draw_plane = true;
            _viewer_model.synchronization_enable = false;
            _viewer_model.support_non_syncronized_mode = false; //pipeline outputs only syncronized frameset
        }

        bool tool_model::start(ux_window& window)
        {
            bool valid_config = false;
            std::vector<rs2::config> cfgs;

            rs2::config cfg;
            // Preferred configuration Depth + Synthetic Color
            cfg.enable_stream(RS2_STREAM_DEPTH, -1, 0, 0, RS2_FORMAT_Z16, 30);
            cfg.enable_stream(RS2_STREAM_INFRARED, -1, 0, 0, RS2_FORMAT_RGB8, 30);
            cfgs.push_back(cfg);

            // Use Infrared luminocity as a secondary video in case synthetic chroma is not supported
            cfg.disable_all_streams();
            cfg.enable_stream(RS2_STREAM_DEPTH, 0, 0, 0, RS2_FORMAT_Z16, 30);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1, 0, 0, RS2_FORMAT_Y8, 30);
            cfgs.push_back(cfg);

            for (auto& cfg : cfgs)
            {
                if (valid_config = cfg.can_resolve(_pipe))
                {
                    try {
                        _pipe.start(cfg);
                        break;
                    }
                    catch (...)
                    {
                        if (!_device_in_use)
                        {
                            window.add_on_load_message("Device is not functional or busy!");
                            _device_in_use = true;
                        }
                    }
                }
            }

            if (valid_config)
            {
                // Toggle advanced mode
                auto dev = _pipe.get_active_profile().get_device();
                if (dev.is<rs400::advanced_mode>())
                {
                    auto advanced_mode = dev.as<rs400::advanced_mode>();
                    if (!advanced_mode.is_enabled())
                    {
                        window.add_on_load_message("Toggling device into Advanced Mode...");
                        advanced_mode.toggle_advanced_mode(true);
                        valid_config = false;
                    }
                }

                update_configuration();
            }

            _ctx.set_devices_changed_callback([this, &window](rs2::event_information info) mutable
            {
                auto dev = get_active_device();
                if (dev && info.was_removed(dev))
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    reset(window);
                }
            });

            return valid_config;
        }

        void draw_notification(ux_window& win, const rect& viewer_rect, int w,
            const std::string& msg, const std::string& second_line)
        {
            auto flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_Text, yellow);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, blend(sensor_bg, 0.8f));
            ImGui::SetNextWindowPos({ viewer_rect.w / 2 + viewer_rect.x - w / 2, viewer_rect.h / 2 + viewer_rect.y - 38.f });

            ImGui::SetNextWindowSize({ float(w), 76.f });
            ImGui::Begin(msg.c_str(), nullptr, flags);

            if (second_line != "")
            {
                auto pos = ImGui::GetCursorPos();
                ImGui::SetCursorPosY(pos.y - 10);
            }
            ImGui::PushFont(win.get_large_font());
            ImGui::Text("%s", msg.c_str());
            ImGui::PopFont();

            ImGui::PushFont(win.get_font());
            ImGui::Text("%s", second_line.c_str());
            ImGui::PopFont();

            ImGui::End();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
        }

        bool tool_model::draw_instructions(ux_window& win, const rect& viewer_rect, bool& distance, bool& orientation)
        {
            auto plane_fit_found = is_valid(_metrics_model.get_plane());
            _metrics_model.set_plane_fit(plane_fit_found);
            _roi_located.add_value(plane_fit_found);
            if (!_roi_located.eval())
            {
                draw_notification(win, viewer_rect, 450,
                    u8"\n   \uf1b2  Please point the camera to a flat Wall / Surface!",
                    "");
                return false;
            }

            _angle_alert.add_value(fabs(_metrics_model.get_last_metrics().angle) > _max_angle);
            if (_angle_alert.eval())
            {
                orientation = true;
            }

            _skew_right.add_value(orientation && _metrics_model.get_last_metrics().angle_x > 0.f
                && fabs(_metrics_model.get_last_metrics().angle_x) > fabs(_metrics_model.get_last_metrics().angle_y));

            _skew_left.add_value(orientation && _metrics_model.get_last_metrics().angle_x < 0.f
                && fabs(_metrics_model.get_last_metrics().angle_x) > fabs(_metrics_model.get_last_metrics().angle_y));

            _skew_up.add_value(orientation && _metrics_model.get_last_metrics().angle_y < 0.f
                && fabs(_metrics_model.get_last_metrics().angle_x) <= fabs(_metrics_model.get_last_metrics().angle_y));

            _skew_down.add_value(orientation && _metrics_model.get_last_metrics().angle_y > 0.f
                && fabs(_metrics_model.get_last_metrics().angle_x) <= fabs(_metrics_model.get_last_metrics().angle_y));

            _too_close.add_value(_metrics_model.get_last_metrics().distance < _min_dist);
            _too_far.add_value(_metrics_model.get_last_metrics().distance > _max_dist);

            constexpr const char* orientation_instruction = "                         Recommended angle: < 3 degrees"; // "             Use the orientation gimbal to align the camera";
            constexpr const char* distance_instruction    = "          Recommended distance: 0.3m-2m from the target"; // "             Use the distance locator to position the camera";

            if (_skew_right.eval())
            {
                draw_notification(win, viewer_rect, 400,
                    u8"\n          \uf061  Rotate the camera slightly Right",
                    orientation_instruction);
                return false;
            }

            if (_skew_left.eval())
            {
                draw_notification(win, viewer_rect, 400,
                    u8"\n           \uf060  Rotate the camera slightly Left",
                    orientation_instruction);
                return false;
            }

            if (_skew_up.eval())
            {
                draw_notification(win, viewer_rect, 400,
                    u8"\n            \uf062  Rotate the camera slightly Up",
                    orientation_instruction);
                return false;
            }

            if (_skew_down.eval())
            {
                draw_notification(win, viewer_rect, 400,
                    u8"\n          \uf063  Rotate the camera slightly Down",
                    orientation_instruction);
                return false;
            }

            if (_too_close.eval())
            {
                draw_notification(win, viewer_rect, 400,
                    u8"\n          \uf0b2  Move the camera further Away",
                    distance_instruction);
                distance = true;
                return true; // Show metrics even when too close/far
            }

            if (_too_far.eval())
            {
                draw_notification(win, viewer_rect, 400,
                    u8"\n        \uf066  Move the camera Closer to the wall",
                    distance_instruction);
                distance = true;
                return true;
            }

            return true;
        }

        void tool_model::draw_guides(ux_window& win, const rect& viewer_rect, bool distance_guide, bool orientation_guide)
        {
            static const float fade_factor = 0.6f;
            static timer animation_clock;

            auto flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            bool any_guide = distance_guide || orientation_guide;
            if (any_guide)
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, dark_sensor_bg);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, dark_sensor_bg);
            }

            const auto window_y = viewer_rect.y + viewer_rect.h / 2;
            const auto window_h = viewer_rect.h / 2 - 5;
            ImGui::SetNextWindowPos({ viewer_rect.w + viewer_rect.x - 95.f, window_y });
            ImGui::SetNextWindowSize({ 90.f, window_h });
            ImGui::Begin("Guides", nullptr, flags);

            ImGui::PushStyleColor(ImGuiCol_Text,
                blend(light_grey, any_guide ? 1.f : fade_factor));

            //ImGui::PushFont(win.get_large_font());
            //ImGui::Text(u8"\uf1e5 ");
            //ImGui::PopFont();

            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text,
                blend(light_grey, orientation_guide ? 1.f : fade_factor));

            ImGui::Text("Orientation:");
            auto angle = _metrics_model.get_last_metrics().angle;
            auto x = _metrics_model.get_last_metrics().angle_x;
            auto y = _metrics_model.get_last_metrics().angle_y;

            static float prev_x = 0.f;
            static float prev_y = 0.f;

            prev_x = (prev_x + x) / 2.f;
            prev_y = (prev_y + y) / 2.f;

            ImGui::Text("%.1f degree", angle);

            auto pos = ImGui::GetCursorPos();

            ImGui::SetCursorPos({ pos.x, pos.y + 5 });
            pos = ImGui::GetCursorPos();
            auto pos1 = ImGui::GetCursorScreenPos();

            ImGui::GetWindowDrawList()->AddCircle(
            { pos1.x + 41, pos1.y + 40 }, 41,
                ImColor(blend(light_grey, orientation_guide ? 1.f : fade_factor)), 64);

            for (int i = 2; i < 7; i += 1)
            {
                auto t = (animation_clock.elapsed_ms() / 500) * M_PI - i * (M_PI / 5);
                float alpha = (1.f + float(sin(t))) / 2.f;

                auto c = blend(grey, (1.f - float(i)/7.f)*fade_factor);
                if (orientation_guide) c = blend(light_blue, alpha);

                ImGui::GetWindowDrawList()->AddCircle(
                { pos1.x + 41 - 2 * i * prev_x, pos1.y + 40 - 2 * i * prev_y }, 40.f - i*i,
                    ImColor(c), 64);
            }

            if (angle < 50)
            {
                ImGui::GetWindowDrawList()->AddCircleFilled(
                { pos1.x + 41 + 70 * prev_x, pos1.y + 40 + 70 * prev_y }, 10.f,
                    ImColor(blend(grey, orientation_guide ? 1.f : fade_factor)), 64);
            }

            ImGui::SetCursorPos({ pos.x, pos.y + 90 });

            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text,
                blend(light_grey, distance_guide ? 1.f : fade_factor));

            if (window_h > 100)
            {
                ImGui::Text("Distance:");

                auto wall_dist = _metrics_model.get_last_metrics().distance;
                ImGui::Text("%.2f mm", wall_dist);

                if (window_h > 220)
                {
                    pos = ImGui::GetCursorPos();
                    pos1 = ImGui::GetCursorScreenPos();
                    auto min_y = pos1.y + 10;
                    auto max_y = window_h + window_y;
                    int bar_spacing = 15;
                    int parts = int(max_y - min_y) / bar_spacing;

                    ImGui::GetWindowDrawList()->AddRect(
                    { pos1.x + 1, pos1.y },
                    { pos1.x + 81, max_y - 5 },
                        ImColor(blend(light_grey, distance_guide ? 1.f : fade_factor * fade_factor)));

                    for (int i = 0; i < parts; i++)
                    {
                        auto y = min_y + bar_spacing * i + 5;
                        auto t = 1.f - float(i) / (parts - 1);
                        auto tnext = 1.f - float(i + 1) / (parts - 1);
                        float d = t * 1.5f * _max_dist;
                        float dnext = tnext * 1.5f * _max_dist;

                        auto c = light_grey;

                        ImGui::GetWindowDrawList()->AddLine({ pos1.x + 45, y },
                        { pos1.x + 80, y },
                            ImColor(blend(c, distance_guide ? 1.f : fade_factor * fade_factor)));
                        if (d >= _min_dist && d <= _max_dist)
                            c = green;
                        ImGui::SetCursorPos({ pos.x + 7, pos.y + bar_spacing * i + 5 });
                        ImGui::PushStyleColor(ImGuiCol_Text, blend(c, distance_guide ? 1.f : fade_factor));
                        ImGui::Text("%.0f", d);
                        ImGui::PopStyleColor();

                        _depth_scale_events[i].add_value(d > wall_dist && dnext <= wall_dist);
                        if (_depth_scale_events[i].eval())
                        {
                            for (int j = -2; j < 2; j++)
                            {
                                auto yc = yellow;
                                auto factor = (1 - abs(j) / 3.f);
                                yc = blend(yc, factor*factor);
                                if (!distance_guide) yc = blend(yc, fade_factor);
                                ImGui::GetWindowDrawList()->AddRectFilled(
                                { pos1.x + 45, y + (bar_spacing * j) + 1 },
                                { pos1.x + 80, y + (bar_spacing * (j + 1)) }, ImColor(yc));
                            }

                            if (wall_dist < _min_dist)
                            {
                                for (int j = 1; j < 5; j++)
                                {
                                    auto t = (animation_clock.elapsed_ms() / 500) * M_PI - j * (M_PI / 5);
                                    auto alpha = (1 + float(sin(t))) / 2.f;

                                    ImGui::SetCursorPos({ pos.x + 57, pos.y + bar_spacing * (i - j) + 14 });
                                    ImGui::PushStyleColor(ImGuiCol_Text,
                                        blend(blend(light_grey, alpha), distance_guide ? 1.f : fade_factor));
                                    ImGui::Text(u8"\uf106");
                                    ImGui::PopStyleColor();
                                }
                            }

                            if (wall_dist > _max_dist)
                            {
                                for (int j = 1; j < 5; j++)
                                {
                                    auto t = (animation_clock.elapsed_ms() / 500) * M_PI - j * (M_PI / 5);
                                    auto alpha = (1.f + float(sin(t))) / 2.f;

                                    ImGui::SetCursorPos({ pos.x + 57, pos.y + bar_spacing * (i + j) + 14 });
                                    ImGui::PushStyleColor(ImGuiCol_Text,
                                        blend(blend(light_grey, alpha), distance_guide ? 1.f : fade_factor));
                                    ImGui::Text(u8"\uf107");
                                    ImGui::PopStyleColor();
                                }
                            }
                        }
                    }
                }
            }

            ImGui::PopStyleColor();

            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }

        void tool_model::render(ux_window& win)
        {
            rect viewer_rect = { _viewer_model.panel_width,
                _viewer_model.panel_y, win.width() -
                _viewer_model.panel_width,
                win.height() - _viewer_model.panel_y };

            if (_first_frame)
            {
                _viewer_model.update_3d_camera(viewer_rect, win.get_mouse(), true);
                _first_frame = false;
            }

            _viewer_model.show_top_bar(win, viewer_rect);
            _viewer_model.roi_rect = _metrics_model.get_plane();

            bool distance_guide = false;
            bool orientation_guide = false;
            bool found = draw_instructions(win, viewer_rect, distance_guide, orientation_guide);

            ImGui::PushStyleColor(ImGuiCol_WindowBg, button_color);
            ImGui::SetNextWindowPos({ 0, 0 });
            ImGui::SetNextWindowSize({ _viewer_model.panel_width, _viewer_model.panel_y });
            ImGui::Begin("Add Device Panel", nullptr, viewer_ui_traits::imgui_flags);
            ImGui::End();
            ImGui::PopStyleColor();

            // Set window position and size
            ImGui::SetNextWindowPos({ 0, _viewer_model.panel_y });
            ImGui::SetNextWindowSize({ _viewer_model.panel_width, win.height() - _viewer_model.panel_y });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, sensor_bg);

            // *********************
            // Creating window menus
            // *********************
            ImGui::Begin("Control Panel", nullptr, viewer_ui_traits::imgui_flags | ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::SetContentRegionWidth(_viewer_model.panel_width - 26);

            if (_device_model.get())
            {
                device_model* device_to_remove = nullptr;
                std::vector<std::function<void()>> draw_later;
                auto windows_width = ImGui::GetContentRegionMax().x;

                _device_model->draw_controls(_viewer_model.panel_width, _viewer_model.panel_y,
                    win,
                    _error_message, device_to_remove, _viewer_model, windows_width,
                    _update_readonly_options_timer,
                    draw_later);

                ImGui::SetContentRegionWidth(windows_width);
                auto pos = ImGui::GetCursorScreenPos();

                for (auto&& lambda : draw_later)
                {
                    try
                    {
                        lambda();
                    }
                    catch (const error& e)
                    {
                        _error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        _error_message = e.what();
                    }
                }
                // Restore the cursor position after invoking lambdas
                ImGui::SetCursorScreenPos(pos);

                if (_depth_sensor_model.get())
                {
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sensor_bg);
                    ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0xc3, 0xd5, 0xe5, 0xff));
                    ImGui::PushFont(win.get_font());

                    ImGui::PushStyleColor(ImGuiCol_Header, sensor_header_light_blue);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 10 });
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });

                    if (ImGui::TreeNodeEx("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleVar();
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, 2 });

                        if (_depth_sensor_model->draw_stream_selection())
                        {
                            if (_depth_sensor_model->is_selected_combination_supported())
                            {
                                // Preserve streams and ui selections
                                auto primary = _depth_sensor_model->get_selected_profiles().front().as<video_stream_profile>();
                                auto secondary = _pipe.get_active_profile().get_streams().back().as<video_stream_profile>();
                                _depth_sensor_model->store_ui_selection();

                                _pipe.stop();

                                rs2::config cfg;
                                // We have a single resolution control that obides both streams
                                cfg.enable_stream(primary.stream_type(), primary.stream_index(),
                                    primary.width(), primary.height(), primary.format(), primary.fps());
                                cfg.enable_stream(secondary.stream_type(), secondary.stream_index(),
                                    primary.width(), primary.height(), secondary.format(), primary.fps());

                                // The secondary stream may use its previous resolution when appropriate
                                if (!cfg.can_resolve(_pipe))
                                {
                                    cfg.disable_stream(secondary.stream_type());
                                    cfg.enable_stream(secondary.stream_type(), secondary.stream_index(),
                                        secondary.width(), secondary.height(), secondary.format(), primary.fps());
                                }

                                // Wait till a valid device is registered and responsive
                                bool success = false;
                                do
                                {
                                    try // Retries are needed to cope with HW stability issues
                                    {
                                        _pipe.start(cfg);
                                        success = true;
                                    }
                                    catch (...)
                                    {
                                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                    }
                                } while (!success);

                                update_configuration();
                            }
                            else
                            {
                                _error_message = "Selected configuration is not supported!";
                                _depth_sensor_model->restore_ui_selection();
                            }
                        }

                        auto col0 = ImGui::GetCursorPosX();
                        auto col1 = 145.f;

                        ImGui::Text("Region of Interest:");
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);

                        ImGui::PushItemWidth(-1);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });

                        static std::vector<std::string> items{ "80%", "60%", "40%", "20%" };
                        if (draw_combo_box("##ROI Percent", items, _roi_combo_index))
                        {
                            if (_roi_combo_index == 0) _roi_percent = 0.8f;
                            else if (_roi_combo_index == 1) _roi_percent = 0.6f;
                            else if (_roi_combo_index == 2) _roi_percent = 0.4f;
                            else if (_roi_combo_index == 3) _roi_percent = 0.2f;
                            update_configuration();
                        }

                        ImGui::PopStyleColor();
                        ImGui::PopItemWidth();
                        ImGui::SetCursorPosX(col0);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                        ImGui::Text("Distance:");
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Estimated distance to the wall in mm");
                        }
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);
                        ImGui::Text("%.2f mm", _metrics_model.get_last_metrics().distance);

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);
                        std::string gt_str("Ground Truth");
                        if (_use_ground_truth) gt_str += ":";
                        if (ImGui::Checkbox(gt_str.c_str(), &_use_ground_truth))
                        {
                            if (_use_ground_truth) _metrics_model.set_ground_truth(_ground_truth);
                            else _metrics_model.disable_ground_truth();
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("True measured distance to the wall in mm");
                        }
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);
                        if (_use_ground_truth)
                        {
                            ImGui::PushItemWidth(120);
                            if (ImGui::InputInt("##GT", &_ground_truth, 1))
                            {
                                _metrics_model.set_ground_truth(_ground_truth);
                            }
                            ImGui::PopItemWidth();
                            ImGui::SetCursorPosX(col1 + 120); ImGui::SameLine();
                            ImGui::Text("(mm)");
                        }
                        else
                        {
                            _ground_truth = int(_metrics_model.get_last_metrics().distance);
                            ImGui::Dummy({ 1,1 });
                        }
                        ImGui::PopStyleColor();

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                        ImGui::Text("Angle:");
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Estimated angle to the wall in degrees");
                        }
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);
                        ImGui::Text("%.2f deg", _metrics_model.get_last_metrics().angle);

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                        ImGui::TreePop();
                    }

                    ImGui::PopStyleVar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });

                    ImGui::PopStyleVar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });

                    if (ImGui::TreeNodeEx("Metrics", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleVar();
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, 2 });

                        _metrics_model.render(win);

                        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                        if (ImGui::Button(u8"\uf0c7 Save Report", { 140, 25 }))
                        {
                            snapshot_metrics();
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Save Metrics snapshot. This will create:\nPNG image with the depth frame\nPLY 3D model with the point cloud\nJSON file with camera settings you can load later\nand a CSV with metrics recent values");
                        }
                        ImGui::PopStyleColor(2);

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                        ImGui::TreePop();
                    }

                    ImGui::PopStyleVar();
                    ImGui::PopStyleVar();
                    ImGui::PopFont();
                    ImGui::PopStyleColor(3);
                }
            }

            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            try
            {
                frameset f;
                if (_pipe.poll_for_frames(&f))
                {
                    _viewer_model.ppf.frames_queue[f.get_profile().unique_id()].enqueue(f);
                }
                frame dpt = _viewer_model.handle_ready_frames(viewer_rect, win, 1, _error_message);
                if (dpt)
                    _metrics_model.begin_process_frame(dpt);
            }
            catch (...){} // on device disconnect

        }

        void tool_model::update_configuration()
        {
            // Capture the old configuration before reconfiguring the stream
            bool save = false;
            subdevice_ui_selection prev_ui;

            if (_depth_sensor_model)
            {
                prev_ui = _depth_sensor_model->last_valid_ui;
                save = true;

                // Clean-up the models for new configuration
                for (auto&& s : _device_model->subdevices)
                    s->streaming = false;
                _viewer_model.gc_streams();
                _viewer_model.ppf.reset();
                _viewer_model.selected_depth_source_uid = -1;
                _viewer_model.selected_tex_source_uid = -1;
            }

            auto dev = _pipe.get_active_profile().get_device();
            auto dpt_sensor = std::make_shared<sensor>(dev.first<depth_sensor>());
            _device_model = std::shared_ptr<rs2::device_model>(new device_model(dev, _error_message, _viewer_model));
            _device_model->allow_remove = false;
            _device_model->show_depth_only = true;
            _device_model->show_stream_selection = false;
            _depth_sensor_model = std::shared_ptr<rs2::subdevice_model>(
                new subdevice_model(dev, dpt_sensor, _error_message));
            _depth_sensor_model->draw_streams_selector = false;
            _depth_sensor_model->draw_fps_selector = true;

            // Retrieve stereo baseline for supported devices
            auto baseline_mm = -1.f;
            auto profiles = dpt_sensor->get_stream_profiles();
            auto right_sensor = std::find_if(profiles.begin(), profiles.end(), [](rs2::stream_profile& p)
            { return (p.stream_index() == 2) && (p.stream_type() == RS2_STREAM_INFRARED); });

            if (right_sensor != profiles.end())
            {
                auto left_sensor = std::find_if(profiles.begin(), profiles.end(), [](rs2::stream_profile& p)
                { return (p.stream_index() == 0) && (p.stream_type() == RS2_STREAM_DEPTH); });
                try
                {
                    auto extrin = (*left_sensor).get_extrinsics_to(*right_sensor);
                    baseline_mm = fabs(extrin.translation[0]) * 1000;  // baseline in mm
                }
                catch (...) {
                    _error_message = "Extrinsic parameters are not available";
                }
            }

            _metrics_model.reset();

            // Restore GUI controls to the selected configuration
            if (save)
            {
                _depth_sensor_model->ui = _depth_sensor_model->last_valid_ui = prev_ui;
            }

            // Connect the device_model to the viewer_model
            for (auto&& sub : _device_model.get()->subdevices)
            {
                if (!sub->s->is<depth_sensor>()) continue;

                sub->show_algo_roi = true;
                auto profiles = _pipe.get_active_profile().get_streams();
                sub->streaming = true;      // The streaming activated externally to the device_model
                sub->depth_colorizer->set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0.f);
                sub->depth_colorizer->set_option(RS2_OPTION_MIN_DISTANCE, 0.3f);
                sub->depth_colorizer->set_option(RS2_OPTION_MAX_DISTANCE, 2.7f);
                sub->options_invalidated = true;

                for (auto&& profile : profiles)
                {
                    _viewer_model.begin_stream(sub, profile);
                    _viewer_model.streams[profile.unique_id()].texture->colorize = sub->depth_colorizer;

                    if (profile.stream_type() == RS2_STREAM_DEPTH)
                    {
                        auto depth_profile = profile.as<video_stream_profile>();
                        _metrics_model.update_stream_attributes(depth_profile.get_intrinsics(),
                            sub->s->as<depth_sensor>().get_depth_scale(), baseline_mm);

                        _metrics_model.update_roi_attributes({ int(depth_profile.width() * (0.5f - 0.5f*_roi_percent)),
                                                           int(depth_profile.height() * (0.5f - 0.5f*_roi_percent)),
                                                           int(depth_profile.width() * (0.5f + 0.5f*_roi_percent)),
                                                           int(depth_profile.height() * (0.5f + 0.5f*_roi_percent)) },
                                                            _roi_percent);
                    }
                }

                sub->algo_roi = _metrics_model.get_roi();
            }
        }

        // Reset tool state when the active camera is disconnected
        void tool_model::reset(ux_window& win)
        {
            try
            {
                _pipe.stop();
            }
            catch(...){}

            _device_in_use = false;
            _first_frame = true;
            win.reset();
        }

        bool metric_plot::has_trend(bool positive)
        {
            const auto window_size = 110;
            const auto curr_window = 10;
            auto best = ranges[GREEN_RANGE].x;
            if (ranges[RED_RANGE].x < ranges[GREEN_RANGE].x)
                best = ranges[GREEN_RANGE].y;

            auto min_val = 0.f;
            for (int i = 0; i < curr_window; i++)
            {
                auto val = fabs(best - _vals[(SIZE + _idx - i) % SIZE]);
                min_val += val / curr_window;
            }

            auto improved = 0;
            for (int i = curr_window; i <= window_size; i++)
            {
                auto val = fabs(best - _vals[(SIZE + _idx - i) % SIZE]);
                if (positive && min_val < val * 0.8) improved++;
                if (!positive && min_val * 0.8 > val) improved++;
            }
            return improved > window_size * 0.4;
        }

        void tool_model::snapshot_metrics()
        {
            if (auto ret = file_dialog_open(save_file, NULL, NULL, NULL))
            {
                std::string filename_base(ret);

                // Save depth/ir images
                for (auto const &stream : _viewer_model.streams)
                {
                    if (auto frame = stream.second.texture->get_last_frame().as<video_frame>())
                    {
                        // Use the colorizer to get an rgb image for the depth stream
                        if (frame.is<rs2::depth_frame>())
                        {
                            rs2::colorizer color_map;
                            frame = color_map(frame);
                        }

                        std::string stream_desc = rs2_stream_to_string(frame.get_profile().stream_type());
                        std::string filename = filename_base + "_" + stream_desc + ".png";
                        save_to_png(filename.data(), frame.get_width(), frame.get_height(), frame.get_bytes_per_pixel(), frame.get_data(), frame.get_width() * frame.get_bytes_per_pixel());

                        _viewer_model.not_model.add_notification({ to_string() << stream_desc << " snapshot was saved to " << filename,
                            0, RS2_LOG_SEVERITY_INFO,
                            RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                    }
                }

                // Export 3d view in PLY format
                frame ply_texture;
                if (_viewer_model.selected_tex_source_uid >= 0)
                {
                    ply_texture = _viewer_model.streams[_viewer_model.selected_tex_source_uid].texture->get_last_frame();
                    if (ply_texture)
                        _viewer_model.ppf.update_texture(ply_texture);
                }
                export_to_ply(filename_base + "_3d_mesh.ply", _viewer_model.not_model, _viewer_model.ppf.get_points(), ply_texture);

                // Save Metrics
                _metrics_model.serialize_to_csv(filename_base + "_depth_metrics.csv", capture_description());

                // Save camera configuration - supported when camera is in advanced mode only
                if (_device_model.get())
                {
                    if (auto adv = _device_model->dev.as<rs400::advanced_mode>())
                    {
                        std::string filename = filename_base + "_configuration.json";
                        std::ofstream out(filename);
                        out << adv.serialize_json();
                        out.close();
                    }
                }
            }
        }

        metrics_model::metrics_model() :
            _frame_queue(1),
            _depth_scale_units(0.f),
            _stereo_baseline_mm(0.f),
            _ground_truth_mm(0),
            _use_gt(false),
            _plane_fit(false),
            _roi_percentage(0.4f),
            _active(true)
        {
            _worker_thread = std::thread([this]() {
                while (_active)
                {
                    rs2::frame depth_frame;
                    if (!_frame_queue.poll_for_frame(&depth_frame))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }

                    auto profile = depth_frame.get_profile();
                    auto stream_type = profile.stream_type();

                    if (RS2_STREAM_DEPTH == stream_type)
                    {
                        float su = 0, baseline = -1.f;
                        rs2_intrinsics intrin{};
                        int gt_mm{};
                        bool plane_fit_set{};
                        region_of_interest roi{};

                        {
                            std::lock_guard<std::mutex> lock(_m);
                            su = _depth_scale_units;
                            baseline = _stereo_baseline_mm;
                            auto depth_profile = profile.as<video_stream_profile>();
                            intrin = depth_profile.get_intrinsics();
                            _depth_intrinsic = intrin;
                            _roi = { int(intrin.width * (0.5f - 0.5f*this->_roi_percentage)),
                                int(intrin.height * (0.5f - 0.5f*this->_roi_percentage)),
                                int(intrin.width * (0.5f + 0.5f*this->_roi_percentage)),
                                int(intrin.height * (0.5f + 0.5f*this->_roi_percentage)) };

                            roi = _roi;
                        }

                        std::tie(gt_mm, plane_fit_set) = get_inputs();
                        auto metrics = analyze_depth_image(depth_frame, su, baseline, &intrin, roi, gt_mm, plane_fit_set,callback);

                        {
                            std::lock_guard<std::mutex> lock(_m);
                            _latest_metrics = metrics;
                        }
                    }

                    // Artificially slow down the calculation, so even on small ROIs / resolutions
                    // the output is updated within reasonable interval (keeping it human readable)
                    std::this_thread::sleep_for(std::chrono::milliseconds(80));
                }
            });
        }

        metrics_model::~metrics_model()
        {
            _active = false;
            _worker_thread.join();
            reset();
        }

        std::shared_ptr<metric_plot> tool_model::make_metric(
            const std::string& name, float min, float max, const bool requires_plane_fit,
            const std::string& units,
            const std::string& description)
        {
            auto res = std::make_shared<metric_plot>(name, min, max, units, description, requires_plane_fit);
            _metrics_model.add_metric(res);
            return res;
        }

        rs2::device tool_model::get_active_device(void) const
        {
            rs2::device dev{};
            try
            {
                if (_pipe.get_active_profile())
                    dev = _pipe.get_active_profile().get_device();
            }
            catch(...){}

            return dev;
        }

        std::string tool_model::capture_description()
        {
            std::stringstream ss;
            ss << "Device Info:"
                << "\nType:," << _device_model->dev.get_info(RS2_CAMERA_INFO_NAME)
                << "\nHW Id:," << _device_model->dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID)
                << "\nSerial Num:," << _device_model->dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                << "\nFirmware Ver:," << _device_model->dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)
                << "\n\nStreaming profile:\nStream,Format,Resolution,FPS\n";

            for (auto& stream : _pipe.get_active_profile().get_streams())
            {
                auto vs = stream.as<video_stream_profile>();
                ss << vs.stream_name() << ","
                    << rs2_format_to_string(vs.format()) << ","
                    << vs.width() << "x" << vs.height() << "," << vs.fps() << "\n";
            }

            return ss.str();
        }

        void metric_plot::render(ux_window& win)
        {
            std::lock_guard<std::mutex> lock(_m);

            if (!_persistent_visibility.eval()) return;

            std::stringstream ss;
            auto val = _vals[(SIZE + _idx - 1) % SIZE];

            ss << _label << std::setprecision(2) << std::fixed << std::setw(3) << val << " " << _units;

            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, sensor_bg);

            auto range = get_range(val);
            if (range == GREEN_RANGE)
                ImGui::PushStyleColor(ImGuiCol_Text, green);
            else if (range == YELLOW_RANGE)
                ImGui::PushStyleColor(ImGuiCol_Text, yellow);
            else if (range == RED_RANGE)
                ImGui::PushStyleColor(ImGuiCol_Text, redish);
            else
                ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0xc3, 0xd5, 0xe5, 0xff));

            ImGui::PushFont(win.get_font());

            ImGui::PushStyleColor(ImGuiCol_Header, sensor_header_light_blue);

            const auto left_x = 295.f;
            const auto indicator_flicker_rate = 200;
            auto alpha_value = static_cast<float>(fabs(sin(_model_timer.elapsed_ms() / indicator_flicker_rate)));

            _trending_up.add_value(has_trend(true));
            _trending_down.add_value(has_trend(false));

            if (_trending_up.eval())
            {
                auto color = blend(green, alpha_value);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                auto col0 = ImGui::GetCursorPos();
                ImGui::SetCursorPosX(left_x);
                ImGui::PushFont(win.get_large_font());
                ImGui::Text(u8"\uf102");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("This metric shows positive trend");
                }
                ImGui::PopFont();
                ImGui::SameLine(); ImGui::SetCursorPos(col0);
                ImGui::PopStyleColor();
            }
            else if (_trending_down.eval())
            {
                auto color = blend(redish, alpha_value);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                auto col0 = ImGui::GetCursorPos();
                ImGui::SetCursorPosX(left_x);
                ImGui::PushFont(win.get_large_font());
                ImGui::Text(u8"\uf103");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("This metric shows negative trend");
                }
                ImGui::PopFont();
                ImGui::SameLine(); ImGui::SetCursorPos(col0);
                ImGui::PopStyleColor();
            }
            else
            {
                auto col0 = ImGui::GetCursorPos();
                ImGui::SetCursorPosX(left_x);
                ImGui::PushFont(win.get_large_font());
                ImGui::Text(" ");
                ImGui::PopFont();
                ImGui::SameLine(); ImGui::SetCursorPos(col0);
            }

            if (ImGui::TreeNode(_label.c_str(), "%s", ss.str().c_str()))
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, device_info_color);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
                std::string did = to_string() << _id << "-desc";
                ImVec2 desc_size = { 270, 50 };
                auto lines = std::count(_description.begin(), _description.end(), '\n') + 1;
                desc_size.y = lines * 20.f;
                ImGui::InputTextMultiline(did.c_str(), const_cast<char*>(_description.c_str()),
                    _description.size() + 1, desc_size, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor(3);

                ImGui::PlotLines(_id.c_str(), (float*)&_vals, int(SIZE), int(_idx), ss.str().c_str(), _min, _max, { 270, 50 });

                ImGui::TreePop();
            }

            ImGui::PopFont();
            ImGui::PopStyleColor(3);
        }

        // Display the latest metrics in the panel view
        void metrics_model::render(ux_window& win)
        {
            for (auto&& plot : _plots)
            {
                plot->render(win);
            }
        }

        void metrics_model::serialize_to_csv(const std::string& filename, const std::string& camera_info) const
        {
            std::ofstream csv;

            csv.open(filename);

            // Store the device info and the streaming profile details
            csv << camera_info;

            //Store metric environment
            csv << "\nEnvironment:\nPlane-Fit_distance_mm," << (_plane_fit ? std::to_string(_latest_metrics.distance) : "N/A") << std::endl;
            csv << "Ground-Truth_Distance_mm," << (_use_gt ? std::to_string(_ground_truth_mm ) : "N/A") << std::endl;

            // Generate columns header
            csv << "\nSample Id,Timestamp (ms),";
            for (auto&& plot : _plots)
            {
                csv << plot->_name << " " << plot->_units << ",";
            }
            csv << std::endl;

            // Populate metrics data using the fill-rate persistent metric as pivot
            for (size_t i = _plots[0]->_first_idx, rec = 0; i != _plots[0]->_idx; i = (++i) % metric_plot::SIZE)
            {
                csv << ++rec << "," << std::fixed << std::setprecision(4) << _plots[0]->_timestamps[i] << ",";
                for (auto&& plot : _plots)
                {
                    csv << plot->_vals[i] << ",";
                }
                csv << std::endl;
            }

            csv.close();
        }
    }
}
