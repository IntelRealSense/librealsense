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
#include "calibration-model.h"
#include "../src/algo.h"
#include "../src/ds5/ds5-private.h"
#include "../tools/depth-quality/depth-metrics.h"

namespace rs2
{
    on_chip_calib_manager::on_chip_calib_manager(viewer_model& viewer, std::shared_ptr<subdevice_model> sub, device_model& model, device dev, std::shared_ptr<subdevice_model> sub_color)
        : process_manager("On-Chip Calibration"), _model(model), _dev(dev), _sub(sub), _viewer(viewer), _sub_color(sub_color)
    {
        auto dev_name = dev.get_info(RS2_CAMERA_INFO_NAME);
        if (!strcmp(dev_name, "Intel RealSense D415"))
            speed = 4;
    }

    on_chip_calib_manager::~on_chip_calib_manager()
    {
        turn_roi_off();
    }

    void on_chip_calib_manager::turn_roi_on()
    {
        if (_sub)
        {
            _sub->show_algo_roi = true;
            _sub->algo_roi = { librealsense::rect_gaussian_dots_target_calculator::_roi_ws, 
                               librealsense::rect_gaussian_dots_target_calculator::_roi_hs, 
                               librealsense::rect_gaussian_dots_target_calculator::_roi_we, 
                               librealsense::rect_gaussian_dots_target_calculator::_roi_he };
        }

        if (_sub_color)
        {
            _sub_color->show_algo_roi = true;
            _sub_color->algo_roi = { librealsense::rect_gaussian_dots_target_calculator::_roi_ws, 
                                     librealsense::rect_gaussian_dots_target_calculator::_roi_hs, 
                                     librealsense::rect_gaussian_dots_target_calculator::_roi_we, 
                                     librealsense::rect_gaussian_dots_target_calculator::_roi_he };
        }
    }

    void on_chip_calib_manager::turn_roi_off()
    {
        if (_sub)
        {
            _sub->show_algo_roi = false;
            _sub->algo_roi = { 0, 0, 0, 0 };
        }

        if (_sub_color)
        {
            _sub_color->show_algo_roi = false;
            _sub_color->algo_roi = { 0, 0, 0, 0 };
        }
    }

