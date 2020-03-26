// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <glad/glad.h>
#include "on-chip-calib.h"

#include <map>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <model-views.h>
#include <viewer.h>

#include "../tools/depth-quality/depth-metrics.h"

namespace rs2
{
    void on_chip_calib_manager::stop_viewer(invoker invoke)
    {
        try
        {
            auto profiles = _sub->get_selected_profiles();

            invoke([&](){
                // Stop viewer UI
                _sub->stop(_viewer);
            });

            // Wait until frames from all active profiles stop arriving
            bool frame_arrived = false;
            while (frame_arrived && _viewer.streams.size())
            {
                for (auto&& stream : _viewer.streams)
                {
                    if (std::find(profiles.begin(), profiles.end(),
                        stream.second.original_profile) != profiles.end())
                    {
                        auto now = std::chrono::high_resolution_clock::now();
                        if (now - stream.second.last_frame > std::chrono::milliseconds(200))
                            frame_arrived = false;
                    }
                    else frame_arrived = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        catch (...) {}
    }

    // Wait for next depth frame and return it
    rs2::depth_frame on_chip_calib_manager::fetch_depth_frame(invoker invoke)
    {
        auto profiles = _sub->get_selected_profiles();
        bool frame_arrived = false;
        rs2::depth_frame res = rs2::frame{};
        while (!frame_arrived)
        {
            for (auto&& stream : _viewer.streams)
            {
                if (std::find(profiles.begin(), profiles.end(),
                    stream.second.original_profile) != profiles.end())
                {
                    auto now = std::chrono::high_resolution_clock::now();
                    if (now - stream.second.last_frame < std::chrono::milliseconds(100))
                    {
                        if (auto f = stream.second.texture->get_last_frame(false).as<rs2::depth_frame>())
                        {
                            frame_arrived = true;
                            res = f;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return res;
    }

    void on_chip_calib_manager::start_viewer(int w, int h, int fps, invoker invoke)
    {
        try
        {
            if (_ui) _sub->ui = *_ui; // Save previous configuration

            // Select only depth stream
            _sub->ui.selected_format_id.clear();
            _sub->ui.selected_format_id[RS2_STREAM_DEPTH] = 0;

            // Select FPS value
            for (int i = 0; i < _sub->shared_fps_values.size(); i++)
            {
                if (_sub->shared_fps_values[i] == fps)
                    _sub->ui.selected_shared_fps_id = i;
            }

            // Select Resolution
            for (int i = 0; i < _sub->res_values.size(); i++)
            {
                auto kvp = _sub->res_values[i];
                if (kvp.first == w && kvp.second == h)
                    _sub->ui.selected_res_id = i;
            }

            // If not supported, try WxHx30
            if (!_sub->is_selected_combination_supported())
            {
                for (int i = 0; i < _sub->shared_fps_values.size(); i++)
                {
                    //if (_sub->shared_fps_values[i] == 30)
                        _sub->ui.selected_shared_fps_id = i;
                        if (_sub->is_selected_combination_supported()) break;
                }

                // If still not supported, try VGA30
                if (!_sub->is_selected_combination_supported())
                {
                    for (int i = 0; i < _sub->res_values.size(); i++)
                    {
                        auto kvp = _sub->res_values[i];
                        if (kvp.first == 640 && kvp.second == 480)
                            _sub->ui.selected_res_id = i;
                    }
                }
            }

            auto profiles = _sub->get_selected_profiles();

            invoke([&](){
                if (!_model.dev_syncer)
                    _model.dev_syncer = _viewer.syncer->create_syncer();

                // Start streaming
                _sub->play(profiles, _viewer, _model.dev_syncer);
                for (auto&& profile : profiles)
                {
                    _viewer.begin_stream(_sub, profile);
                }
            });

            // Wait for frames to arrive
            bool frame_arrived = false;
            int count = 0;
            while (!frame_arrived && count++ < 100)
            {
                for (auto&& stream : _viewer.streams)
                {
                    if (std::find(profiles.begin(), profiles.end(),
                        stream.second.original_profile) != profiles.end())
                    {
                        auto now = std::chrono::high_resolution_clock::now();
                        if (now - stream.second.last_frame < std::chrono::milliseconds(100))
                            frame_arrived = true;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        catch (...) {}
    }

    std::pair<float, float> on_chip_calib_manager::get_metric(bool use_new)
    {
        return _metrics[use_new ? 1 : 0];
    }

    std::pair<float, float> on_chip_calib_manager::get_depth_metrics(invoker invoke)
    {
        using namespace depth_quality;

        auto f = fetch_depth_frame(invoke);
        auto sensor = _sub->s->as<rs2::depth_stereo_sensor>();
        auto intr = f.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
        rs2::region_of_interest roi { (int)(f.get_width() * 0.45f), (int)(f.get_height()  * 0.45f), 
                                      (int)(f.get_width() * 0.55f), (int)(f.get_height() * 0.55f) };
        std::vector<single_metric_data> v;

        std::vector<float> fill_rates;
        std::vector<float> rmses;

        auto show_plane = _viewer.draw_plane;

        auto on_frame = [sensor, &fill_rates, &rmses, this](
            const std::vector<rs2::float3>& points,
            const plane p,
            const rs2::region_of_interest roi,
            const float baseline_mm,
            const float focal_length_pixels,
            const int ground_thruth_mm,
            const bool plane_fit,
            const float plane_fit_to_ground_truth_mm,
            const float distance_mm,
            bool record,
            std::vector<single_metric_data>& samples)
        {
            float TO_METERS = sensor.get_depth_scale();
            static const float TO_MM = 1000.f;
            static const float TO_PERCENT = 100.f;

            // Calculate fill rate relative to the ROI
            auto fill_rate = points.size() / float((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * TO_PERCENT;
            fill_rates.push_back(fill_rate);

            if (!plane_fit) return;

            std::vector<rs2::float3> points_set = points;
            std::vector<float> distances;

            // Reserve memory for the data
            distances.reserve(points.size());

            // Convert Z values into Depth values by aligning the Fitted plane with the Ground Truth (GT) plane
            // Calculate distance and disparity of Z values to the fitted plane.
            // Use the rotated plane fit to calculate GT errors
            for (auto point : points_set)
            {
                // Find distance from point to the reconstructed plane
                auto dist2plane = p.a*point.x + p.b*point.y + p.c*point.z + p.d;
                // Project the point to plane in 3D and find distance to the intersection point
                rs2::float3 plane_intersect = { float(point.x - dist2plane*p.a),
                    float(point.y - dist2plane*p.b),
                    float(point.z - dist2plane*p.c) };

                // Store distance, disparity and gt- error
                distances.push_back(dist2plane * TO_MM);
            }

            // Remove outliers [below 1% and above 99%)
            std::sort(points_set.begin(), points_set.end(), [](const rs2::float3& a, const rs2::float3& b) { return a.z < b.z; });
            size_t outliers = points_set.size() / 50;
            points_set.erase(points_set.begin(), points_set.begin() + outliers); // crop min 0.5% of the dataset
            points_set.resize(points_set.size() - outliers); // crop max 0.5% of the dataset

            // Calculate Plane Fit RMS  (Spatial Noise) mm
            double plane_fit_err_sqr_sum = std::inner_product(distances.begin(), distances.end(), distances.begin(), 0.);
            auto rms_error_val = static_cast<float>(std::sqrt(plane_fit_err_sqr_sum / distances.size()));
            auto rms_error_val_per = TO_PERCENT * (rms_error_val / distance_mm);
            rmses.push_back(rms_error_val_per);
        };

        auto rms_std = 1000.f;
        auto new_rms_std = rms_std;
        auto count = 0;

        // Capture metrics on bundles of 31 frame
        // Repeat until get "decent" bundle or reach 10 sec
        do
        {
            rms_std = new_rms_std;

            rmses.clear();

            for (int i = 0; i < 31; i++)
            {
                f = fetch_depth_frame(invoke);
                auto res = depth_quality::analyze_depth_image(f, sensor.get_depth_scale(), sensor.get_stereo_baseline(),
                    &intr, roi, 0, true, v, false, on_frame);

                _viewer.draw_plane = true;
                _viewer.roi_rect = res.plane_corners;
            }

            auto rmses_sum_sqr = std::inner_product(rmses.begin(), rmses.end(), rmses.begin(), 0.);
            new_rms_std = static_cast<float>(std::sqrt(rmses_sum_sqr / rmses.size()));
        } while ((new_rms_std < rms_std * 0.8f && new_rms_std > 10.f) && count++ < 10);

        std::sort(fill_rates.begin(), fill_rates.end());
        std::sort(rmses.begin(), rmses.end());

        float median_fill_rate, median_rms;
        if (fill_rates.empty())
            median_fill_rate = 0;
        else
            median_fill_rate = fill_rates[fill_rates.size() / 2];
        if (rmses.empty())
            median_rms = 0;
        else
            median_rms = rmses[rmses.size() / 2];

        _viewer.draw_plane = show_plane;

        return { median_fill_rate, median_rms };
    }

    std::vector<uint8_t> on_chip_calib_manager::safe_send_command(
        const std::vector<uint8_t>& cmd, const std::string& name)
    {
        auto dp = _dev.as<debug_protocol>();
        if (!dp) throw std::runtime_error("Device does not support debug protocol!");

        auto res = dp.send_and_receive_raw_data(cmd);

        if (res.size() < sizeof(int32_t)) throw std::runtime_error(to_string() << "Not enough data from " << name << "!");
        auto return_code = *((int32_t*)res.data());
        if (return_code < 0)  throw std::runtime_error(to_string() << "Firmware error (" << return_code << ") from " << name << "!");

        return res;
    }

    void on_chip_calib_manager::update_last_used()
    {
        time_t rawtime;
        time(&rawtime);
        std::string id = to_string() << configurations::viewer::last_calib_notice << "." << _sub->s->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        config_file::instance().set(id.c_str(), (long long)rawtime);
    }

    void on_chip_calib_manager::calibrate()
    {
        std::stringstream ss;
        ss << "{\n \"speed\":" << speed <<
               ",\n \"average step count\":" << average_step_count <<
               ",\n \"scan parameter\":" << (intrinsic_scan ? 0 : 1) <<
               ",\n \"step count\":" << step_count <<
               ",\n \"apply preset\":" << (apply_preset ? 1 : 0) <<
               ",\n \"accuracy\":" << accuracy <<"}";

        std::string json = ss.str();
        

        auto calib_dev = _dev.as<auto_calibrated_device>();
        if (tare)
            _new_calib = calib_dev.run_tare_calibration(ground_truth, json, [&](const float progress) {_progress = progress;}, 5000);
        else
            _new_calib = calib_dev.run_on_chip_calibration(json, &_health, [&](const float progress) {_progress = progress;}, 5000);
    }

    void on_chip_calib_manager::process_flow(std::function<void()> cleanup, 
        invoker invoke)
    {
        update_last_used();

        log(to_string() << "Starting calibration at speed " << speed);

        _in_3d_view = _viewer.is_3d_view;
        _viewer.is_3d_view = true;

        config_file::instance().set(configurations::viewer::ground_truth_r, ground_truth);


        auto calib_dev = _dev.as<auto_calibrated_device>();
        _old_calib = calib_dev.get_calibration_table();

        _was_streaming = _sub->streaming;
        _synchronized = _viewer.synchronization_enable.load();
        _post_processing = _sub->post_processing_enabled;
        _sub->post_processing_enabled = false;
        _viewer.synchronization_enable = false;

        _restored = false;

        if (!_was_streaming) 
        {
            start_viewer(0,0,0, invoke);
        }

        // Capture metrics before
        auto metrics_before = get_depth_metrics(invoke);
        _metrics.push_back(metrics_before);
       
        stop_viewer(invoke);

        _ui = std::make_shared<subdevice_ui_selection>(_sub->ui);
        
        // Switch into special Auto-Calibration mode
        start_viewer(256, 144, 90, invoke);

        calibrate();

        log(to_string() << "Calibration completed, health factor = " << _health);

        stop_viewer(invoke);

        start_viewer(0, 0, 0, invoke); // Start with default settings

        // Make new calibration active
        apply_calib(true);

        // Capture metrics after
        auto metrics_after = get_depth_metrics(invoke);
        _metrics.push_back(metrics_after);

        _progress = 100;

        _done = true;
    }

    void on_chip_calib_manager::restore_workspace(invoker invoke)
    {
        try
        {
            if (_restored) return;

            _viewer.is_3d_view = _in_3d_view;

            _viewer.ground_truth_r = ground_truth;
            config_file::instance().set(configurations::viewer::ground_truth_r, ground_truth);

            _viewer.synchronization_enable = _synchronized;

            stop_viewer(invoke);

            if (_ui.get())
            {
                _sub->ui = *_ui;
                _ui.reset();
            }

            _sub->post_processing_enabled = _post_processing;

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            if (_was_streaming) start_viewer(0, 0, 0, invoke);

            _restored = true;
        }
        catch (...) {}
    }

    void on_chip_calib_manager::keep()
    {
        // Write new calibration using SETINITCAL command
        auto calib_dev = _dev.as<auto_calibrated_device>();
        calib_dev.write_calibration();

    }

    void on_chip_calib_manager::apply_calib(bool use_new)
    {
        auto calib_dev = _dev.as<auto_calibrated_device>();
        calib_dev.set_calibration_table(use_new ? _new_calib : _old_calib);
    }

    void autocalib_notification_model::draw_dismiss(ux_window& win, int x, int y)
    {
        using namespace std;
        using namespace chrono;

        auto health = get_manager().get_health();
        auto recommend_keep = fabs(health) > 0.25f;  
        if (!recommend_keep && update_state == RS2_CALIB_STATE_CALIB_COMPLETE && !get_manager().tare)
        {
            auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

            ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
            notification_model::draw_dismiss(win, x, y);
            ImGui::PopStyleColor(2);
        }
        else
            notification_model::draw_dismiss(win, x, y);
    }

    void autocalib_notification_model::draw_intrinsic_extrinsic(int x, int y)
    {
        bool intrinsic = get_manager().intrinsic_scan;
        bool extrinsic = !intrinsic;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 35 + ImGui::GetTextLineHeightWithSpacing()) });

        std::string id = to_string() << "##Intrinsic_" << index;
        if (ImGui::Checkbox("Intrinsic", &intrinsic))
        {
            extrinsic = !intrinsic;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", "Calibrate intrinsic parameters of the camera");
        }
        ImGui::SetCursorScreenPos({ float(x + 135), float(y + 35 + ImGui::GetTextLineHeightWithSpacing()) });

        id = to_string() << "##Intrinsic_" << index;

        if (ImGui::Checkbox("Extrinsic", &extrinsic))
        {
            intrinsic = !extrinsic;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", "Calibrate extrinsic parameters between left and right cameras");
        }

        get_manager().intrinsic_scan = intrinsic;
    }

    void autocalib_notification_model::draw_content(ux_window& win, int x, int y, float t, std::string& error_message)
    {
        using namespace std;
        using namespace chrono;

        const auto bar_width = width - 115;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
        { float(x + width), float(y + 25) }, ImColor(shadow));

        if (update_state != RS2_CALIB_STATE_COMPLETE)
        {
            if (update_state == RS2_CALIB_STATE_INITIAL_PROMPT)
                ImGui::Text("%s", "Calibration Health-Check");
            else if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS ||
                     update_state == RS2_CALIB_STATE_CALIB_COMPLETE ||
                     update_state == RS2_CALIB_STATE_SELF_INPUT)
                ImGui::Text("%s", "On-Chip Calibration");
            else if (update_state == RS2_CALIB_STATE_TARE_INPUT || update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                ImGui::Text("%s", "Tare Calibration");
            if (update_state == RS2_CALIB_STATE_FAILED)
                ImGui::Text("%s", "Calibration Failed");

            if (update_state == RS2_CALIB_STATE_TARE_INPUT || update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                ImGui::SetCursorScreenPos({ float(x + width - 30), float(y) });
            else if (update_state == RS2_CALIB_STATE_FAILED)
                ImGui::SetCursorScreenPos({ float(x + 2), float(y + 27) });
            else
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 27) });

            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1. - t));

            if (update_state == RS2_CALIB_STATE_INITIAL_PROMPT)
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

                ImGui::Text("%s", "Following devices support On-Chip Calibration:");
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 47) });

                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::Text("%s", message.c_str());
                ImGui::PopStyleColor();

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 65) });
                ImGui::Text("%s", "Run quick calibration Health-Check? (~30 sec)");
            }
            else if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS)
            {
                enable_dismiss = false;
                ImGui::Text("%s", "Camera is being calibrated...\nKeep the camera stationary pointing at a wall");
            }
            else if (update_state == RS2_CALIB_STATE_TARE_INPUT || update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, update_state != RS2_CALIB_STATE_TARE_INPUT_ADVANCED ? light_grey : light_blue);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, update_state != RS2_CALIB_STATE_TARE_INPUT_ADVANCED ? light_grey : light_blue);

                if (ImGui::Button(u8"\uf0d7"))
                {
                    if (update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                        update_state = RS2_CALIB_STATE_TARE_INPUT;
                    else
                        update_state = RS2_CALIB_STATE_TARE_INPUT_ADVANCED;
                }

                if (ImGui::IsItemHovered())
                {
                    if(update_state == RS2_CALIB_STATE_TARE_INPUT)
                        ImGui::SetTooltip("%s", "More Options...");
                    else
                        ImGui::SetTooltip("%s", "Less Options...");
                }

                ImGui::PopStyleColor(2);
                if(update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                {
                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                    ImGui::Text("%s", "Avg Step Count:");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "Number of frames to average, Min = 1, Max = 30, Default = 20"); 
                    }
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });

                    std::string id = to_string() << "##avg_step_count_" << index;
                    ImGui::PushItemWidth(width - 145);
                    ImGui::SliderInt(id.c_str(), &get_manager().average_step_count, 1, 30);
                    ImGui::PopItemWidth();

                    //-------------------------

                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 38 + ImGui::GetTextLineHeightWithSpacing()) });
                    ImGui::Text("%s", "Step Count:");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "Max iteration steps, Min = 5, Max = 30, Default = 20");
                    }
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 35 + ImGui::GetTextLineHeightWithSpacing()) });

                    id = to_string() << "##step_count_" << index;

                    ImGui::PushItemWidth(width - 145);
                    ImGui::SliderInt(id.c_str(), &get_manager().step_count, 1, 30);
                    ImGui::PopItemWidth();

                    //-------------------------

                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 43 + 2 * ImGui::GetTextLineHeightWithSpacing()) });
                    ImGui::Text("%s", "Accuracy:");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "Subpixel accuracy level, Very high = 0 (0.025%), High = 1 (0.05%), Medium = 2 (0.1%), Low = 3 (0.2%), Default = Very high (0.025%)");
                    }

                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 40 + 2 * ImGui::GetTextLineHeightWithSpacing()) });

                    id = to_string() << "##accuracy_" << index;

                    std::vector<std::string> vals{ "Very High", "High", "Medium", "Low" };
                    std::vector<const char*> vals_cstr;
                    for (auto&& s : vals) vals_cstr.push_back(s.c_str());

                    ImGui::PushItemWidth(width - 145);
                    ImGui::Combo(id.c_str(), &get_manager().accuracy, vals_cstr.data(), vals.size());
                   
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 35 + ImGui::GetTextLineHeightWithSpacing()) });

                    ImGui::PopItemWidth();

                    draw_intrinsic_extrinsic(x, y + 3 * ImGui::GetTextLineHeightWithSpacing() - 10);

                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 52 + 4 * ImGui::GetTextLineHeightWithSpacing()) });
                    id = to_string() << "Apply High-Accuracy Preset##apply_preset_" << index;
                    ImGui::Checkbox(id.c_str(), &get_manager().apply_preset);
                }

                if (update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                {
                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 60 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                    ImGui::Text("%s", "Ground Truth(mm):");
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 58 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                }
                else
                {
                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                    ImGui::Text("%s", "Ground Truth(mm):");
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Tare depth in 1 / 100 of depth unit, Min = 2500, Max = 2000000");
                }
                

                //ImGui::SetCursorScreenPos({ float(x + 135), float(y + 45 + 3 * ImGui::GetTextLineHeightWithSpacing()) });

                std::string id = to_string() << "##ground_truth_for_tare" << index;

                get_manager().ground_truth = config_file::instance().get_or_default(configurations::viewer::ground_truth_r,2500);

                std::string gt = to_string() << get_manager().ground_truth;
                const int MAX_SIZE = 256;
                char buff[MAX_SIZE];
                memcpy(buff, gt.c_str(), gt.size() + 1);

                ImGui::PushItemWidth(width - 145);
                if (ImGui::InputText(id.c_str(), buff, std::max((int)gt.size() + 1, 10)))
                {
                    std::stringstream ss;
                    ss << buff;
                    ss >> get_manager().ground_truth;
                }
                ImGui::PopItemWidth();

                config_file::instance().set(configurations::viewer::ground_truth_r, get_manager().ground_truth);

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                std::string button_name = to_string() << "Calibrate" << "##tare" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a){ a(); });
                    get_manager().reset();
                    get_manager().tare = true;
                    get_manager().start(shared_from_this());
                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Begin Tare Calibration");
                }
            }
            else if (update_state == RS2_CALIB_STATE_SELF_INPUT)
            {
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                ImGui::Text("%s", "Speed:");

                ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });

                std::string id = to_string() << "##speed_" << index;

                std::vector<std::string> vals{ "Very Fast", "Fast", "Medium", "Slow", "White Wall" };
                std::vector<const char*> vals_cstr;
                for (auto&& s : vals) vals_cstr.push_back(s.c_str());

                ImGui::PushItemWidth(width - 145);

                ImGui::Combo(id.c_str(), &get_manager().speed, vals_cstr.data(), vals.size());
                ImGui::PopItemWidth();

                draw_intrinsic_extrinsic(x, y);

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                std::string button_name = to_string() << "Calibrate" << "##self" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
                    get_manager().tare = false;
                    get_manager().start(shared_from_this());
                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Begin On-Chip Calibration");
                }
            }
            else if (update_state == RS2_CALIB_STATE_FAILED)
            {
                ImGui::Text("%s", _error_message.c_str());

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                ImGui::PushStyleColor(ImGuiCol_Button, saturate(redish, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(redish, 1.5f));

                std::string button_name = to_string() << "Retry" << "##retry" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a){ a(); });
                    get_manager().reset();
                    get_manager().start(shared_from_this());
                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Retry on-chip calibration process");
                }
            }
            else if (update_state == RS2_CALIB_STATE_CALIB_COMPLETE)
            {
                auto health = get_manager().get_health();

                auto recommend_keep = fabs(health) > 0.25f;

                ImGui::SetCursorScreenPos({ float(x + 15), float(y + 33) });

                if (get_manager().tare)
                {
                    ImGui::Text("%s", "Tare calibration complete:");
                }
                else
                {
                    ImGui::Text("%s", "Health-Check: ");

                    std::stringstream ss; ss << std::fixed << std::setprecision(2) << health;
                    auto health_str = ss.str();

                    std::string text_name = to_string() << "##notification_text_" << index;
                    
                    ImGui::SetCursorScreenPos({ float(x + 136), float(y + 30) });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(health_str.c_str()),
                        strlen(health_str.c_str()) + 1, { 50, 
                                                        ImGui::GetTextLineHeight() + 6 },
                        ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    ImGui::SetCursorScreenPos({ float(x + 172), float(y + 33) });

                    if (!recommend_keep)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                        ImGui::Text("%s", "(Good)");
                    }
                    else if (fabs(health) < 0.75f)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, yellowish);
                        ImGui::Text("%s", "(Can be Improved)");
                    }
                    else  
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, redish);
                        ImGui::Text("%s", "(Requires Calibration)");
                    }
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "Calibration Health-Check captures how far camera calibration is from the optimal one\n"
                            "[0, 0.25) - Good\n"
                            "[0.25, 0.75) - Can be Improved\n"
                            "[0.75, ) - Requires Calibration");
                    }
                }

                auto old_fr = get_manager().get_metric(false).first;
                auto new_fr = get_manager().get_metric(true).first;

                auto old_rms = fabs(get_manager().get_metric(false).second);
                auto new_rms = fabs(get_manager().get_metric(true).second);

                auto fr_improvement = 100.f * ((new_fr - old_fr) / old_fr);
                auto rms_improvement = 100.f * ((old_rms - new_rms) / old_rms);

                std::string old_units = "mm";
                if (old_rms > 10.f)
                {
                    old_rms /= 10.f;
                    old_units = "cm";
                }
                std::string new_units = "mm";
                if (new_rms > 10.f)
                {
                    new_rms /= 10.f;
                    new_units = "cm";
                }

                // NOTE: Disabling metrics temporarily
                // TODO: Re-enable in future release
                if (/* fr_improvement > 1.f || rms_improvement > 1.f */ false)
                    {
                        std::string txt = to_string() << "  Fill-Rate: " << std::setprecision(1) << std::fixed << new_fr << "%%";

                        if (!use_new_calib)
                        {
                            txt = to_string() << "  Fill-Rate: " << std::setprecision(1) << std::fixed << old_fr << "%%\n";
                        }

                        ImGui::SetCursorScreenPos({ float(x + 12), float(y + 90) });
                        ImGui::PushFont(win.get_large_font());
                        ImGui::Text("%s", static_cast<const char *>(textual_icons::check));
                        ImGui::PopFont();

                        ImGui::SetCursorScreenPos({ float(x + 35), float(y + 92) });
                        ImGui::Text("%s", txt.c_str());

                        if (use_new_calib)
                        {
                            ImGui::SameLine();

                            ImGui::PushStyleColor(ImGuiCol_Text, white);
                            txt = to_string() << " ( +" << std::fixed << std::setprecision(0) << fr_improvement << "%% )";
                            ImGui::Text("%s", txt.c_str());
                            ImGui::PopStyleColor();
                        }

                        if (rms_improvement > 1.f)
                        {
                            if (use_new_calib)
                            {
                                txt = to_string() << "  Noise Estimate: " << std::setprecision(2) << std::fixed << new_rms << new_units;
                            }
                            else
                            {
                                txt = to_string() << "  Noise Estimate: " << std::setprecision(2) << std::fixed << old_rms << old_units;
                            }

                            ImGui::SetCursorScreenPos({ float(x + 12), float(y + 90 + ImGui::GetTextLineHeight() + 6) });
                            ImGui::PushFont(win.get_large_font());
                            ImGui::Text("%s", static_cast<const char *>(textual_icons::check));
                            ImGui::PopFont();

                            ImGui::SetCursorScreenPos({ float(x + 35), float(y + 92 + ImGui::GetTextLineHeight() + 6) });
                            ImGui::Text("%s", txt.c_str());

                            if (use_new_calib)
                            {
                                ImGui::SameLine();

                                ImGui::PushStyleColor(ImGuiCol_Text, white);
                                txt = to_string() << " ( -" << std::setprecision(0) << std::fixed << rms_improvement << "%% )";
                                ImGui::Text("%s", txt.c_str());
                                ImGui::PopStyleColor();
                            }
                        }
                    }
                else
                {
                    ImGui::SetCursorScreenPos({ float(x + 12), float(y + 100) });
                    ImGui::Text("%s", "Please compare new vs old calibration\nand decide if to keep or discard the result...");
                }

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 60) });

                if (ImGui::RadioButton("New", use_new_calib))
                {
                    use_new_calib = true;
                    get_manager().apply_calib(true);
                }

                ImGui::SetCursorScreenPos({ float(x + 150), float(y + 60) });
                if (ImGui::RadioButton("Original", !use_new_calib))
                {
                    use_new_calib = false;
                    get_manager().apply_calib(false);
                }

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                if (recommend_keep || get_manager().tare)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                }
                
                std::string button_name = to_string() << "Apply New" << "##apply" << index;
                if (!use_new_calib) button_name = to_string() << "Keep Original" << "##original" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    if (use_new_calib)
                    {
                        get_manager().keep();
                        update_state = RS2_CALIB_STATE_COMPLETE;
                        pinned = false;
                        enable_dismiss = false;
                        last_progress_time = last_interacted = system_clock::now() + milliseconds(500);
                    }
                    else dismiss(false);

                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                }
                if (recommend_keep || get_manager().tare)
                {
                    ImGui::PopStyleColor(2);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "New calibration values will be saved in device memory");
                }
            }

            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Text("%s", "Calibration Complete");

            ImGui::SetCursorScreenPos({ float(x + 10), float(y + 35) });
            ImGui::PushFont(win.get_large_font());
            std::string txt = to_string() << textual_icons::throphy;
            ImGui::Text("%s", txt.c_str());
            ImGui::PopFont();

            ImGui::SetCursorScreenPos({ float(x + 40), float(y + 35) });

            ImGui::Text("%s", "Camera Calibration Applied Successfully");
        }

        ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

        if (update_manager)
        {
            if (update_state == RS2_CALIB_STATE_INITIAL_PROMPT)
            {
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                std::string button_name = to_string() << "Health-Check" << "##health_check" << index;

                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }) || update_manager->started())
                {
                    if (!update_manager->started()) update_manager->start(shared_from_this());

                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                    last_progress_time = system_clock::now();
                }
                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Keep the camera pointing at an object or a wall");
                }
            }
            else if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_CALIB_STATE_CALIB_COMPLETE;
                    enable_dismiss = true;
                    get_manager().apply_calib(true);
                    use_new_calib = true;
                }

                if (!expanded)
                {
                    if (update_manager->failed())
                    {
                        update_manager->check_error(_error_message);
                        update_state = RS2_CALIB_STATE_FAILED;
                        enable_dismiss = true;
                        //pinned = false;
                        //dismiss(false);
                    }

                    draw_progress_bar(win, bar_width);

                    ImGui::SetCursorScreenPos({ float(x + width - 105), float(y + height - 25) });

                    string id = to_string() << "Expand" << "##" << index;
                    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                    if (ImGui::Button(id.c_str(), { 100, 20 }))
                    {
                        expanded = true;
                    }

                    ImGui::PopStyleColor();
                }
            }
        }
    }

    void autocalib_notification_model::dismiss(bool snooze)
    {
        get_manager().update_last_used();

        if (!use_new_calib && get_manager().done()) 
            get_manager().apply_calib(false);

        get_manager().restore_workspace([this](std::function<void()> a){ a(); });

        if (update_state != RS2_CALIB_STATE_TARE_INPUT)
            update_state = RS2_CALIB_STATE_INITIAL_PROMPT;
        get_manager().reset();

        notification_model::dismiss(snooze);
    }

    void autocalib_notification_model::draw_expanded(ux_window& win, std::string& error_message)
    {
        if (update_manager->started() && update_state == RS2_CALIB_STATE_INITIAL_PROMPT) 
            update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
        ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(500, 100));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

        std::string title = "On-Chip Calibration";
        if (update_manager->failed()) title += " Failed";

        ImGui::OpenPopup(title.c_str());
        if (ImGui::BeginPopupModal(title.c_str(), nullptr, flags))
        {
            ImGui::SetCursorPosX(200);
            std::string progress_str = to_string() << "Progress: " << update_manager->get_progress() << "%";
            ImGui::Text("%s", progress_str.c_str());

            ImGui::SetCursorPosX(5);

            draw_progress_bar(win, 490);

            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, regular_blue);
            auto s = update_manager->get_log();
            ImGui::InputTextMultiline("##autocalib_log", const_cast<char*>(s.c_str()),
                s.size() + 1, { 490,100 }, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();

            ImGui::SetCursorPosX(190);
            if (visible || update_manager->done() || update_manager->failed())
            {
                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    if (update_manager->failed())
                    {
                        update_state = RS2_CALIB_STATE_FAILED;
                        //pinned = false;
                        //dismiss(false);
                    }
                    expanded = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, transparent);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
                ImGui::PushStyleColor(ImGuiCol_Text, transparent);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, transparent);
                ImGui::Button("OK", ImVec2(120, 0));
                ImGui::PopStyleColor(5);
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        error_message = "";
    }

    int autocalib_notification_model::calc_height()
    {
        if (update_state == RS2_CALIB_STATE_COMPLETE) return 65;
        else if (update_state == RS2_CALIB_STATE_INITIAL_PROMPT) return 120;
        else if (update_state == RS2_CALIB_STATE_CALIB_COMPLETE)
        {
            if (get_manager().allow_calib_keep()) return 170;
            else return 80;
        }
        else if (update_state == RS2_CALIB_STATE_SELF_INPUT) return 110;
        else if (update_state == RS2_CALIB_STATE_TARE_INPUT) return 85;
        else if (update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED) return 210;
        else if (update_state == RS2_CALIB_STATE_FAILED) return 110;
        else return 100;
    }

    void autocalib_notification_model::set_color_scheme(float t) const
    {
        notification_model::set_color_scheme(t);

        ImGui::PopStyleColor(1);

        ImVec4 c;

        if (update_state == RS2_CALIB_STATE_COMPLETE)
        {
            c = alpha(saturate(light_blue, 0.7f), 1 - t);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
        }
        else if (update_state == RS2_CALIB_STATE_FAILED)
        {
            c = alpha(dark_red, 1 - t);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
        }
        else
        {
            c = alpha(sensor_bg, 1 - t);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, c);
        }
    }

    autocalib_notification_model::autocalib_notification_model(std::string name,
        std::shared_ptr<on_chip_calib_manager> manager, bool exp)
        : process_notification_model(manager)
    {
        enable_expand = false;
        enable_dismiss = true;
        expanded = exp;
        if (expanded) visible = false;

        message = name;
        this->severity = RS2_LOG_SEVERITY_INFO;
        this->category = RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT;

        pinned = true;
    }
}
