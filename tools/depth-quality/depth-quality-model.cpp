#include "depth-quality-model.h"
#include "model-views.h"

using namespace std;

namespace rs2
{
    namespace depth_quality
    {
        //// A transient state required to acquire and distribute depth stream specifics among relevant parties
        /*void depth_profiler_model::configure_cam::update(depth_profiler_model& app)
        {
            auto dev = app._device_model.get()->dev;
            auto depth_scale_units = dev.first<depth_sensor>().get_depth_scale();
            auto profiles = app._device_model.get()->subdevices.front()->get_selected_profiles();
            auto depth_intrinsic = profiles.front().as<video_stream_profile>().get_intrinsics();

            {
                std::lock_guard<std::mutex> lock(app._m);
                app._metrics_model.update_stream_attributes(depth_intrinsic, depth_scale_units);
            }
        }*/

        //void depth_profiler_model::profile_metrics::update(depth_profiler_model& app)
        //{
        //    // Update active ROI for computation
        //    std::lock_guard<std::mutex> lock(app._m);
        //    // Evgeni - get actual ROI selection from stream_model
        //    //auto roi = app._viewer_model.streams[0].dev->roi_rect; 
        //    //app._metrics_model.update_frame_attributes({ (int)roi.x, (int)roi.y, (int)(roi.x+roi.w), (int)(roi.y + roi.h)});
        //    auto res = app._viewer_model.streams[0].size;
        //    region_of_interest roi{ int(res.x / 3), int(res.y / 3), int(res.x * 2 / 3), int(res.y * 2 / 3) };
        //    app._metrics_model.update_frame_attributes(roi);
        //}

        tool_model::tool_model()
            : _update_readonly_options_timer(std::chrono::seconds(6)), _roi_percent(0.33f)
        {
            _viewer_model.is_3d_view = true;
            _viewer_model.allow_3d_source_change = false;
            _viewer_model.allow_stream_close = false;
            _viewer_model.draw_plane = true;
        }

        void tool_model::start()
        {
            _pipe.enable_stream(RS2_STREAM_DEPTH, 0, 0, 0, RS2_FORMAT_Z16, 30);
            _pipe.enable_stream(RS2_STREAM_INFRARED, 1, 0, 0, RS2_FORMAT_Y8, 30);

            // Wait till a valid device is found
            _pipe.start();

            update_configuration();
        }

        void tool_model::render(ux_window& win)
        {
            win.begin_viewport();

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
            _viewer_model.roi_rect = _metrics.get_plane();
            _viewer_model.draw_viewport(viewer_rect, win, 1, _error_message);

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
                std::map<subdevice_model*, float> model_to_y;
                std::map<subdevice_model*, float> model_to_abs_y;
                auto windows_width = ImGui::GetContentRegionMax().x;

                _device_model->draw_controls(_viewer_model.panel_width, _viewer_model.panel_y,
                    win.get_font(), win.get_large_font(), win.get_mouse(),
                    _error_message, device_to_remove, _viewer_model, windows_width,
                    _update_readonly_options_timer,
                    model_to_y, model_to_abs_y);


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
                                auto config = _depth_sensor_model->get_selected_profiles().front().as<video_stream_profile>();

                                _pipe.stop();
                                _pipe.disable_all();

                                _pipe.enable_stream(RS2_STREAM_DEPTH, 0, config.width(), config.height(), RS2_FORMAT_Z16, config.fps());
                                _pipe.enable_stream(RS2_STREAM_INFRARED, 1, config.width(), config.height(), RS2_FORMAT_Y8, config.fps());

                                // Wait till a valid device is found
                                _pipe.start();

                                update_configuration();
                            }
                            else
                            {
                                _error_message = "Selected configuration is not supported!";
                            }
                        }

                        auto col0 = ImGui::GetCursorPosX();
                        auto col1 = 145.f;

                        ImGui::Text("Region of Interest:");
                        ImGui::SameLine(); ImGui::SetCursorPosX(col1);