    void on_chip_calib_manager::stop_viewer(invoker invoke)
    {
        try
        {
            auto profiles = _sub->get_selected_profiles();

            invoke([&](){
                // Stop viewer UI
                _sub->stop(_viewer.not_model);
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

    bool on_chip_calib_manager::start_viewer(int w, int h, int fps, invoker invoke)
    {
        bool frame_arrived = false;
        try
        {
            bool run_fl_calib = action == RS2_CALIB_ACTION_FL_CALIB && w == 256 && h == 144 && fps == 90;
            if (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
            {
                _uid = 1;
                for (const auto& format : _sub->formats)
                {
                    if (format.second[0] == "Y8")
                    {
                        _uid = format.first;
                        break;
                    }
                }

                if (_sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                    _sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, 0.0f);
            }
            else if (action == RS2_CALIB_ACTION_UVMAPPING)
            {
                _uid = 1;
                _uid2 = 2;
                bool first_done = 0;
                bool second_done = 0;
                for (const auto& format : _sub->formats)
                {
                    if (format.second[0] == "Y8" && !first_done)
                    {
                        _uid = format.first;
                        first_done = true;
                    }

                    if (format.second[0] == "Z16" && !second_done)
                    {
                        _uid2 = format.first;
                        second_done = true;
                    }

                    if (first_done && second_done)
                        break;
                }

                for (const auto& format : _sub_color->formats)
                {
                    int done = false;
                    for (int i = 0; i < format.second.size(); ++i)
                    {
                        if (format.second[i] == "RGB8")
                        {
                            _uid_color = format.first;
                            done = true;
                            break;
                        }
                    }
                    if (done)
                        break;
                }

                if (_sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                    _sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, 0.0f);
            }
            else if (run_fl_calib)
            {
                _uid = 1;
                _uid2 = 2;
                bool first_done = 0;
                for (const auto& format : _sub->formats)
                {
                    if (format.second[0] == "Y8")
                    {
                        if (!first_done)
                        {
                            _uid = format.first;
                            first_done = true;
                        }
                        else
                        {
                            _uid2 = format.first;
                            break;
                        }
                    }
                }

                if (_sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                    _sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, 0.0f);
            }
            else
            {
                _uid = 0;
                for (const auto& format : _sub->formats)
                {
                    if (format.second[0] == "Z16")
                    {
                        _uid = format.first;
                        break;
                    }
                }
            }

            // Select stream
            _sub->stream_enabled.clear();
            _sub->stream_enabled[_uid] = true;
            if (run_fl_calib || action == RS2_CALIB_ACTION_UVMAPPING)
                _sub->stream_enabled[_uid2] = true;

            _sub->ui.selected_format_id.clear();
            _sub->ui.selected_format_id[_uid] = 0;
            if (run_fl_calib)
                _sub->ui.selected_format_id[_uid2] = 0;

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

            std::vector<stream_profile> profiles_color;
            if (action == RS2_CALIB_ACTION_UVMAPPING)
            {
                _sub_color->stream_enabled[_uid_color] = true;
                _sub_color->ui.selected_format_id.clear();
                _sub_color->ui.selected_format_id[_uid_color] = 0;

                for (int i = 0; i < _sub_color->shared_fps_values.size(); i++)
                {
                    if (_sub_color->shared_fps_values[i] == fps)
                        _sub_color->ui.selected_shared_fps_id = i;
                }

                for (int i = 0; i < _sub_color->res_values.size(); i++)
                {
                    auto kvp = _sub_color->res_values[i];
                    if (kvp.first == w && kvp.second == h)
                        _sub_color->ui.selected_res_id = i;
                }

                profiles_color = _sub_color->get_selected_profiles();
            }

            invoke([&]() 
                {
                    if (!_model.dev_syncer)
                        _model.dev_syncer = _viewer.syncer->create_syncer();

                    // Start streaming
                    _sub->play(profiles, _viewer, _model.dev_syncer);
                    for (auto&& profile : profiles)
                        _viewer.begin_stream(_sub, profile);

                    if (action == RS2_CALIB_ACTION_UVMAPPING)
                    {
                        _sub_color->play(profiles_color, _viewer, _model.dev_syncer);
                        for (auto&& profile : profiles_color)
                            _viewer.begin_stream(_sub_color, profile);
                    }
                });

            // Wait for frames to arrive
            int count = 0;
            while (!frame_arrived && count++ < 200)
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

        return frame_arrived;
    }

    std::pair<float, float> on_chip_calib_manager::get_metric(bool use_new)
    {
        return _metrics[use_new ? 1 : 0];
    }

    void on_chip_calib_manager::try_start_viewer(int w, int h, int fps, invoker invoke)
    {
        bool started = start_viewer(w, h, fps, invoke);
        if (!started)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            started = start_viewer(w, h, fps, invoke);
        }

        if (!started)
        {
            stop_viewer(invoke);
            log(to_string() << "Failed to start streaming");
            throw std::runtime_error(to_string() << "Failed to start streaming (" << w << ", " << h << ", " << fps << ")!");
        }
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
        int occ_timeout_ms = 9000;
        if (action == RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || action == RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
        {
            if (toggle)
            {
                occ_timeout_ms = 12000;
                if (speed_fl == 0)
                    speed_fl = 1;
                else if (speed_fl == 1)
                    speed_fl = 0;
                toggle = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            }

            if (speed_fl == 0)
            {
                speed = 1;
                fl_step_count = 41;
                fy_scan_range = 30;
                white_wall_mode = 0;
            }
            else if (speed_fl == 1)
            {
                speed = 3;
                fl_step_count = 51;
                fy_scan_range = 40;
                white_wall_mode = 0;
            }
            else if (speed_fl == 2)
            {
                speed = 4;
                fl_step_count = 41;
                fy_scan_range = 30;
                white_wall_mode = 1;
            }
        }

        std::stringstream ss;
        if (action == RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
        {
            ss << "{\n \"calib type\":" << 1 <<
                  ",\n \"fl step count\":" << fl_step_count <<
                  ",\n \"fy scan range\":" << fy_scan_range <<
                  ",\n \"keep new value after sucessful scan\":" << keep_new_value_after_sucessful_scan <<
                  ",\n \"fl data sampling\":" << fl_data_sampling <<
                  ",\n \"adjust both sides\":" << adjust_both_sides <<  
                  ",\n \"fl scan location\":" << fl_scan_location <<
                  ",\n \"fy scan direction\":" << fy_scan_direction <<
                  ",\n \"white wall mode\":" << white_wall_mode << "}";
        }
        else if (action == RS2_CALIB_ACTION_ON_CHIP_CALIB)
        {
            ss << "{\n \"calib type\":" << 0 <<
                  ",\n \"speed\":" << speed <<
                  ",\n \"average step count\":" << average_step_count <<
                  ",\n \"scan parameter\":" << (intrinsic_scan ? 0 : 1) <<
                  ",\n \"step count\":" << step_count <<
                  ",\n \"apply preset\":" << (apply_preset ? 1 : 0) <<
                  ",\n \"accuracy\":" << accuracy << "}";
        }
        else
        {
            ss << "{\n \"calib type\":" << 2 <<
                  ",\n \"fl step count\":" << fl_step_count <<
                  ",\n \"fy scan range\":" << fy_scan_range <<
                  ",\n \"keep new value after sucessful scan\":" << keep_new_value_after_sucessful_scan <<
                  ",\n \"fl data sampling\":" << fl_data_sampling <<
                  ",\n \"adjust both sides\":" << adjust_both_sides <<
                  ",\n \"fl scan location\":" << fl_scan_location <<
                  ",\n \"fy scan direction\":" << fy_scan_direction <<
                  ",\n \"white wall mode\":" << white_wall_mode << 
                  ",\n \"speed\":" << speed <<
                  ",\n \"average step count\":" << average_step_count <<
                  ",\n \"scan parameter\":" << (intrinsic_scan ? 0 : 1) <<
                  ",\n \"step count\":" << step_count <<
                  ",\n \"apply preset\":" << (apply_preset ? 1 : 0) <<
                  ",\n \"accuracy\":" << accuracy << "}";
        }
        std::string json = ss.str();

        float health[2] = { -1.0f, -1.0f };
        auto calib_dev = _dev.as<auto_calibrated_device>();
        if (action == RS2_CALIB_ACTION_TARE_CALIB)
            _new_calib = calib_dev.run_tare_calibration(ground_truth, json, health, [&](const float progress) {_progress = int(progress);}, 5000);
        else if (action == RS2_CALIB_ACTION_ON_CHIP_CALIB || action == RS2_CALIB_ACTION_ON_CHIP_FL_CALIB || action == RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
            _new_calib = calib_dev.run_on_chip_calibration(json, &_health, [&](const float progress) {_progress = int(progress);}, occ_timeout_ms);

        if (action == RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
        {
            int h_both = static_cast<int>(_health);
            int h_1 = (h_both & 0x00000FFF);
            int h_2 = (h_both & 0x00FFF000) >> 12;
            int sign = (h_both & 0x0F000000) >> 24;

            _health_1 = h_1 / 1000.0f;
            if (sign & 1)
                _health_1 = -_health_1;

            _health_2 = h_2 / 1000.0f;
            if (sign & 2)
                _health_2 = -_health_2;
        }
        else if (action == RS2_CALIB_ACTION_TARE_CALIB)
        {
            _health_1 = health[0] * 100;
            _health_2 = health[1] * 100;
        }
    }

    void on_chip_calib_manager::calibrate_fl()
    {
        try
        {
            auto sensor = _sub->s->as<rs2::depth_stereo_sensor>();
            float stereo_baseline = 0.0f;
            if (sensor)
                stereo_baseline = sensor.get_option(RS2_OPTION_STEREO_BASELINE);

            std::shared_ptr<rect_calculator> gt_calculator[2];
            bool created[2] = { false, false };

            int counter = 0;
            int limit = rect_calculator::_frame_num << 1;
            int step = limit / rect_calculator::_frame_num;

            int ret = { 0 };
            int id[2] = { _uid, _uid2 };
            float fx[2] = { 0 };
            float fy[2] = { 0 };
            float rec_sides[2][4] = { 0 };
            float target_fw[2] = { 0 };
            float target_fh[2] = { 0 };

            rs2::frame f;
            bool done[2] = { false, false };
            while (counter < limit)
            {
                for (int i = 0; i < 2; ++i)
                {
                    if (!done[i])
                    {
                        f = _viewer.ppf.frames_queue[id[i]].wait_for_frame();
                        if (f)
                        {
                            if (!created[i])
                            {
                                stream_profile profile = f.get_profile();
                                auto vsp = profile.as<video_stream_profile>();

                                gt_calculator[i] = std::make_shared<rect_calculator>();
                                fx[i] = vsp.get_intrinsics().fx;
                                fy[i] = vsp.get_intrinsics().fy;
                                target_fw[i] = vsp.get_intrinsics().fx * config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f);
                                target_fh[i] = vsp.get_intrinsics().fy * config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f);
                                created[i] = true;
                            }

                            ret = gt_calculator[i]->calculate(f.get(), rec_sides[i]);
                            if (ret == 0)
                                ++counter;
                            else if (ret == 1)
                                _progress += step;
                            else if (ret == 2)
                            {
                                _progress += step;
                                done[i] = true;
                            }
                        }
                    }
                }

                if (done[0] && done[1])
                    break;
            }

            if (done[0] && done[1] && fx[1] > 0.1f && fy[1] > 0.1f)
            {
                float ar[2] = { 0 };
                float tmp = rec_sides[0][2] + rec_sides[0][3];
                if (tmp > 0.1f)
                    ar[0] = (rec_sides[0][0] + rec_sides[0][1]) / tmp;

                tmp = rec_sides[1][2] + rec_sides[1][3];
                if (tmp > 0.1f)
                    ar[1] = (rec_sides[1][0] + rec_sides[1][1]) / tmp;

                if (ar[0] > 0.0f)
                    align = ar[1] / ar[0] - 1.0f;

                float ta[2] = { 0 };
                float gt[4] = { 0 };
                float ave_gt = 0.0f;
                for (int i = 0; i < 2; ++i)
                {
                    if (rec_sides[i][0] > 0)
                        gt[0] = target_fw[i] / rec_sides[i][0];

                    if (rec_sides[i][1] > 0)
                        gt[1] = target_fw[i] / rec_sides[i][1];

                    if (rec_sides[i][2] > 0)
                        gt[2] = target_fh[i] / rec_sides[i][2];

                    if (rec_sides[i][3] > 0)
                        gt[3] = target_fh[i] / rec_sides[i][3];

                    ave_gt = 0.0f;
                    for (int i = 0; i < 4; ++i)
                        ave_gt += gt[i];
                    ave_gt /= 4.0;

                    ta[i] = atanf(align * ave_gt / stereo_baseline);
                    ta[i] = rad2deg(ta[i]);
                }

                tilt_angle = (ta[0] + ta[1]) / 2;

                align *= 100;

                float r[4] = { 0 };
                float c = fx[0] / fx[1];

                if (rec_sides[0][0] > 0.1f)
                    r[0] = c * rec_sides[1][0] / rec_sides[0][0];

                if (rec_sides[0][1] > 0.1f)
                    r[1] = c * rec_sides[1][1] / rec_sides[0][1];

                c = fy[0] / fy[1];
                if (rec_sides[0][2] > 0.1f)
                    r[2] = c * rec_sides[1][2] / rec_sides[0][2];

                if (rec_sides[0][3] > 0.1f)
                    r[3] = c * rec_sides[1][3] / rec_sides[0][3];

                ratio = 0.0f;
                for (int i = 0; i < 4; ++i)
                    ratio += r[i];
                ratio /= 4;

                ratio -= 1.0f;
                ratio *= 100;

                corrected_ratio = ratio - correction_factor * align;

                float ratio_to_apply = corrected_ratio / 100.0f + 1.0f;
                _new_calib = _old_calib;
                auto table = (librealsense::ds::coefficients_table*)_new_calib.data();
                table->intrinsic_right.x.x *= ratio_to_apply;
                table->intrinsic_right.x.y *= ratio_to_apply;

                auto actual_data = _new_calib.data() + sizeof(librealsense::ds::table_header);
                auto actual_data_size = _new_calib.size() - sizeof(librealsense::ds::table_header);
                auto crc = helpers::calc_crc32(actual_data, actual_data_size);
                table->header.crc32 = crc;
            }
            else
                fail("Please adjust the camera position \nand make sure the specific target is \nin the middle of the camera image!");
        }
        catch (const std::runtime_error& error)
        {
            fail(error.what());
        }
        catch (...)
        {
            fail("Focal length calibration failed!");
        }
    }

    void on_chip_calib_manager::undistort(uint8_t* img, int width, int height, const rs2_intrinsics & intrin)
    {
        int size = width * height;
        std::vector<uint8_t> tmp(size);
        memset(tmp.data(), 0, size);

        float x = 0;
        float y = 0;
        int m = 0;
        int n = 0;
        uint8_t* p = img;
        for (int j = 0; j < height; ++j)
        {
            for (int i = 0; i < width; ++i)
            {
                x = static_cast<float>(i);
                y = static_cast<float>(j);
                assert(intrin.model == RS2_DISTORTION_INVERSE_BROWN_CONRADY);

                if (abs(intrin.fx) > 0.00001f && abs(intrin.fy) > 0.0001f)
                {
                    x = (x - intrin.ppx) / intrin.fx;
                    y = (y - intrin.ppy) / intrin.fy;

                    float r2 = x * x + y * y;
                    float f = 1 + intrin.coeffs[0] * r2 + intrin.coeffs[1] * r2 * r2 + intrin.coeffs[4] * r2 * r2 * r2;
                    float ux = x * f + 2 * intrin.coeffs[2] * x * y + intrin.coeffs[3] * (r2 + 2 * x * x);
                    float uy = y * f + 2 * intrin.coeffs[3] * x * y + intrin.coeffs[2] * (r2 + 2 * y * y);
                    x = ux;
                    y = uy;

                    x = x * intrin.fx + intrin.ppx;
                    y = y * intrin.fy + intrin.ppy;
                }

                m = static_cast<int>(x + 0.5f);
                if (m >= 0 && m < width)
                {
                    n = static_cast<int>(y + 0.5f);
                    if (n >= 0 && n < height)
                        tmp[n * width + m] = *p++;
                }
            }
        }

        memmove(img, tmp.data(), size);
    }

    void on_chip_calib_manager::FindZatCorners(float left_x[4], float left_y[4], int width, int num, std::vector<std::vector<uint16_t>>& depth, float left_z[4])
    {
        int x1[4] = { 0 };
        int y1[4] = { 0 };
        int x2[4] = { 0 };
        int y2[4] = { 0 };
        int pos_tl[4] = { 0 };
        int pos_tr[4] = { 0 };
        int pos_bl[4] = { 0 };
        int pos_br[4] = { 0 };
        for (int i = 0; i < 4; ++i)
        {
            x1[i] = static_cast<int>(left_x[i]);
            y1[i] = static_cast<int>(left_y[i]);

            x2[i] = static_cast<int>(left_x[i] + 1.0f);
            y2[i] = static_cast<int>(left_y[i] + 1.0f);

            pos_tl[i] = y1[i] * width + x1[i];
            pos_tr[i] = y1[i] * width + x2[i];
            pos_bl[i] = y2[i] * width + x1[i];
            pos_br[i] = y2[i] * width + x2[i];
        }

        float left_z_tl[4] = { 0 };
        float left_z_tr[4] = { 0 };
        float left_z_bl[4] = { 0 };
        float left_z_br[4] = { 0 };
        for (int j = 0; j < num; ++j)
        {
            for (int i = 0; i < 4; ++i)
            {
                left_z_tl[i] += static_cast<float>(depth[j][pos_tl[i]]);
                left_z_tr[i] += static_cast<float>(depth[j][pos_tr[i]]);
                left_z_bl[i] += static_cast<float>(depth[j][pos_bl[i]]);
                left_z_br[i] += static_cast<float>(depth[j][pos_br[i]]);
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            left_z_tl[i] /= num;
            left_z_tr[i] /= num;
            left_z_bl[i] /= num;
            left_z_br[i] /= num;
        }

        float z_1 = 0.0f;
        float z_2 = 0.0f;
        float s = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            s = left_x[i] - x1[i];
            z_1 = (1.0f - s) * left_z_bl[i] + s * left_z_br[i];
            z_2 = (1.0f - s) * left_z_tl[i] + s * left_z_tr[i];

            s = left_y[i] - y1[i];
            left_z[i] = (1.0f - s) * z_2 + s * z_1;
            left_z[i] *= 0.001f;
        }
    }

    void on_chip_calib_manager::calibrate_uvmapping()
    {
        try
        {
            std::shared_ptr<dots_calculator> gt_calculator[2];
            bool created[3] = { false, false, false };

            int counter = 0;
            int limit = dots_calculator::_frame_num << 1;
            int step = limit / dots_calculator::_frame_num;

            int ret = { 0 };
            int id[3] = { _uid, _uid_color, _uid2 }; // 0 for left, 1 for color, and 2 for depth
            rs2_intrinsics intrin[2];
            stream_profile profile[2];
            float dots_x[2][4] = { 0 };
            float dots_y[2][4] = { 0 };

            int idx = 0;
            int depth_frame_size = 0;
            std::vector<std::vector<uint16_t>> depth(dots_calculator::_frame_num);

            rs2::frame f;
            int width = 0;
            int height = 0;
            bool done[3] = { false, false, false };
            while (counter < limit)
            {
                if (!done[2])
                {
                    f = _viewer.ppf.frames_queue[id[2]].wait_for_frame();

                    if (!created[2])
                    {
                        profile[1] = f.get_profile();
                        auto vsp = profile[1].as<video_stream_profile>();
                        width = vsp.width();
                        depth_frame_size = vsp.width() * vsp.height() * sizeof(uint16_t);
                        created[2] = true;
                    }

                    depth[idx].resize(depth_frame_size);
                    memmove(depth[idx++].data(), f.get_data(), depth_frame_size);

                    if (idx == dots_calculator::_frame_num)
                        done[2] = true;
                }

                if (!done[0])
                {
                    f = _viewer.ppf.frames_queue[id[0]].wait_for_frame();

                    if (!created[0])
                    {
                        profile[0] = f.get_profile();
                        auto vsp = profile[0].as<video_stream_profile>();

                        gt_calculator[0] = std::make_shared<dots_calculator>();
                        intrin[0] = vsp.get_intrinsics();
                        created[0] = true;
                    }

                    ret = gt_calculator[0]->calculate(f.get(), dots_x[0], dots_y[0]);
                    if (ret == 0)
                        ++counter;
                    else if (ret == 1)
                        _progress += step;
                    else if (ret == 2)
                    {
                        _progress += step;
                        done[0] = true;
                    }
                }

                if (!done[1])
                {
                    f = _viewer.ppf.frames_queue[id[1]].wait_for_frame();

                    if (!created[1])
                    {
                        profile[1] = f.get_profile();
                        auto vsp = profile[1].as<video_stream_profile>();
                        width = vsp.width();
                        height = vsp.height();

                        gt_calculator[1] = std::make_shared<dots_calculator>();
                        intrin[1] = vsp.get_intrinsics();
                        created[1] = true;
                    }

                    undistort(const_cast<uint8_t *>(static_cast<const uint8_t *>(f.get_data())), width, height, intrin[1]);
                    ret = gt_calculator[1]->calculate(f.get(), dots_x[1], dots_y[1]);
                    if (ret == 0)
                        ++counter;
                    else if (ret == 1)
                        _progress += step;
                    else if (ret == 2)
                    {
                        _progress += step;
                        done[1] = true;
                    }
                }

                if (done[0] && done[1] && done[2])
                    break;
            }

            if (done[0] && done[1] && done[2])
            {
                rs2_extrinsics extrin = profile[0].get_extrinsics_to(profile[1]);

                float z[4] = { 0 };
                FindZatCorners(dots_x[0], dots_y[0], width, dots_calculator::_frame_num, depth, z);

                uvmapping_calib calib(4, dots_x[0], dots_y[0], z, dots_x[1], dots_y[1], intrin[0], intrin[1], extrin);



            }
            else
                fail("Please adjust the camera position\nand make sure the specific target is\nin the middle of the camera images!");
        }
        catch (const std::runtime_error& error)
        {
            fail(error.what());
        }
        catch (...)
        {
            fail("UVMapping calibration failed!");
        }
    }

    void on_chip_calib_manager::get_ground_truth()
    {
        try
        {
            std::shared_ptr<rect_calculator> gt_calculator;
            bool created = false;

            int counter = 0;
            int limit = rect_calculator::_frame_num << 1;
            int step = 100 / rect_calculator::_frame_num;

            float rect_sides[4] = { 0 };
            float target_fw = 0;
            float target_fh = 0;

            int ret = 0;
            rs2::frame f;
            while (counter < limit)
            {
                f = _viewer.ppf.frames_queue[_uid].wait_for_frame();
                if (f)
                {
                    if (!created)
                    {
                        stream_profile profile = f.get_profile();
                        auto vsp = profile.as<video_stream_profile>();

                        gt_calculator = std::make_shared<rect_calculator>();
                        target_fw = vsp.get_intrinsics().fx * config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f);
                        target_fh = vsp.get_intrinsics().fy * config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f);
                        created = true;
                    }

                    ret = gt_calculator->calculate(f.get(), rect_sides);
                    if (ret == 0)
                        ++counter;
                    else if (ret == 1)
                        _progress += step;
                    else if (ret == 2)
                    {
                        _progress += step;
                        break;
                    }
                }
            }

            if (ret != 2)
                fail("Please adjust the camera position \nand make sure the specific target is \nin the middle of the camera image!");
            else
            {
                float gt[4] = { 0 };

                if (rect_sides[0] > 0)
                    gt[0] = target_fw / rect_sides[0];

                if (rect_sides[1] > 0)
                    gt[1] = target_fw / rect_sides[1];

                if (rect_sides[2] > 0)
                    gt[2] = target_fh / rect_sides[2];

                if (rect_sides[3] > 0)
                    gt[3] = target_fh / rect_sides[3];

                if (gt[0] <= 0.1f || gt[1] <= 0.1f || gt[2] <= 0.1f || gt[3] <= 0.1f)
                    fail("Bad target rectangle side sizes returned!");

                ground_truth = 0.0;
                for (int i = 0; i < 4; ++i)
                    ground_truth += gt[i];
                ground_truth /= 4.0;

                config_file::instance().set(configurations::viewer::ground_truth_r, ground_truth);
            }
        }
        catch (const std::runtime_error& error)
        {
            fail(error.what());
        }
        catch (...)
        {
            fail("Getting ground truth failed!");
        }
    }

    void on_chip_calib_manager::process_flow(std::function<void()> cleanup, invoker invoke)
    {
        if (action == RS2_CALIB_ACTION_FL_CALIB || action == RS2_CALIB_ACTION_UVMAPPING)
            stop_viewer(invoke);

        update_last_used();

        if (action == RS2_CALIB_ACTION_ON_CHIP_FL_CALIB || action == RS2_CALIB_ACTION_FL_CALIB)
            log(to_string() << "Starting focal length calibration");
        else if (action == RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
            log(to_string() << "Starting OCC Extended");
        else if (action == RS2_CALIB_ACTION_UVMAPPING)
            log(to_string() << "Starting UVMapping calibration");
        else
            log(to_string() << "Starting OCC at speed " << speed);

        _in_3d_view = _viewer.is_3d_view;
        _viewer.is_3d_view = (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH ? false : true);

        auto calib_dev = _dev.as<auto_calibrated_device>();
        _old_calib = calib_dev.get_calibration_table();

        _was_streaming = _sub->streaming;
        _synchronized = _viewer.synchronization_enable.load();
        _post_processing = _sub->post_processing_enabled;
        _sub->post_processing_enabled = false;
        _viewer.synchronization_enable = false;

        _restored = false;

        if (action != RS2_CALIB_ACTION_TARE_GROUND_TRUTH && action != RS2_CALIB_ACTION_UVMAPPING)
        {
            if (!_was_streaming)
            {
                if (action == RS2_CALIB_ACTION_FL_CALIB)
                    try_start_viewer(848, 480, 30, invoke);
                else
                    try_start_viewer(0, 0, 0, invoke);
            }

            // Capture metrics before
            auto metrics_before = get_depth_metrics(invoke);
            _metrics.push_back(metrics_before);
        }

        stop_viewer(invoke);

        _ui = std::make_shared<subdevice_ui_selection>(_sub->ui);
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        // Switch into special Auto-Calibration mode
        if (action == RS2_CALIB_ACTION_FL_CALIB || action == RS2_CALIB_ACTION_UVMAPPING)
            _viewer.is_3d_view = false;

        if (action == RS2_CALIB_ACTION_UVMAPPING)
            try_start_viewer(1280, 720, 15, invoke);
        else
            try_start_viewer(256, 144, 90, invoke);

        if (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
            get_ground_truth();
        else
        {
            try
            {
                if (action == RS2_CALIB_ACTION_FL_CALIB)
                    calibrate_fl();
                else if (action == RS2_CALIB_ACTION_UVMAPPING)
                    calibrate_uvmapping();
                else
                    calibrate();
            }
            catch (...)
            {
                log(to_string() << "Calibration failed with exception");
                stop_viewer(invoke);
                if (_ui.get())
                    _sub->ui = *_ui;
                if (_was_streaming)
                    start_viewer(0, 0, 0, invoke);
                throw;
            }
        }

        if (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
            log(to_string() << "Tare ground truth is got: " << ground_truth);
        else if (action == RS2_CALIB_ACTION_FL_CALIB)
            log(to_string() << "Focal length ratio is got: " << ratio);
        else if (action == RS2_CALIB_ACTION_UVMAPPING)
            log(to_string() << "UVMapping calibration completed.");
        else
            log(to_string() << "Calibration completed, health factor = " << _health);

        stop_viewer(invoke);
        if (_ui.get())
            _sub->ui = *_ui;

        if (action != RS2_CALIB_ACTION_TARE_GROUND_TRUTH && action != RS2_CALIB_ACTION_UVMAPPING)
        {
            if (action == RS2_CALIB_ACTION_FL_CALIB)
                _viewer.is_3d_view = true;

            try_start_viewer(0, 0, 0, invoke); // Start with default settings

            // Make new calibration active
            apply_calib(true);

            // Capture metrics after
            auto metrics_after = get_depth_metrics(invoke);
            _metrics.push_back(metrics_after);
        }

        _progress = 100;

        _done = true;
    }

    void on_chip_calib_manager::restore_workspace(invoker invoke)
    {
        try
        {
            if (_restored) return;

            _viewer.is_3d_view = _in_3d_view;
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

        auto recommend_keep = false;  
        if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
        {
            float health_1 = get_manager().get_health_1();
            float health_2 = get_manager().get_health_2();
            bool recommend_keep_1 = fabs(health_1) < 0.25f;
            bool recommend_keep_2 = fabs(health_2) < 0.15f;
            recommend_keep = (recommend_keep_1 && recommend_keep_2);
        }
        else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
            recommend_keep = fabs(get_manager().get_health()) < 0.15f;
        else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB)
            recommend_keep = fabs(get_manager().get_health()) < 0.25f;

        if (recommend_keep && update_state == RS2_CALIB_STATE_CALIB_COMPLETE && (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB))
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

        if (update_state == RS2_CALIB_STATE_UVMAPPING_INPUT || update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS && get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_UVMAPPING)
            get_manager().turn_roi_on();
        else
            get_manager().turn_roi_off();

        const auto bar_width = width - 115;

        ImGui::SetCursorScreenPos({ float(x + 9), float(y + 4) });

        ImVec4 shadow{ 1.f, 1.f, 1.f, 0.1f };
        ImGui::GetWindowDrawList()->AddRectFilled({ float(x), float(y) },
        { float(x + width), float(y + 25) }, ImColor(shadow));

        if (update_state != RS2_CALIB_STATE_COMPLETE)
        {
            if (update_state == RS2_CALIB_STATE_INITIAL_PROMPT)
                ImGui::Text("%s", "Calibration Health-Check");
            else if (update_state == RS2_CALIB_STATE_UVMAPPING_INPUT)
                ImGui::Text("%s", "UVMapping Calibration");
            else if (update_state == RS2_CALIB_STATE_CALIB_IN_PROCESS ||
                     update_state == RS2_CALIB_STATE_CALIB_COMPLETE ||
                     update_state == RS2_CALIB_STATE_SELF_INPUT)
            {
               if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
                   ImGui::Text("%s", "On-Chip Calibration Extended");
               else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
                   ImGui::Text("%s", "On-Chip Focal Length Calibration");
               else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
                   ImGui::Text("%s", "Tare Calibration");
               else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB)
                   ImGui::Text("%s", "Focal Length Calibration");
               else
                   ImGui::Text("%s", "On-Chip Calibration");
            }
            else if (update_state == RS2_CALIB_STATE_FL_INPUT)
                ImGui::Text("%s", "Focal Length Calibration");
            else if (update_state == RS2_CALIB_STATE_TARE_INPUT || update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                ImGui::Text("%s", "Tare Calibration");
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH || update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_IN_PROCESS || update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_COMPLETE)
                ImGui::Text("%s", "Get Tare Calibration Ground Truth");
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_FAILED)
                ImGui::Text("%s", "Get Tare Calibration Ground Truth Failed");
            else if (update_state == RS2_CALIB_STATE_FAILED && !((get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB) && get_manager().retry_times < 3))
                ImGui::Text("%s", "Calibration Failed");

            if (update_state == RS2_CALIB_STATE_TARE_INPUT || update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                ImGui::SetCursorScreenPos({ float(x + width - 30), float(y) });
            else if (update_state == RS2_CALIB_STATE_FAILED)
                ImGui::SetCursorScreenPos({ float(x + 2), float(y + 27) });
            else
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 27) });

            ImGui::PushStyleColor(ImGuiCol_Text, alpha(light_grey, 1.f - t));

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
            else if (update_state == RS2_CALIB_STATE_UVMAPPING_INPUT)
            {
                ImGui::SetCursorScreenPos({ float(x + 15), float(y + 33) });
                ImGui::Text("%s", "Please start left and color streaming with\nresolution 1280x720 and adjust camera\nposition if necessary to make sure the target\nis in the middle of both left and color images.");

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - 25) });
                std::string button_name = to_string() << "Calibrate" << "##uvmapping" << index;
                if (ImGui::Button(button_name.c_str(), { float(bar_width - 60), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
                    get_manager().retry_times = 0;
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_UVMAPPING;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Begin UVMapping calibration after adjusting camera position");
                }
                ImGui::PopStyleColor(2);
            }
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH)
            {
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                ImGui::Text("%s", "Target Width:");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "The width of the rectangle in millimeter inside the specific target");
                }

                const int MAX_SIZE = 256;
                char buff[MAX_SIZE];

                ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });
                std::string id = to_string() << "##target_width_" << index;
                ImGui::PushItemWidth(width - 145.0f);
                float target_width = config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f);
                std::string tw = to_string() << target_width;
                memcpy(buff, tw.c_str(), tw.size() + 1);
                if (ImGui::InputText(id.c_str(), buff, std::max((int)tw.size() + 1, 10)))
                {
                    std::stringstream ss;
                    ss << buff;
                    ss >> target_width;
                    config_file::instance().set(configurations::viewer::target_width_r, target_width);
                }
                ImGui::PopItemWidth();

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 38 + ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Target Height:");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "The height of the rectangle in millimeter inside the specific target");
                }

                ImGui::SetCursorScreenPos({ float(x + 135), float(y + 35 + ImGui::GetTextLineHeightWithSpacing()) });
                id = to_string() << "##target_height_" << index;
                ImGui::PushItemWidth(width - 145.0f);
                float target_height = config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f);
                std::string th = to_string() << target_height;
                memcpy(buff, th.c_str(), th.size() + 1);
                if (ImGui::InputText(id.c_str(), buff, std::max((int)th.size() + 1, 10)))
                {
                    std::stringstream ss;
                    ss << buff;
                    ss >> target_height;
                    config_file::instance().set(configurations::viewer::target_height_r, target_height);
                }
                ImGui::PopItemWidth();

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - 25) });
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                std::string back_button_name = to_string() << "Back" << "##tare" << index;
                if (ImGui::Button(back_button_name.c_str(), { float(60), 20.f }))
                {
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB;
                    update_state = update_state_prev;
                    if (get_manager()._sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                        get_manager()._sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, get_manager().laser_status_prev);
                }

                ImGui::SetCursorScreenPos({ float(x + 85), float(y + height - 25) });
                std::string button_name = to_string() << "Calculate" << "##tare" << index;
                if (ImGui::Button(button_name.c_str(), { float(bar_width - 70), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
                    get_manager().retry_times = 0;
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_TARE_GROUND_TRUTH;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_IN_PROCESS;
                    enable_dismiss = false;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Begin Calculating Tare Calibration Ground Truth");
                }
            }
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_IN_PROCESS)
            {
                enable_dismiss = false;
                ImGui::Text("%s", "Calculating ground truth is in process...\nKeep camera stationary pointing at the target");
            }
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_COMPLETE)
            {
                get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB;
                update_state = update_state_prev;
                if (get_manager()._sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                    get_manager()._sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, get_manager().laser_status_prev);          
            }
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_FAILED)
            {
                ImGui::Text("%s", _error_message.c_str());

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                ImGui::PushStyleColor(ImGuiCol_Button, saturate(redish, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(redish, 1.5f));

                std::string button_name = to_string() << "Retry" << "##retry" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_IN_PROCESS;
                    enable_dismiss = false;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Retry calculating ground truth");
                }
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
                    if (update_state == RS2_CALIB_STATE_TARE_INPUT)
                        ImGui::SetTooltip("%s", "More Options...");
                    else
                        ImGui::SetTooltip("%s", "Less Options...");
                }

                ImGui::PopStyleColor(2);
                if (update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                {
                    ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                    ImGui::Text("%s", "Avg Step Count:");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", "Number of frames to average, Min = 1, Max = 30, Default = 20");
                    }
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });

                    std::string id = to_string() << "##avg_step_count_" << index;
                    ImGui::PushItemWidth(width - 145.f);
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

                    ImGui::PushItemWidth(width - 145.f);
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

                    ImGui::PushItemWidth(width - 145.f);
                    ImGui::Combo(id.c_str(), &get_manager().accuracy, vals_cstr.data(), int(vals.size()));

                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 35 + ImGui::GetTextLineHeightWithSpacing()) });

                    ImGui::PopItemWidth();

                    draw_intrinsic_extrinsic(x, y + 3 * int(ImGui::GetTextLineHeightWithSpacing()) - 10);

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
                    ImGui::Text("%s", "Ground Truth (mm):");
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Distance in millimeter to the flat wall, between 60 and 10000.");

                std::string id = to_string() << "##ground_truth_for_tare" << index;
                get_manager().ground_truth = config_file::instance().get_or_default(configurations::viewer::ground_truth_r, 1200.0f);

                std::string gt = to_string() << get_manager().ground_truth;
                const int MAX_SIZE = 256;
                char buff[MAX_SIZE];
                memcpy(buff, gt.c_str(), gt.size() + 1);

                ImGui::PushItemWidth(width - 196.f);
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

                std::string get_button_name = to_string() << "Get" << "##tare" << index;
                if (update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED)
                    ImGui::SetCursorScreenPos({ float(x + width - 52), float(y + 58 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                else
                    ImGui::SetCursorScreenPos({ float(x + width - 52), float(y + 30) });

                if (ImGui::Button(get_button_name.c_str(), { 42.0f, 20.f }))
                {
                    if (get_manager()._sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                        get_manager().laser_status_prev = get_manager()._sub->s->get_option(RS2_OPTION_EMITTER_ENABLED);

                    update_state_prev = update_state;
                    update_state = RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH;
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculate ground truth for the specific target");

                std::string button_name = to_string() << "Calibrate" << "##tare" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([](std::function<void()> a) { a(); });
                    get_manager().reset();
                    get_manager().retry_times = 0;
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
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

                std::vector<const char*> vals_cstr;
                if (get_manager().action != on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB)
                {
                    std::vector<std::string> vals{ "Fast", "Slow", "White Wall" };
                    for (auto&& s : vals) vals_cstr.push_back(s.c_str());

                    ImGui::PushItemWidth(width - 145.f);
                    ImGui::Combo(id.c_str(), &get_manager().speed_fl, vals_cstr.data(), int(vals.size()));
                    ImGui::PopItemWidth();

                }
                else
                {
                    std::vector<std::string> vals{ "Very Fast", "Fast", "Medium", "Slow", "White Wall" };
                    for (auto&& s : vals) vals_cstr.push_back(s.c_str());

                    ImGui::PushItemWidth(width - 145.f);
                    ImGui::Combo(id.c_str(), &get_manager().speed, vals_cstr.data(), int(vals.size()));
                    ImGui::PopItemWidth();
                }

                if (get_manager().action != on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
                    draw_intrinsic_extrinsic(x, y);

                if (get_manager().action != on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB)
                {
                    float tmp_y = (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB ?
                        float(y + 40 + 2 * ImGui::GetTextLineHeightWithSpacing()) : float(y + 35 + ImGui::GetTextLineHeightWithSpacing()));
                    ImGui::SetCursorScreenPos({ float(x + 9), tmp_y });
                    id = to_string() << "##restore_" << index;
                    bool restore = (get_manager().adjust_both_sides == 1);
                    if (ImGui::Checkbox("Adjust both sides focal length", &restore))
                        get_manager().adjust_both_sides = (restore ? 1 : 0);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", "check = adjust both sides, uncheck = adjust right side only");
                }

                float tmp_y = (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB ? 
                    float(y + 45 + 3 * ImGui::GetTextLineHeightWithSpacing()) : float(y + 40 + 2 * ImGui::GetTextLineHeightWithSpacing()));
                ImGui::SetCursorScreenPos({ float(x + 9),  tmp_y });
                if (ImGui::RadioButton("OCC", (int*)&(get_manager().action), 1))
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "On-Chip Calibration");

                ImGui::SetCursorScreenPos({ float(x + 135),  tmp_y });
                if (ImGui::RadioButton("OCC Extended", (int *)&(get_manager().action), 0))
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "On-Chip Calibration Extended");

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                std::string button_name = to_string() << "Calibrate" << "##self" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
                    get_manager().retry_times = 0;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {_this->invoke(action);};
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                }

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Begin On-Chip Calibration");
            }
            else if (update_state == RS2_CALIB_STATE_FL_INPUT)
            {
                ImGui::SetCursorScreenPos({ float(x + 15), float(y + 33) });
                ImGui::Text("%s", "Please start left and right streaming with\nresolution 256x144 and adjust camera\nposition if necessary to make sure the target\nis in the middle of both left and right images.");

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + height - 25) });
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                std::string button_name = to_string() << "Calibrate" << "##fl" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    if (get_manager()._sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                        get_manager().laser_status_prev = get_manager()._sub->s->get_option(RS2_OPTION_EMITTER_ENABLED);
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
                    get_manager().retry_times = 0;
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Start focal length calibration after setting up camera position correctly.");
                ImGui::PopStyleColor(2);
            }
            else if (update_state == RS2_CALIB_STATE_FAILED)
            {
                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB
                    || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
                {           
                    if (get_manager().retry_times < 3)
                    {
                        get_manager().restore_workspace([](std::function<void()> a){ a(); });
                        get_manager().reset();
                        ++get_manager().retry_times;
                        get_manager().toggle = true;

                        auto _this = shared_from_this();
                        auto invoke = [_this](std::function<void()> action) {_this->invoke(action);};
                        get_manager().start(invoke);
                        update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                        enable_dismiss = false;
                    }
                    else
                    {
                        ImGui::Text("%s", (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB ? "OCC FL calibraton cannot work with this camera!" : "OCC Extended calibraton cannot work with this camera!"));
                    }
                }
                else
                {
                    ImGui::Text("%s", _error_message.c_str());

                    auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                    ImGui::PushStyleColor(ImGuiCol_Button, saturate(redish, sat));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(redish, 1.5f));

                    std::string button_name = to_string() << "Retry" << "##retry" << index;
                    ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                    if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                    {
                        get_manager().restore_workspace([](std::function<void()> a) { a(); });
                        get_manager().reset();
                        auto _this = shared_from_this();
                        auto invoke = [_this](std::function<void()> action) {
                            _this->invoke(action);
                        };
                        get_manager().start(invoke);
                        update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                        enable_dismiss = false;
                    }

                    ImGui::PopStyleColor(2);

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", "Retry on-chip calibration process");
                }
            }
            else if (update_state == RS2_CALIB_STATE_CALIB_COMPLETE)
            {
                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB)
                {
                    if (get_manager()._sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                        get_manager()._sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, get_manager().laser_status_prev);
                }
            
                auto health = get_manager().get_health();

                auto recommend_keep = fabs(health) < 0.25f;
                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
                    recommend_keep = fabs(health) < 0.15f;

                float health_1 = -1.0f;
                float health_2 = -1.0f;
                bool recommend_keep_1 = false;
                bool recommend_keep_2 = false;
                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
                {
                    health_1 = get_manager().get_health_1();
                    health_2 = get_manager().get_health_2();
                    recommend_keep_1 = fabs(health_1) < 0.25f;
                    recommend_keep_2 = fabs(health_2) < 0.15f;
                    recommend_keep = (recommend_keep_1 && recommend_keep_2);
                }

                ImGui::SetCursorScreenPos({ float(x + 10), float(y + 33) });

                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
                {
                    health_1 = get_manager().get_health_1();
                    health_2 = get_manager().get_health_2();

                    ImGui::Text("%s", "Health-Check Before Calibration: ");

                    std::stringstream ss_1;
                    ss_1 << std::fixed << std::setprecision(4) << health_1 << "%";
                    auto health_str = ss_1.str();

                    std::string text_name = to_string() << "##notification_text_1_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 225), float(y + 30) });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(health_str.c_str()), strlen(health_str.c_str()) + 1, { 86, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", "Health-check number before Tare Calibration");

                    ImGui::SetCursorScreenPos({ float(x + 10), float(y + 38) + ImGui::GetTextLineHeightWithSpacing() });
                    ImGui::Text("%s", "Health-Check After Calibration: ");

                    std::stringstream ss_2;
                    ss_2 << std::fixed << std::setprecision(4) << health_2 << "%";
                    health_str = ss_2.str();

                    text_name = to_string() << "##notification_text_2_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 225), float(y + 35) + ImGui::GetTextLineHeightWithSpacing() });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(health_str.c_str()), strlen(health_str.c_str()) + 1, { 86, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", "Health-check number after Tare Calibration");
                }
                else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
                {
                    ImGui::Text("%s", "Health-Check: ");

                    std::stringstream ss_1;
                    ss_1 << std::fixed << std::setprecision(2) << health_1;
                    auto health_str = ss_1.str();

                    std::string text_name = to_string() << "##notification_text_1_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 125), float(y + 30) });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(health_str.c_str()), strlen(health_str.c_str()) + 1, { 66, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    ImGui::SetCursorScreenPos({ float(x + 177), float(y + 33) });

                    if (recommend_keep_1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                        ImGui::Text("%s", "(Good)");
                    }
                    else if (fabs(health_1) < 0.75f)
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
                        ImGui::SetTooltip("%s", "OCC Health-Check captures how far camera calibration is from the optimal one\n"
                            "[0, 0.25) - Good\n"
                            "[0.25, 0.75) - Can be Improved\n"
                            "[0.75, ) - Requires Calibration");
                    }

                    ImGui::SetCursorScreenPos({ float(x + 10), float(y + 38) + ImGui::GetTextLineHeightWithSpacing() });
                    ImGui::Text("%s", "FL Health-Check: ");

                    std::stringstream ss_2;
                    ss_2 << std::fixed << std::setprecision(2) << health_2;
                    health_str = ss_2.str();

                    text_name = to_string() << "##notification_text_2_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 125), float(y + 35) + ImGui::GetTextLineHeightWithSpacing() });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(health_str.c_str()), strlen(health_str.c_str()) + 1, { 66, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    ImGui::SetCursorScreenPos({ float(x + 175), float(y + 38) + ImGui::GetTextLineHeightWithSpacing() });

                    if (recommend_keep_2)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                        ImGui::Text("%s", "(Good)");
                    }
                    else if (fabs(health_2) < 0.75f)
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
                        ImGui::SetTooltip("%s", "OCC-FL Health-Check captures how far camera calibration is from the optimal one\n"
                            "[0, 0.15) - Good\n"
                            "[0.15, 0.75) - Can be Improved\n"
                            "[0.75, ) - Requires Calibration");
                    }
                }
                else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB)
                {
                    ImGui::Text("%s", "Focal Length Imbalance: ");

                    std::stringstream ss_1;
                    ss_1 << std::fixed << std::setprecision(3) << get_manager().corrected_ratio;
                    auto ratio_str = ss_1.str();
                    ratio_str += " %";

                    std::string text_name = to_string() << "##notification_text_1_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 175), float(y + 30) });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(ratio_str.c_str()), strlen(ratio_str.c_str()) + 1, { 86, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    ImGui::SetCursorScreenPos({ float(x + 10), float(y + 38) + ImGui::GetTextLineHeightWithSpacing() });
                    ImGui::Text("%s", "Estimated Tilt Angle: ");

                    std::stringstream ss_2;
                    ss_2 << std::fixed << std::setprecision(3) << get_manager().tilt_angle;
                    auto align_str = ss_2.str();
                    align_str += " deg";

                    text_name = to_string() << "##notification_text_2_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 175), float(y + 35) + ImGui::GetTextLineHeightWithSpacing() });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(align_str.c_str()), strlen(align_str.c_str()) + 1, { 86, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);
                }
                else
                {
                    ImGui::Text("%s", (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB ? "Health-Check: " : "FL Health-Check: "));

                    std::stringstream ss; ss << std::fixed << std::setprecision(2) << health;
                    auto health_str = ss.str();

                    std::string text_name = to_string() << "##notification_text_" << index;

                    ImGui::SetCursorScreenPos({ float(x + 125), float(y + 30) });
                    ImGui::PushStyleColor(ImGuiCol_Text, white);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, transparent);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, transparent);
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                    ImGui::InputTextMultiline(text_name.c_str(), const_cast<char*>(health_str.c_str()), strlen(health_str.c_str()) + 1, { 66, ImGui::GetTextLineHeight() + 6 }, ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(7);

                    ImGui::SetCursorScreenPos({ float(x + 177), float(y + 33) });

                    if (recommend_keep)
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
                        if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB)
                        {
                            ImGui::SetTooltip("%s", "Calibration Health-Check captures how far camera calibration is from the optimal one\n"
                                "[0, 0.25) - Good\n"
                                "[0.25, 0.75) - Can be Improved\n"
                                "[0.75, ) - Requires Calibration");
                        }
                        else
                        {
                            ImGui::SetTooltip("%s", "Calibration Health-Check captures how far camera calibration is from the optimal one\n"
                                "[0, 0.15) - Good\n"
                                "[0.15, 0.75) - Can be Improved\n"
                                "[0.75, ) - Requires Calibration");
                        }
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
                        txt = to_string() << "  Fill-Rate: " << std::setprecision(1) << std::fixed << old_fr << "%%\n";

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
                    ImGui::SetCursorScreenPos({ float(x + 7), (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB ? float(y + 105) + ImGui::GetTextLineHeightWithSpacing() : float(y + 100)) });
                    ImGui::Text("%s", "Please compare new vs old calibration\nand decide if to keep or discard the result...");
                }

                ImGui::SetCursorScreenPos({ float(x + 20), (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB ? float(y + 70) + ImGui::GetTextLineHeightWithSpacing() : float(y + 60)) });

                if (ImGui::RadioButton("New", use_new_calib))
                {
                    use_new_calib = true;
                    get_manager().apply_calib(true);
                }

                ImGui::SetCursorScreenPos({ float(x + 150), (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB ? float(y + 70) + ImGui::GetTextLineHeightWithSpacing() : float(y + 60)) });
                if (ImGui::RadioButton("Original", !use_new_calib))
                {
                    use_new_calib = false;
                    get_manager().apply_calib(false);
                }

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                if (!recommend_keep || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                }
                
                float scale = float(bar_width) / 3;
                std::string button_name;
                
                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB)
                {
                    scale = float(bar_width) / 7;

                    button_name = to_string() << "Recalibrate" << "##refl" << index;

                    ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                    if (ImGui::Button(button_name.c_str(), { scale * 3, 20.f }))
                    {
                        if (get_manager()._sub->s->supports(RS2_OPTION_EMITTER_ENABLED))
                            get_manager().laser_status_prev = get_manager()._sub->s->get_option(RS2_OPTION_EMITTER_ENABLED);
                        get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                        get_manager().reset();
                        get_manager().retry_times = 0;
                        get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB;
                        auto _this = shared_from_this();
                        auto invoke = [_this](std::function<void()> action) {
                            _this->invoke(action);
                        };
                        get_manager().start(invoke);
                        update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                        enable_dismiss = false;
                    }

                    ImGui::SetCursorScreenPos({ float(x + 5) + 4 * scale, float(y + height - 25) });
                }
                else
                    ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });

                button_name = to_string() << "Apply New" << "##apply" << index;
                if (!use_new_calib) button_name = to_string() << "Keep Original" << "##original" << index;

                if (ImGui::Button(button_name.c_str(), { scale * 3, 20.f }))
                {
                    if (use_new_calib)
                    {
                        get_manager().keep();
                        update_state = RS2_CALIB_STATE_COMPLETE;
                        pinned = false;
                        enable_dismiss = false;
                        _progress_bar.last_progress_time = last_interacted = system_clock::now() + milliseconds(500);
                    }
                    else
                    {
                        dismiss(false);
                    }

                    get_manager().restore_workspace([](std::function<void()> a) { a(); });
                }

                if (!recommend_keep || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
                    ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "New calibration values will be saved in device");
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
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };

                    if (!update_manager->started()) update_manager->start(invoke);

                    update_state = RS2_CALIB_STATE_CALIB_IN_PROCESS;
                    enable_dismiss = false;
                    _progress_bar.last_progress_time = system_clock::now();
                }
                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Keep the camera pointing at an object or a wall");
                }
            }
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_IN_PROCESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_COMPLETE;
                    enable_dismiss = true;
                }

                if (update_manager->failed())
                {
                    update_manager->check_error(_error_message);
                    update_state = RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_FAILED;
                    enable_dismiss = true;
                }

                draw_progress_bar(win, bar_width);
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

        get_manager().restore_workspace([](std::function<void()> a){ a(); });

        if (update_state != RS2_CALIB_STATE_TARE_INPUT)
            update_state = RS2_CALIB_STATE_INITIAL_PROMPT;

        get_manager().turn_roi_off();
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

        std::string title;
        if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB)
            title = "On-Chip Focal Length Calibration";
        else if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB)
            title = "On-Chip Calibration Extended";
        else
            title = "On-Chip Calibration";
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
                        update_state = RS2_CALIB_STATE_FAILED;

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
            if (get_manager().allow_calib_keep()) return (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_FL_CALIB ? 190 : 170);
            else return 80;
        }
        else if (update_state == RS2_CALIB_STATE_SELF_INPUT) return (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB ? 160 : 140);
        else if (update_state == RS2_CALIB_STATE_TARE_INPUT) return 85;
        else if (update_state == RS2_CALIB_STATE_TARE_INPUT_ADVANCED) return 210;
        else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH) return 110;
        else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_FAILED) return 115;
        else if (update_state == RS2_CALIB_STATE_FAILED) return ((get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_OB_CALIB || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_FL_CALIB) ? (get_manager().retry_times < 3 ? 0 : 80) : 110);
        else if (update_state == RS2_CALIB_STATE_FL_INPUT || update_state == RS2_CALIB_STATE_UVMAPPING_INPUT) return 135;
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

    int rect_calculator::calculate(const rs2_frame* frame_ref, float rect_sides[4])
    {
        static int reset_counter = 0;
        if (reset_counter > _reset_limit)
        {
            reset_counter = 0;
            _rec_idx = 0;
            _rec_num = 0;
        }

        int ret = 0;
        rs2_error* e = nullptr;
        rs2_extract_target_dimensions(frame_ref, RS2_CALIB_TARGET_RECT_GAUSSIAN_DOT_VERTICES, _rec_sides[_rec_idx], 4, &e);
        if (e == nullptr)
        {
            ret = 1;
            reset_counter = 0;
            _rec_idx = (++_rec_idx) % _frame_num;
            ++_rec_num;

            if (_rec_num == _frame_num)
            {
                ret = 2;
                calculate_rect_sides(rect_sides);
                --_rec_num;
            }
        }
        else
            ++reset_counter;

        return ret;
    }

    void rect_calculator::calculate_rect_sides(float rect_sides[4])
    {
        for (int i = 0; i < 4; ++i)
            rect_sides[i] = _rec_sides[0][i];

        for (int j = 1; j < _frame_num; ++j)
        {
            for (int i = 0; i < 4; ++i)
                rect_sides[i] += _rec_sides[j][i];
        }

        for (int i = 0; i < 4; ++i)
            rect_sides[i] /= _frame_num;
    }

    int dots_calculator::calculate(const rs2_frame* frame_ref, float dots_x[4], float dots_y[4])
    {
        static int reset_counter = 0;
        if (reset_counter > _reset_limit)
        {
            reset_counter = 0;
            _rec_idx = 0;
            _rec_num = 0;
        }

        int ret = 0;
        rs2_error* e = nullptr;
        float dim[8] = { 0 };
        rs2_extract_target_dimensions(frame_ref, RS2_CALIB_TARGET_RECT_GAUSSIAN_DOT_VERTICES, dim, 8, &e);
        if (e == nullptr)
        {
            ret = 1;
            int j = 0;
            for (int i = 0; i < 4; ++i)
            {
                j = i << 1;
                _dots_x[_rec_idx][i] = dim[j];
                _dots_y[_rec_idx][i] = dim[j + 1];
            }

            reset_counter = 0;
            _rec_idx = (++_rec_idx) % _frame_num;
            ++_rec_num;

            if (_rec_num == _frame_num)
            {
                ret = 2;
                calculate_dots_position(dots_x, dots_y);
                --_rec_num;
            }
        }
        else
            ++reset_counter;

        return ret;
    }

    void dots_calculator::calculate_dots_position(float dots_x[4], float dots_y[4])
    {
        for (int i = 0; i < 4; ++i)
        {
            dots_x[i] = _dots_x[0][i];
            dots_y[i] = _dots_y[0][i];
        }

        for (int j = 1; j < _frame_num; ++j)
        {
            for (int i = 0; i < 4; ++i)
            {
                dots_x[i] += _dots_x[j][i];
                dots_y[i] += _dots_y[j][i];
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            dots_x[i] /= _frame_num;
            dots_y[i] /= _frame_num;
        }
    }

    uvmapping_calib::uvmapping_calib(int pt_num, const float* left_x, const float* left_y, const float* left_z, const float* color_x, const float* color_y, const rs2_intrinsics& left_intrin, const rs2_intrinsics& color_intrin, rs2_extrinsics& extrin)
        : _pt_num(pt_num)
    {
        for (int i = 0; i < pt_num; ++i)
        {
            _left_x.emplace_back(left_x[i]);
            _left_y.emplace_back(left_y[i]);
            _left_x.emplace_back(left_z[i]);
            _color_x.emplace_back(color_x[i]);
            _color_y.emplace_back(color_y[i]);
        }

        memmove(&_left_intrin, &left_intrin, sizeof(rs2_intrinsics));
        memmove(&_color_intrin, &color_intrin, sizeof(rs2_intrinsics));
        memmove(&_extrin, &extrin, sizeof(rs2_extrinsics));
    }

}