                        ImGui::PushItemWidth(-1);
                        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1,1,1,1 });

                        static std::vector<std::string> items{ "66%", "33%", "11%" };
                        if (draw_combo_box("##ROI Percent", items, _roi_combo_index))
                        {
                            if (_roi_combo_index == 0) _roi_percent = 0.66f;
                            else if (_roi_combo_index == 1) _roi_percent = 0.33f;
                            else if (_roi_combo_index == 2) _roi_percent = 0.11f;
                            update_configuration();
                        }

                        ImGui::PopStyleColor();
                        ImGui::PopItemWidth();
                        ImGui::SetCursorPosX(col0);

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                        ImGui::TreePop();
                    }

                    ImGui::PopStyleVar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });

                    if (ImGui::TreeNodeEx("Metrics", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleVar();
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, 2 });

                        _metrics.render(win);

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

            frameset f;
            if (_pipe.poll_for_frames(&f))
            {
                for (auto&& frame : f)
                {
                    if (frame.is<depth_frame>() && !_viewer_model.paused)
                    {
                        _metrics.begin_process_frame(frame);
                    }
                    _viewer_model.upload_frame(std::move(frame));
                }
            }

            _viewer_model.gc_streams();
            _viewer_model.popup_if_error(win.get_font(), _error_message);
        }

        void tool_model::update_configuration()
        {
            auto old_res = -1;
            if (_depth_sensor_model)
                old_res = _depth_sensor_model->selected_res_id;

            auto dev = _pipe.get_device();
            _device_model = std::shared_ptr<rs2::device_model>(new device_model(dev, _error_message, _viewer_model));
            _device_model->allow_remove = false;
            _device_model->show_depth_only = true;
            _device_model->show_stream_selection = false;
            _depth_sensor_model = std::shared_ptr<rs2::subdevice_model>(
                new subdevice_model(dev, dev.first<depth_sensor>(), _error_message));
            _depth_sensor_model->draw_streams_selector = false;
            _depth_sensor_model->draw_fps_selector = false;
            
            if (old_res != -1) _depth_sensor_model->selected_res_id = old_res;

            // Connect the device_model to the viewer_model
            for (auto&& sub : _device_model.get()->subdevices)
            {
                if (!sub->s.is<depth_sensor>()) continue;

                sub->show_algo_roi = true;
                auto profiles = _pipe.get_active_streams();
                sub->streaming = true;      // The streaming activated externally to the device_model
                for (auto&& profile : profiles)
                {
                    _viewer_model.streams[profile.unique_id()].dev = sub;

                    if (profile.stream_type() == RS2_STREAM_DEPTH)
                    {
                        auto depth_profile = profile.as<video_stream_profile>();
                        _metrics.update_stream_attributes(depth_profile.get_intrinsics(),
                                                          sub->s.as<depth_sensor>().get_depth_scale());

                        _metrics.update_frame_attributes({ int(depth_profile.width() * (0.5f - 0.5f*_roi_percent)), 
                                                           int(depth_profile.height() * (0.5f - 0.5f*_roi_percent)),
                                                           int(depth_profile.width() * (0.5f + 0.5f*_roi_percent)),
                                                           int(depth_profile.height() * (0.5f + 0.5f*_roi_percent)) });
                    }
                }

                sub->algo_roi = _metrics.get_roi();
            }
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
                auto val = abs(best - vals[(SIZE + idx - i) % SIZE]);
                min_val += val / curr_window;
            }

            auto improved = 0;
            for (int i = curr_window; i <= window_size; i++)
            {
                auto val = abs(best - vals[(SIZE + idx - i) % SIZE]);
                if (positive && min_val < val * 0.8) improved++;
                if (!positive && min_val * 0.8 > val) improved++;
            }
            return improved > window_size * 0.4;
        }

        //void depth_profiler_model::upload(rs2::frameset &frameset)
        //{
        //    // Upload new frames for rendering
        //    for (size_t i = 0; i < frameset.size(); i++)
        //        _viewer_model.upload_frame(frameset[i]);
        //}

        metrics_model::metrics_model() :
            _frame_queue(1),
            _depth_scale_units(0.f), _active(true),
            _avg_plot("Average Error", 0, 10, { 270, 50 }, " (mm)"),
            _std_plot("std(Error)", 0, 10, { 270, 50 }, " (mm)"),
            _fill_plot("Fill-Rate", 0, 100, { 270, 50 }, "%%"),
            _dist_plot("Distance", 0, 5, { 270, 50 }, " (m)"),
            _angle_plot("Angle", 0, 180, { 270, 50 }, " (deg)"),
            _out_plot("Outliers", 0, 100, { 270, 50 }, "%%")
        {
            _avg_plot.ranges[metric_plot::GREEN_RANGE] = { 0.f, 1.f };
            _avg_plot.ranges[metric_plot::YELLOW_RANGE] = { 0.f, 7.f };
            _avg_plot.ranges[metric_plot::RED_RANGE] = { 0.f, 1000.f };

            _avg_plot.description = "Average Distance from Plane Fit\nThis metric approximates a plane within\nthe ROI and calculates the average\ndistance of points in the ROI\nfrom that plane, in mm";

            _std_plot.ranges[metric_plot::GREEN_RANGE] = { 0.f, 1.f };
            _std_plot.ranges[metric_plot::YELLOW_RANGE] = { 0.f, 7.f };
            _std_plot.ranges[metric_plot::RED_RANGE] = { 0.f, 1000.f };

            _std_plot.description = "Standard Deviation from Plane Fit\nThis metric approximates a plane within\nthe ROI and calculates the\nstandard deviation of distances\nof points in the ROI from that plane";

            _fill_plot.ranges[metric_plot::GREEN_RANGE] = { 90.f, 100.f };
            _fill_plot.ranges[metric_plot::YELLOW_RANGE] = { 50.f, 100.f };
            _fill_plot.ranges[metric_plot::RED_RANGE] = { 0.f, 100.f };

            _fill_plot.description = "Fill Rate\nPercentage of pixels with valid depth values\nout of all pixels within the ROI";

            _dist_plot.ranges[metric_plot::GREEN_RANGE] = { 0.f, 2.f };
            _dist_plot.ranges[metric_plot::YELLOW_RANGE] = { 0.f, 3.f };
            _dist_plot.ranges[metric_plot::RED_RANGE] = { 0.f, 7.f };

            _dist_plot.description = "Approximate Distance\nWhen facing a flat wall at right angle\nthis metric estimates the distance\nin meters to that wall";

            _angle_plot.ranges[metric_plot::GREEN_RANGE] = { -5.f, 5.f };
            _angle_plot.ranges[metric_plot::YELLOW_RANGE] = { -10.f, 10.f };
            _angle_plot.ranges[metric_plot::RED_RANGE] = { -100.f, 100.f };

            _angle_plot.description = "Wall Angle\nWhen facing a flat wall this metric\nestimates the angle to the wall.";

            _out_plot.ranges[metric_plot::GREEN_RANGE] = { 0.f, 5.f };
            _out_plot.ranges[metric_plot::YELLOW_RANGE] = { 0.f, 20.f };
            _out_plot.ranges[metric_plot::RED_RANGE] = { 0.f, 100.f };

            _out_plot.description = "Outliers Percentage\nMeasures the percentage of pixels\nthat do not fit in with\nthe gaussian distribution";

            _worker_thread = std::thread([this]() {
                while (_active)
                {
                    rs2::frame depth_frame;
                    if (!_frame_queue.poll_for_frame(&depth_frame))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }


                    auto stream_type = depth_frame.get_profile().stream_type();

                    if (RS2_STREAM_DEPTH == stream_type)
                    {
                        float su = 0;
                        rs2_intrinsics intrin;
                        region_of_interest roi;
                        {
                            std::lock_guard<std::mutex> lock(_m);
                            su = _depth_scale_units;
                            intrin = _depth_intrinsic;
                            roi = _roi;
                        }

                        auto metrics = analyze_depth_image(depth_frame, su, &intrin, _roi);

                        {
                            std::lock_guard<std::mutex> lock(_m);
                            _latest_metrics = metrics;
                        }
                        cout << "Depth Average distance is : " << _latest_metrics.depth.avg_dist << endl;
                        cout << "Standard_deviation is : " << _latest_metrics.depth.std_dev << endl;
                        cout << "Fillrate is : " << _latest_metrics.non_null_pct << endl;
                    }
                    else
                    {
                        std::cout << __FUNCTION__ << " : unexpected frame type received - " << rs2_stream_to_string(stream_type) << std::endl;
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

            rs2::frame f;
            while (_frame_queue.poll_for_frame(&f));
        }

        void metric_plot::render(ux_window& win)
        {
            std::stringstream ss;
            auto val = vals[(SIZE + idx - 1) % SIZE];
            ss << label << val << tail;

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

            const auto left_x = 295;
            const auto indicator_flicker_rate = 150;
            if (has_trend(true))
            {
                auto color = blend(green, abs(sin(model_timer.elapsed_ms() / indicator_flicker_rate)));
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
            else if (has_trend(false))
            {
                auto color = blend(redish, abs(sin(model_timer.elapsed_ms() / indicator_flicker_rate)));
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

            if (ImGui::TreeNode(label.c_str(), ss.str().c_str()))
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, device_info_color);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
                std::string did = to_string() << id << "-desc";
                auto desc_size = size;
                auto lines = std::count(description.begin(), description.end(), '\n') + 1;
                desc_size.y = lines * 20;
                ImGui::InputTextMultiline(did.c_str(), const_cast<char*>(description.c_str()),
                    description.size() + 1, desc_size, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor(3);

                ImGui::PlotLines(id.c_str(), vals, 100, idx, ss.str().c_str(), min, max, size);
                ImGui::TreePop();
            }

            ImGui::PopFont();
            ImGui::PopStyleColor(3);
        }

        void metrics_model::render(ux_window& win)
        {
            std::lock_guard<std::mutex> lock(_m);
            metrics data = _latest_metrics.planar;

            _avg_plot.add_value(data.avg_dist);
            _std_plot.add_value(data.std_dev);
            _fill_plot.add_value(_latest_metrics.non_null_pct);
            _dist_plot.add_value(data.distance);
            _angle_plot.add_value(data.angle);
            _out_plot.add_value(data.outlier_pct);

            _avg_plot.render(win);
            _std_plot.render(win);
            _fill_plot.render(win);
            _dist_plot.render(win);
            _angle_plot.render(win);
            _out_plot.render(win);

            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            if (ImGui::Button("Save Report", { 290, 25 }))
            {

            }
            ImGui::PopStyleColor();
        }
    }
}
