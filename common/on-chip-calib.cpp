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
#include "../tools/depth-quality/depth-metrics.h"
#include "../src/ds5/ds5-private.h"

namespace rs2
{
    on_chip_calib_manager::~on_chip_calib_manager()
    {
        if (fl_viewer_status == 3)
            _sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, laser_status_prev);
    }

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

    void on_chip_calib_manager::start_fl_viewer()
    {
        if (fl_viewer_status == 3)
            return;

        try
        {
            // stop
            if (fl_viewer_status == 0)
            {
                auto invoke = [this](std::function<void()> a) { a(); };
                if (!_sub->streaming)
                    start_viewer(0, 0, 0, invoke);

                stop_viewer(invoke);

                if (_ui.get())
                {
                    _sub->ui = *_ui;
                    _ui.reset();
                }

                reset();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                ++fl_viewer_status;
            }
            else if (fl_viewer_status == 1)
                ++fl_viewer_status;
            else if (fl_viewer_status == 2)
            {
                // Configuration
                int w = 256;
                int h = 144;
                int fps = 90;

                _sub->stream_enabled[0] = false;
                _sub->stream_enabled[1] = true;
                _sub->stream_enabled[2] = true;

                _viewer.is_3d_view = false;
                _viewer.synchronization_enable = false;

                laser_status_prev = _sub->s->get_option(RS2_OPTION_EMITTER_ENABLED);
                _sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, 0.0f);

                // Select only left infrared Y8 stream
                _sub->ui.selected_format_id.clear();
                for (int i = 0; i < _sub->format_values[1].size(); i++)
                {
                    if (_sub->format_values[1][i] == RS2_FORMAT_Y8)
                    {
                        _sub->ui.selected_format_id[1] = i;
                        _sub->ui.selected_format_id[2] = i;
                        break;
                    }
                }

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

                auto profiles = _sub->get_selected_profiles();

                // Start streaming
                _sub->play(profiles, _viewer, _model.dev_syncer);
                for (auto&& profile : profiles)
                    _viewer.begin_stream(_sub, profile);

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

                ++fl_viewer_status;
            }
        }
        catch (...) {}
    }

    void on_chip_calib_manager::start_viewer(int w, int h, int fps, invoker invoke)
    {
        try
        {
            if (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
            {
                _sub->stream_enabled[0] = false;
                _sub->stream_enabled[1] = true;
                _sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, 0.0f);

                // Select only left infrared Y8 stream
                _sub->ui.selected_format_id.clear();
                for (int i = 0; i < _sub->format_values[1].size(); i++)
                {
                    if (_sub->format_values[1][i] == RS2_FORMAT_Y8)
                    {
                        _sub->ui.selected_format_id[1] = i;
                        break;
                    }
                }
            }
            else
            {
                _sub->stream_enabled[0] = true;
                _sub->stream_enabled[1] = false;

                if (_ui) _sub->ui = *_ui; // Save previous configuration

                // Select only depth stream
                _sub->ui.selected_format_id.clear();
                _sub->ui.selected_format_id[RS2_STREAM_DEPTH] = 0;
            }

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
        std::stringstream ss;
        ss << "{\n \"speed\":" << speed <<
               ",\n \"average step count\":" << average_step_count <<
               ",\n \"scan parameter\":" << (intrinsic_scan ? 0 : 1) <<
               ",\n \"step count\":" << step_count <<
               ",\n \"apply preset\":" << (apply_preset ? 1 : 0) <<
               ",\n \"accuracy\":" << accuracy <<"}";

        std::string json = ss.str();
        

        auto calib_dev = _dev.as<auto_calibrated_device>();
        if (action == RS2_CALIB_ACTION_TARE_CALIB)
            _new_calib = calib_dev.run_tare_calibration(ground_truth, json, [&](const float progress) {_progress = int(progress);}, 5000);
        else
            _new_calib = calib_dev.run_on_chip_calibration(json, &_health, [&](const float progress) {_progress = int(progress); }, 5000);
    }

    void on_chip_calib_manager::get_ground_truth()
    {
        try
        {
            std::shared_ptr<tare_ground_truth_calculator> gt_calculator;
            bool created = false;

            int counter = 0;
            int limit = tare_ground_truth_calculator::_frame_num << 1;
            int step = 100 / tare_ground_truth_calculator::_frame_num;

            int ret = 0;
            rs2::frame f;
            while (counter < limit)
            {
                f = _viewer.ppf.frames_queue[1].wait_for_frame();
                if (f)
                {
                    if (!created)
                    {
                        stream_profile profile = f.get_profile();
                        auto vsp = profile.as<video_stream_profile>();

                        gt_calculator = std::make_shared<tare_ground_truth_calculator>(vsp.width(), vsp.height(), vsp.get_intrinsics().fx,
                            config_file::instance().get_or_default(configurations::viewer::target_width_r, 112.0f),
                            config_file::instance().get_or_default(configurations::viewer::target_height_r, 112.0f));
                        created = true;
                    }

                    ret = gt_calculator->calculate(reinterpret_cast<const uint8_t*> (f.get_data()), ground_truth);
                    if (ret == 0)
                        ++counter;
                    else if (ret == 1)
                        _progress += step;
                    else if (ret == 2)
                    {
                        _progress += step;
                        config_file::instance().set(configurations::viewer::ground_truth_r, ground_truth);
                        break;
                    }
                }
            }

            if (ret != 2)
                fail("Please adjust the camera position \nand make sure the specific target is \nin the middle of the camera image!");
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

    void on_chip_calib_manager::calculate_rect(bool first)
    {
        try
        {
            std::shared_ptr<tare_ground_truth_calculator> gt_calculator_l;
            std::shared_ptr<tare_ground_truth_calculator> gt_calculator_r;
            bool created_l = false;
            bool created_r = false;

            int counter = 0;
            int limit = tare_ground_truth_calculator::_frame_num << 2;
            int step = 50 / tare_ground_truth_calculator::_frame_num;

            int ret = 0;
            rs2::frame f;
            int done = 0;
            while (counter < limit && (done != 3))
            {
                f = _viewer.ppf.frames_queue[1].wait_for_frame();
                if (f && !(done & 1))
                {
                    if (!created_l)
                    {
                        stream_profile profile = f.get_profile();
                        auto vsp = profile.as<video_stream_profile>();

                        _focal_length_left_x = vsp.get_intrinsics().fx;
                        _focal_length_left_y = vsp.get_intrinsics().fy;
                        gt_calculator_l = std::make_shared<tare_ground_truth_calculator>(vsp.width(), vsp.height(), vsp.get_intrinsics().fx,
                            config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f),
                            config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f));

                        created_l = true;
                    }

                    ret = gt_calculator_l->calculate_rect(reinterpret_cast<const uint8_t*> (f.get_data()), (first ? left_rect_1 : left_rect_2));
                    if (ret == 0)
                        ++counter;
                    else if (ret == 1)
                        _progress += step;
                    else if (ret == 2)
                    {
                        _progress += step;
                        done |= 1;
                    }
                }

                f = _viewer.ppf.frames_queue[2].wait_for_frame();
                if (f)
                {
                    if (!created_r)
                    {
                        stream_profile profile = f.get_profile();
                        auto vsp = profile.as<video_stream_profile>();

                        _focal_length_right_x = vsp.get_intrinsics().fx;
                        _focal_length_right_y = vsp.get_intrinsics().fy;
                        gt_calculator_r = std::make_shared<tare_ground_truth_calculator>(vsp.width(), vsp.height(), vsp.get_intrinsics().fx,
                            config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f),
                            config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f));
                        created_r = true;
                    }

                    ret = gt_calculator_r->calculate_rect(reinterpret_cast<const uint8_t*> (f.get_data()), (first ? right_rect_1 : right_rect_2));
                    if (ret == 0)
                        ++counter;
                    else if (ret == 1)
                        _progress += step;
                    else if (ret == 2 && !(done & 2))
                    {
                        _progress += step;
                        done |= 2;
                    }
                }
            }

            if (done != 3)
                fail("Please adjust the camera position \nand make sure the specific target is \nalmost in the middle of both camera images!");
        }
        catch (const std::runtime_error& error)
        {
            fail(error.what());
        }
        catch (...)
        {
            fail("Calculatign target rectangle side length failed!");
        }
    }

    void on_chip_calib_manager::calculate_focal_length()
    {
        float target_width = config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f);
        float target_height = config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f);
        float delta_z = config_file::instance().get_or_default(configurations::viewer::delta_z_r, 200.0f);

        float s[4] = { 0 };
        for (int i = 0; i < 2; ++i)
            s[i] = target_width * abs(left_rect_1[i] - left_rect_2[i]);

        for (int i = 2; i < 4; ++i)
            s[i] = target_height * abs(left_rect_1[i] - left_rect_2[i]);

        float fl = 0.0f;
        if (s[0] > 0.1f && s[1] > 0.1f && s[2] > 0.1f && s[3] > 0.1f)
        {
            for (int i = 0; i < 4; ++i)
                fl += delta_z * left_rect_1[i] * left_rect_2[i] / s[i];

            focal_length_left = fl / 4;
        }

        for (int i = 0; i < 2; ++i)
            s[i] = target_width * abs(right_rect_1[i] - right_rect_2[i]);

        for (int i = 2; i < 4; ++i)
            s[i] = target_height * abs(right_rect_1[i] - right_rect_2[i]);

        fl = 0.0f;
        if (s[0] > 0.1f && s[1] > 0.1f && s[2] > 0.1f && s[3] > 0.1f)
        {
            for (int i = 0; i < 4; ++i)
                fl += delta_z * right_rect_1[i] * right_rect_2[i] / s[i];

            focal_length_right = fl / 4;
        }
    }

    void on_chip_calib_manager::calculate_focal_length_ratio()
    {
        float target_width = config_file::instance().get_or_default(configurations::viewer::target_width_r, 175.0f);
        float target_height = config_file::instance().get_or_default(configurations::viewer::target_height_r, 100.0f);

        float focal_length[4] = { 0 };
        focal_length[0] = ground_truth * left_rect_1[0] / target_width;
        focal_length[1] = ground_truth * left_rect_1[1] / target_width;
        focal_length[2] = ground_truth * left_rect_1[2] / target_height;
        focal_length[3] = ground_truth * left_rect_1[3] / target_height;

        focal_length_left = 0.0;
        for (int i = 0; i < 4; ++i)
            focal_length_left += focal_length[i];

        focal_length_left /= 4;

        focal_length[0] = ground_truth * right_rect_1[0] / target_width;
        focal_length[1] = ground_truth * right_rect_1[1] / target_width;
        focal_length[2] = ground_truth * right_rect_1[2] / target_height;
        focal_length[3] = ground_truth * right_rect_1[3] / target_height;

        focal_length_right = 0.0;
        for (int i = 0; i < 4; ++i)
            focal_length_right += focal_length[i];

        focal_length_right /= 4;

        float gt[4] = { 0 };
        gt[0] = _focal_length_left_x * target_width / left_rect_1[0];
        gt[1] = _focal_length_left_x * target_width / left_rect_1[1];
        gt[2] = _focal_length_left_y * target_height / left_rect_1[2];
        gt[3] = _focal_length_left_y * target_height / left_rect_1[3];

        float gt_left = 0.0;
        for (int i = 0; i < 4; ++i)
            gt_left += gt[i];

        gt_left /= 4;

        gt[0] = _focal_length_right_x * target_width / right_rect_1[0];
        gt[1] = _focal_length_right_x * target_width / right_rect_1[1];
        gt[2] = _focal_length_right_x * target_height / right_rect_1[2];
        gt[3] = _focal_length_right_x * target_height / right_rect_1[3];

        float gt_right = 0.0;
        for (int i = 0; i < 4; ++i)
            gt_right += gt[i];

        gt_right /= 4;

        if (gt_right > 0.1f)
            ratio = gt_left / gt_right;
    }

    void on_chip_calib_manager::process_flow(std::function<void()> cleanup,
        invoker invoke)
    {
        if (action == RS2_CALIB_ACTION_FOCAL_LENGTH_1PT)
        {
            calculate_rect(true);
            calculate_focal_length_ratio();
        }
        else if (action == RS2_CALIB_ACTION_FOCAL_LENGTH_2PT_1)
        {
            calculate_rect(true);
        }
        else if (action == RS2_CALIB_ACTION_FOCAL_LENGTH_2PT_2)
        {
            calculate_rect(false);
            calculate_focal_length();
        }
        else
        {
            update_last_used();

            log(to_string() << "Starting calibration at speed " << speed);

            _in_3d_view = _viewer.is_3d_view;
            _viewer.is_3d_view = (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH ? false : true);

            config_file::instance().set(configurations::viewer::ground_truth_r, ground_truth);

            auto calib_dev = _dev.as<auto_calibrated_device>();
            _old_calib = calib_dev.get_calibration_table();

            _was_streaming = _sub->streaming;
            _synchronized = _viewer.synchronization_enable.load();
            _post_processing = _sub->post_processing_enabled;
            _sub->post_processing_enabled = false;
            _viewer.synchronization_enable = false;

            _restored = false;

            if (action != RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
            {
                if (!_was_streaming)
                {
                    start_viewer(0, 0, 0, invoke);
                }

                // Capture metrics before
                auto metrics_before = get_depth_metrics(invoke);
                _metrics.push_back(metrics_before);
            }

            stop_viewer(invoke);

            _ui = std::make_shared<subdevice_ui_selection>(_sub->ui);

            // Switch into special Auto-Calibration mode
            start_viewer(256, 144, 90, invoke);

            if (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
                get_ground_truth();
            else
                calibrate();

            if (action == RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
                log(to_string() << "Tare ground truth is got: " << ground_truth);
            else
                log(to_string() << "Calibration completed, health factor = " << _health);

            stop_viewer(invoke);

            if (action != RS2_CALIB_ACTION_TARE_GROUND_TRUTH)
            {
                start_viewer(0, 0, 0, invoke); // Start with default settings

                // Make new calibration active
                apply_calib(true);

                // Capture metrics after
                auto metrics_after = get_depth_metrics(invoke);
                _metrics.push_back(metrics_after);
            }
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

            _viewer.ground_truth_r = uint32_t(ground_truth);
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

    void on_chip_calib_manager::apply_ratio()
    {
        if (ratio > 0.01f)
        {
            auto calib_dev = _dev.as<auto_calibrated_device>();
            calibration_table table_data = calib_dev.get_calibration_table();
            auto table = (librealsense::ds::coefficients_table*)table_data.data();
            table->intrinsic_right.x.x *= ratio;
            table->intrinsic_right.x.y *= ratio;
            auto actual_data = table_data.data() + sizeof(librealsense::ds::table_header);
            auto actual_data_size = table_data.size() - sizeof(librealsense::ds::table_header);
            auto crc = helpers::calc_crc32(actual_data, actual_data_size);
            table->header.crc32 = crc;
            calib_dev.set_calibration_table(table_data);
            ratio = 1.0f;
            applied = true;
        }
    }

    bool on_chip_calib_manager::keep_ratio()
    {
        if (applied)
        {
            auto calib_dev = _dev.as<auto_calibrated_device>();
            calib_dev.write_calibration();
            return true;
        }

        return false;
    }

    void autocalib_notification_model::draw_dismiss(ux_window& win, int x, int y)
    {
        using namespace std;
        using namespace chrono;

        auto health = get_manager().get_health();
        auto recommend_keep = fabs(health) > 0.25f;  
        if (!recommend_keep && update_state == RS2_CALIB_STATE_CALIB_COMPLETE && get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB)
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
            else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH)
                ImGui::Text("%s", "Get Tare Calibration Ground Truth");
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_INPUT ||
                update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_IN_PROCESS ||
                update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_COMPLETE)
                ImGui::Text("%s", "Calculate FL and Ratio with Specific Target");
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_INPUT ||
                     update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_IN_PROCESS ||
                     update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_COMPLETE)
                ImGui::Text("%s", "Calculate Focal Length with Specific Target");

            if (update_state == RS2_CALIB_STATE_FAILED)
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
                ImGui::PushItemWidth(float(width - 145));
                float target_width = config_file::instance().get_or_default(configurations::viewer::target_width_r, 112.0f);
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
                ImGui::PushItemWidth(float(width - 145));
                float target_height = config_file::instance().get_or_default(configurations::viewer::target_height_r, 112.0f);
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
                    get_manager()._sub->s->set_option(RS2_OPTION_EMITTER_ENABLED, get_manager().laser_status_prev);
                }

                ImGui::SetCursorScreenPos({ float(x + 85), float(y + height - 25) });
                std::string button_name = to_string() << "Calculate" << "##tare" << index;
                if (ImGui::Button(button_name.c_str(), { float(bar_width - 70), 20.f }))
                {
                    get_manager().restore_workspace([this](std::function<void()> a) { a(); });
                    get_manager().reset();
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
                    ImGui::Text("%s", "Ground Truth(mm):");
                    ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Tare depth in 1 / 100 of depth unit, Min = 2500, Max = 2000000");
                }
                

                //ImGui::SetCursorScreenPos({ float(x + 135), float(y + 45 + 3 * ImGui::GetTextLineHeightWithSpacing()) });

                std::string id = to_string() << "##ground_truth_for_tare" << index;

                get_manager().ground_truth = config_file::instance().get_or_default(configurations::viewer::ground_truth_r, 2500.0f);

                std::string gt = to_string() << get_manager().ground_truth;
                const int MAX_SIZE = 256;
                char buff[MAX_SIZE];
                memcpy(buff, gt.c_str(), gt.size() + 1);

                ImGui::PushItemWidth(float(width - 196));
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
                {
                    ImGui::SetCursorScreenPos({ float(x + width - 52), float(y + 58 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                }
                else
                {
                    ImGui::SetCursorScreenPos({ float(x + width - 52), float(y + 30) });
                }
                if (ImGui::Button(get_button_name.c_str(), { 42.0f, 20.f }))
                {
                    get_manager().laser_status_prev = get_manager()._sub->s->get_option(RS2_OPTION_EMITTER_ENABLED);
                    update_state_prev = update_state;
                    update_state = RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH;
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Calculate ground truth for the specific target");
                }
                
                std::string button_name = to_string() << "Calibrate" << "##tare" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                {
                    get_manager().restore_workspace([](std::function<void()> a){ a(); });
                    get_manager().reset();
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
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_INPUT)
            {
                get_manager().start_fl_viewer();

                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                ImGui::Text("%s", "Ground Truth(mm):");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Z gropund truth distance in millemeter.");
                }

                ImGui::SetCursorScreenPos({ float(x + 135), float(y + 30) });
                std::string id = to_string() << "##ground_truth_for_tare" << index;
                get_manager().ground_truth = config_file::instance().get_or_default(configurations::viewer::ground_truth_r, 1000.0f);
                std::string gt = to_string() << get_manager().ground_truth;
                const int MAX_SIZE = 256;
                char buff[MAX_SIZE];
                memcpy(buff, gt.c_str(), gt.size() + 1);
                ImGui::PushItemWidth(float(width - 196));
                if (ImGui::InputText(id.c_str(), buff, std::max((int)gt.size() + 1, 10)))
                {
                    std::stringstream ss;
                    ss << buff;
                    ss >> get_manager().ground_truth;
                    config_file::instance().set(configurations::viewer::ground_truth_r, get_manager().ground_truth);
                }
                ImGui::PopItemWidth();

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 38 + ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Target Width:");

                ImGui::SetCursorScreenPos({ float(x + 99), float(y + 36 + ImGui::GetTextLineHeightWithSpacing()) });
                id = to_string() << "##target_width_" << index;
                ImGui::PushItemWidth(float(40));
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

                ImGui::SetCursorScreenPos({ float(x + 178), float(y + 38 + ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Target Height:");

                ImGui::SetCursorScreenPos({ float(x + width - 48), float(y + 36 + ImGui::GetTextLineHeightWithSpacing()) });
                id = to_string() << "##target_height_" << index;
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

                ImGui::SetCursorScreenPos({ float(x + width - 216), float(y + 46 + 2 * ImGui::GetTextLineHeightWithSpacing()) });
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                std::string calc_button_1_name = to_string() << "Calculate" << "##tare" << index;
                if (ImGui::Button(calc_button_1_name.c_str(), { 208.0f, 20.f }))
                {
                    get_manager().reset();
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_FOCAL_LENGTH_1PT;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_1PT_IN_PROCESS;
                    enable_dismiss = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculate the target rectangle size, focal length, and relative ratio");

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 54 + 3 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Rectangle size:");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 57 + 4 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Left Rect:    %.2f; %.2f; %.2f; %.2f", get_manager().left_rect_1[0], get_manager().left_rect_1[1], get_manager().left_rect_1[2], get_manager().left_rect_1[3]);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculation result for left camera's position.");

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 60 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Right Rect: %.2f; %.2f; %.2f; %.2f", get_manager().right_rect_1[0], get_manager().right_rect_1[1], get_manager().right_rect_1[2], get_manager().right_rect_1[3]);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculation result for right camera's position.");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 65 + 6 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Focal Length:");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The focal length calculated using the ground truth.");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 68 + 7 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Left Focal Length:    %.4f", get_manager().focal_length_left);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The focal length calculated for left camera.");

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 71 + 8 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Right Focal Length: %.4f", get_manager().focal_length_right);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The focal length calculated for right camera.");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 76 + 9 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Ratio:");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The left calculated ground truth diveded by the right calculated ground truth.");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);

                ImGui::SetCursorScreenPos({ float(x + 50), float(y + 76 + 9 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%.4f", get_manager().ratio);

                ImGui::SetCursorScreenPos({ float(x + width - 216), float(y + 75 + 9 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                std::string apply_button_name = to_string() << "Apply" << "##tare" << index;
                if (ImGui::Button(apply_button_name.c_str(), { 100.0f, 20.f }))
                {
                    get_manager().apply_ratio();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Apply the ratio to the camera calibration table but not keep it permananently yet. You can redo calculatation to check the changes.");
                
                ImGui::SetCursorScreenPos({ float(x + width - 108), float(y + 75 + 9 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));
                std::string keep_button_name = to_string() << "Keep" << "##tare" << index;
                if (ImGui::Button(keep_button_name.c_str(), { 100.0f, 20.f }))
                {
                    if (get_manager().keep_ratio())
                        update_state = RS2_CALIB_STATE_FOCAL_LENGTH_1PT_COMPLETE;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Make the camera calibration table change permananently after \"Apply\" is triggered.");

                ImGui::PopStyleColor(2);
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_IN_PROCESS)
            {
                enable_dismiss = false;
                ImGui::Text("%s", "Calculation is in process...\nKeep camera stationary pointing at the target");
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_FAILED)
            {
                ImGui::Text("%s", _error_message.c_str());

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(redish, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(redish, 1.5f));

                std::string button_name = to_string() << "Retry" << "##retry" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_1PT_INPUT;

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Retry calculation");
                }
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_COMPLETE)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                ImGui::Text("%s", "Focal length is updated sucessfully!");
                ImGui::PopStyleColor(2);
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_INPUT)
            {
                get_manager().start_fl_viewer();

                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
            
                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 33) });
                ImGui::Text("%s", "First Position:");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Make sure the four dots are in almost middle of both left and right images");

                ImGui::PopStyleColor(2);
                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                ImGui::SetCursorScreenPos({ float(x + width - 188), float(y + 30) });
                std::string calc_button_1_name = to_string() << "Get first rectangle size" << "##tare" << index;
                if (ImGui::Button(calc_button_1_name.c_str(), { 180.0f, 20.f }))
                {
                    get_manager().reset();
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_FOCAL_LENGTH_2PT_1;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_2PT_IN_PROCESS;
                    enable_dismiss = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculate the target rectangle size at the first position");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);
                
                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 36 + ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Left Rect:    %.2f; %.2f; %.2f; %.2f", get_manager().left_rect_1[0], get_manager().left_rect_1[1], get_manager().left_rect_1[2], get_manager().left_rect_1[3]);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculation result for left camera's first position.");

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 39 + 2 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Right Rect: %.2f; %.2f; %.2f; %.2f", get_manager().right_rect_1[0], get_manager().right_rect_1[1], get_manager().right_rect_1[2], get_manager().right_rect_1[3]);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculation result for right camera's the first position.");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 47 + 3 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Delta Z:");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The two positions differs along Z direction only!");

                ImGui::SetCursorScreenPos({ float(x + width - 98), float(y + 45 + 3 * ImGui::GetTextLineHeightWithSpacing()) });
                std::string deltaZ = to_string() << "##deltaZ" << index;
                const int MAX_SIZE = 256;
                char buff[MAX_SIZE];
                float delta_z = config_file::instance().get_or_default(configurations::viewer::delta_z_r, 200.0f);
                std::string dz = to_string() << delta_z;
                memcpy(buff, dz.c_str(), dz.size() + 1);
                ImGui::PushItemWidth(90.0f);
                if (ImGui::InputText(deltaZ.c_str(), buff, strlen(buff) + 1))
                {
                    std::stringstream ss;
                    ss << buff;
                    ss >> delta_z;
                    config_file::instance().set(configurations::viewer::delta_z_r, delta_z);
                }
                ImGui::PopItemWidth();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Input the distance between two positions in millimeter.");

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 55 + 4 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Target Width:");

                ImGui::SetCursorScreenPos({ float(x + 99), float(y + 53 + 4 * ImGui::GetTextLineHeightWithSpacing()) });
                std::string id = to_string() << "##target_width_" << index;
                ImGui::PushItemWidth(float(40));
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

                ImGui::SetCursorScreenPos({ float(x + 178), float(y + 55 + 4 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Target Height:");

                ImGui::SetCursorScreenPos({ float(x + width - 48), float(y + 53 + 4 * ImGui::GetTextLineHeightWithSpacing()) });
                id = to_string() << "##target_height_" << index;
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

                ImGui::PopStyleColor(2);

                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 63 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Second Position:");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Make sure the four dots are in almost middle of both left and right images");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Button, saturate(sensor_header_light_blue, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(sensor_header_light_blue, 1.5f));

                ImGui::SetCursorScreenPos({ float(x + width - 188), float(y + 60 + 5 * ImGui::GetTextLineHeightWithSpacing()) });
                std::string calc_button_2_name = to_string() << "Get second rectangle size" << "##tare" << index;
                if (ImGui::Button(calc_button_2_name.c_str(), { 180.0f, 20.f }))
                {
                    get_manager().reset();
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_FOCAL_LENGTH_2PT_2;
                    auto _this = shared_from_this();
                    auto invoke = [_this](std::function<void()> action) {
                        _this->invoke(action);
                    };
                    get_manager().start(invoke);
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_2PT_IN_PROCESS;
                    enable_dismiss = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculate the target rectangle size at the second position");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_grey);

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 66 + 6 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Left Rect:    %.2f; %.2f; %.2f; %.2f", get_manager().left_rect_2[0], get_manager().left_rect_2[1], get_manager().left_rect_2[2], get_manager().left_rect_2[3]);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculation result for left camera's second position.");

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 69 + 7 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Right Rect: %.2f; %.2f; %.2f; %.2f", get_manager().right_rect_2[0], get_manager().right_rect_2[1], get_manager().right_rect_2[2], get_manager().right_rect_2[3]);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "Calculation result for right camera's the second position.");

                ImGui::PopStyleColor(2);
                ImGui::PushStyleColor(ImGuiCol_Text, white);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::SetCursorScreenPos({ float(x + 9), float(y + 77 + 8 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("%s", "Focal Length:");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The focal length will be calculated automatically after the two position calculations.");

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 80 + 9 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Left Focal Length:    %.4f", get_manager().focal_length_left);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The focal length calculated for left camera.");

                ImGui::SetCursorScreenPos({ float(x + 19), float(y + 83 + 10 * ImGui::GetTextLineHeightWithSpacing()) });
                ImGui::Text("Right Focal Length: %.4f", get_manager().focal_length_right);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", "The focal length calculated for right camera.");

                ImGui::PopStyleColor(2);
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_IN_PROCESS)
            {
                enable_dismiss = false;
                ImGui::Text("%s", "Calculating target rectangle sides is in process...\nKeep camera stationary pointing at the target");
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_FAILED)
            {
                ImGui::Text("%s", _error_message.c_str());

                auto sat = 1.f + sin(duration_cast<milliseconds>(system_clock::now() - created_time).count() / 700.f) * 0.1f;

                ImGui::PushStyleColor(ImGuiCol_Button, saturate(redish, sat));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, saturate(redish, 1.5f));

                std::string button_name = to_string() << "Retry" << "##retry" << index;

                ImGui::SetCursorScreenPos({ float(x + 5), float(y + height - 25) });
                if (ImGui::Button(button_name.c_str(), { float(bar_width), 20.f }))
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_2PT_INPUT;

                ImGui::PopStyleColor(2);

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", "Retry calculating rectangle sides");
                }
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_COMPLETE)
            {
                update_state = RS2_CALIB_STATE_FOCAL_LENGTH_2PT_INPUT;
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

                ImGui::PushItemWidth(width - 145.f);

                ImGui::Combo(id.c_str(), &get_manager().speed, vals_cstr.data(), int(vals.size()));
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
                    get_manager().action = on_chip_calib_manager::RS2_CALIB_ACTION_ON_CHIP_CALIB;
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
                    get_manager().restore_workspace([](std::function<void()> a){ a(); });
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
                {
                    ImGui::SetTooltip("%s", "Retry on-chip calibration process");
                }
            }
            else if (update_state == RS2_CALIB_STATE_CALIB_COMPLETE)
            {
                auto health = get_manager().get_health();

                auto recommend_keep = fabs(health) > 0.25f;

                ImGui::SetCursorScreenPos({ float(x + 15), float(y + 33) });

                if (get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
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

                if (recommend_keep || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
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
                        _progress_bar.last_progress_time = last_interacted = system_clock::now() + milliseconds(500);
                    }
                    else dismiss(false);

                    get_manager().restore_workspace([](std::function<void()> a) { a(); });
                }
                if (recommend_keep || get_manager().action == on_chip_calib_manager::RS2_CALIB_ACTION_TARE_CALIB)
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
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_IN_PROCESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_1PT_INPUT;
                    enable_dismiss = true;
                }

                if (update_manager->failed())
                {
                    update_manager->check_error(_error_message);
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_1PT_FAILED;
                    enable_dismiss = true;
                }

                draw_progress_bar(win, bar_width);
            }
            else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_IN_PROCESS)
            {
                if (update_manager->done())
                {
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_2PT_COMPLETE;
                    enable_dismiss = true;
                }

                if (update_manager->failed())
                {
                    update_manager->check_error(_error_message);
                    update_state = RS2_CALIB_STATE_FOCAL_LENGTH_2PT_FAILED;
                    enable_dismiss = true;
                }

                draw_progress_bar(win, bar_width);
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
        else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH) return 110;
        else if (update_state == RS2_CALIB_STATE_GET_TARE_GROUND_TRUTH_FAILED) return 115;
        else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_INPUT) return 310;
        else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_COMPLETE) return 115;
        else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_1PT_FAILED) return 115;
        else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_INPUT) return 315;
        else if (update_state == RS2_CALIB_STATE_FOCAL_LENGTH_2PT_FAILED) return 115;
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

    tare_ground_truth_calculator::tare_ground_truth_calculator(int width, int height, float focal_length, float target_width, float target_height)
        : _width(width), _height(height), _target_fw(focal_length* target_width), _target_fh(focal_length* target_height)
    {
        if (width != 256 || height != 144)
            throw std::runtime_error(to_string() << "Only 256x144 resolution is supported!");

        _size = _width * _height;
        _sizeq = _size >> 2;

        _hwidth = _width >> 1;
        _hheight = _height >> 1;

        _hwt = _hwidth - _tsize;
        _hht = _hheight - _tsize;

        for (int i = 0; i < 4; ++i)
        {
            _imgt[i].resize(_tsize2);

            _nccq[i].resize(_sizeq);
            memset(_nccq[i].data(), 0, _sizeq * sizeof(float));

            _imgq[i].resize(_sizeq);
            memset(_imgq[i].data(), 0, _sizeq * sizeof(float));

            _buf[i].resize(40);
        }
    }

    tare_ground_truth_calculator::~tare_ground_truth_calculator()
    {
    }

    int tare_ground_truth_calculator::calculate_rect(const uint8_t* img, float rect_sides[4])
    {
        pre_calculate(img, _thresh, _patch_size);
        return run(rect_sides);
    }

    int tare_ground_truth_calculator::calculate(const uint8_t* img, float& ground_truth)
    {
        pre_calculate(img, _thresh, _patch_size);
        return run(ground_truth);
    }

    void tare_ground_truth_calculator::normalize_0(const uint8_t* img)
    {
        uint8_t min_val = 255;
        uint8_t max_val = 0;
        const uint8_t* p = img;
        for (int j = 0; j < _hheight; ++j)
        {
            for (int i = 0; i < _hwidth; ++i)
            {
                if (*p < min_val)
                    min_val = *p;

                if (*p > max_val)
                    max_val = *p;

                ++p;
            }

            p += _hwidth;
        }

        if (max_val > min_val)
        {
            float factor = 1.0f / (max_val - min_val);

            float* q = _imgq[0].data();
            p = img;
            for (int j = 0; j < _hheight; ++j)
            {
                for (int i = 0; i < _hwidth; ++i)
                    *q++ = 1.0f - (*p++ - min_val) * factor;

                p += _hwidth;
            }
        }
    }

    void tare_ground_truth_calculator::normalize_1(const uint8_t* img)
    {
        uint8_t min_val = 255;
        uint8_t max_val = 0;
        const uint8_t* p = img;
        for (int j = 0; j < _hheight; ++j)
        {
            p += _hwidth;
            for (int i = 0; i < _hwidth; ++i)
            {
                if (*p < min_val)
                    min_val = *p;

                if (*p > max_val)
                    max_val = *p;

                ++p;
            }
        }

        if (max_val > min_val)
        {
            float factor = 1.0f / (max_val - min_val);

            float* q = _imgq[1].data();
            p = img;
            for (int j = 0; j < _hheight; ++j)
            {
                p += _hwidth;
                for (int i = 0; i < _hwidth; ++i)
                    *q++ = 1.0f - (*p++ - min_val) * factor;
            }
        }
    }

    void tare_ground_truth_calculator::normalize_2(const uint8_t* img)
    {
        uint8_t min_val = 255;
        uint8_t max_val = 0;
        int pos = _hheight * _width;;
        const uint8_t* p = img + pos;
        for (int j = 0; j < _hheight; ++j)
        {
            for (int i = 0; i < _hwidth; ++i)
            {
                if (*p < min_val)
                    min_val = *p;

                if (*p > max_val)
                    max_val = *p;

                ++p;
            }

            p += _hwidth;
        }

        if (max_val > min_val)
        {
            float factor = 1.0f / (max_val - min_val);

            float* q = _imgq[2].data();
            p = img + pos;
            for (int j = 0; j < _hheight; ++j)
            {
                for (int i = 0; i < _hwidth; ++i)
                    *q++ = 1.0f - (*p++ - min_val) * factor;

                p += _hwidth;
            }
        }
    }

    void tare_ground_truth_calculator::normalize_3(const uint8_t* img)
    {
        uint8_t min_val = 255;
        uint8_t max_val = 0;
        int pos = _hheight * _width;;
        const uint8_t* p = img + pos;
        for (int j = 0; j < _hheight; ++j)
        {
            p += _hwidth;
            for (int i = 0; i < _hwidth; ++i)
            {
                if (*p < min_val)
                    min_val = *p;

                if (*p > max_val)
                    max_val = *p;

                ++p;
            }
        }

        if (max_val > min_val)
        {
            float factor = 1.0f / (max_val - min_val);

            float* q = _imgq[3].data();
            p = img + pos;
            for (int j = 0; j < _hheight; ++j)
            {
                p += _hwidth;
                for (int i = 0; i < _hwidth; ++i)
                    *q++ = 1.0f - (*p++ - min_val) * factor;
            }
        }
    }

    void tare_ground_truth_calculator::find_corner(float thresh, int q, int patch_size)
    {
        _corners_found[q] = false;

        // ncc and the corner point
        float* pncc = _nccq[q].data() + (_htsize * _hwidth + _htsize);
        float* pi = _imgq[q].data();
        float* pit = _imgt[q].data();

        const float* pt = nullptr;
        const float* qi = nullptr;

        float sum = 0.0f;
        float mean = 0.0f;
        float norm = 0.0f;

        float peak = 0.0f;
        _pts[q].x = 0;
        _pts[q].y = 0;

        for (int j = 0; j < _hht; ++j)
        {
            for (int i = 0; i < _hwt; ++i)
            {
                qi = pi;
                sum = 0.0f;
                for (int m = 0; m < _tsize; ++m)
                {
                    for (int n = 0; n < _tsize; ++n)
                        sum += *qi++;

                    qi += _hwt;
                }

                mean = sum / _tsize2;

                qi = pi;
                sum = 0.0f;
                pit = _imgt[q].data();
                for (int m = 0; m < _tsize; ++m)
                {
                    for (int n = 0; n < _tsize; ++n)
                    {
                        *pit = *qi++ - mean;
                        sum += *pit * *pit;
                        ++pit;
                    }
                    qi += _hwt;
                }

                norm = sqrt(sum);

                pt = _template.data();
                pit = _imgt[q].data();
                sum = 0.0f;
                for (int k = 0; k < _tsize2; ++k)
                    sum += *pit++ * *pt++;

                *pncc = sum / norm;

                if (*pncc < thresh)
                    *pncc = 0;

                if (*pncc > peak)
                {
                    peak = *pncc;
                    _pts[q].x = i;
                    _pts[q].y = j;
                }

                ++pncc;
                ++pi;
            }

            pncc += _tsize;
            pi += _tsize;
        }

        // refine the corner point
        if (_pts[q].x > 0 && _pts[q].y > 0)
        {
            _corners_found[q] = true;
            _pts[q].x += _htsize;
            _pts[q].y += _htsize;

            float* f = _buf[q].data();
            int hs = patch_size >> 1;
            int pos = (_pts[q].y - hs) * _hwidth + _pts[q].x - hs;

            _corners[_corners_idx][q].x = static_cast<float>(_pts[q].x) - hs;
            minimize_x(_nccq[q].data() + pos, patch_size, f, _corners[_corners_idx][q].x);

            _corners[_corners_idx][q].y = static_cast<float>(_pts[q].y) - hs;
            minimize_y(_nccq[q].data() + pos, patch_size, f, _corners[_corners_idx][q].y);

            switch (q)
            {
            case 1:
                _corners[_corners_idx][q].x += _hwidth;
                break;

            case 2:
                _corners[_corners_idx][q].y += _hheight;
                break;

            case 3:
                _corners[_corners_idx][q].x += _hwidth;
                _corners[_corners_idx][q].y += _hheight;
                break;

            default:
                break;
            }
        }
    }

    void tare_ground_truth_calculator::pre_calculate(const uint8_t* img, float thresh, int patch_size)
    {
        std::vector<std::thread> threads;
        threads.emplace_back(std::thread([this, img, thresh, patch_size]() {normalize_0(img), find_corner(thresh, 0, patch_size); }));
        threads.emplace_back(std::thread([this, img, thresh, patch_size]() {normalize_1(img), find_corner(thresh, 1, patch_size); }));
        threads.emplace_back(std::thread([this, img, thresh, patch_size]() {normalize_2(img), find_corner(thresh, 2, patch_size); }));
        threads.emplace_back(std::thread([this, img, thresh, patch_size]() {normalize_3(img), find_corner(thresh, 3, patch_size); }));

        for (auto& thread : threads)
            thread.join();
    }

    int tare_ground_truth_calculator::run(float& ground_truth)
    {
        static int reset_counter = 0;
        if (reset_counter > _reset_limit)
        {
            reset_counter = 0;
            _corners_idx = 0;
            _corners_num = 0;
        }

        int ret = 0;
        if (_corners_found[0] && _corners_found[1] && _corners_found[2] && _corners_found[3])
        {
            ret = 1;
            reset_counter = 0;
            _corners_idx = ++_corners_idx % _frame_num;
            ++_corners_num;

            if (_corners_num == _frame_num)
            {
                ret = 2;
                ground_truth = calculate_depth();
                --_corners_num;
            }

        }
        else
            ++reset_counter;

        return ret;
    }

    int tare_ground_truth_calculator::run(float rect_sides[4])
    {
        static int reset_counter = 0;
        if (reset_counter > _reset_limit)
        {
            reset_counter = 0;
            _corners_idx = 0;
            _corners_num = 0;
        }

        int ret = 0;
        if (_corners_found[0] && _corners_found[1] && _corners_found[2] && _corners_found[3])
        {
            ret = 1;
            reset_counter = 0;
            _corners_idx = ++_corners_idx % _frame_num;
            ++_corners_num;

            if (_corners_num == _frame_num)
            {
                ret = 2;
                calculate_rect(rect_sides);
                --_corners_num;
            }

        }
        else
            ++reset_counter;

        return ret;
    }

    float tare_ground_truth_calculator::calculate_depth()
    {
        point<float> corners_avg[4];
        for (int i = 0; i < 4; ++i)
        {
            corners_avg[i].x = _corners[0][i].x;
            corners_avg[i].y = _corners[0][i].y;
        }

        for (int j = 1; j < _frame_num; ++j)
        {
            for (int i = 0; i < 4; ++i)
            {
                corners_avg[i].x += _corners[j][i].x;
                corners_avg[i].y += _corners[j][i].y;
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            corners_avg[i].x /= _frame_num;
            corners_avg[i].y /= _frame_num;
        }

        float lx = corners_avg[1].x - corners_avg[0].x;
        float ly = corners_avg[1].y - corners_avg[0].y;
        float d1 = _target_fw / sqrtf(lx * lx + ly * ly);

        lx = corners_avg[3].x - corners_avg[2].x;
        ly = corners_avg[3].y - corners_avg[2].y;
        float d2 = _target_fw / sqrtf(lx * lx + ly * ly);

        lx = corners_avg[2].x - corners_avg[0].x;
        ly = corners_avg[2].y - corners_avg[0].y;
        float d3 = _target_fh / sqrtf(lx * lx + ly * ly);

        lx = corners_avg[3].x - corners_avg[1].x;
        ly = corners_avg[3].y - corners_avg[1].y;
        float d4 = _target_fh / sqrtf(lx * lx + ly * ly);

        return (d1 + d2 + d3 + d4) / 4;
    }

    void tare_ground_truth_calculator::calculate_rect(float rect_sides[4])
    {
        point<float> corners_avg[4];
        for (int i = 0; i < 4; ++i)
        {
            corners_avg[i].x = _corners[0][i].x;
            corners_avg[i].y = _corners[0][i].y;
        }

        for (int j = 1; j < _frame_num; ++j)
        {
            for (int i = 0; i < 4; ++i)
            {
                corners_avg[i].x += _corners[j][i].x;
                corners_avg[i].y += _corners[j][i].y;
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            corners_avg[i].x /= _frame_num;
            corners_avg[i].y /= _frame_num;
        }

        float lx = corners_avg[1].x - corners_avg[0].x;
        float ly = corners_avg[1].y - corners_avg[0].y;
        rect_sides[0] = sqrtf(lx * lx + ly * ly);

        lx = corners_avg[3].x - corners_avg[2].x;
        ly = corners_avg[3].y - corners_avg[2].y;
        rect_sides[1] = sqrtf(lx * lx + ly * ly);

        lx = corners_avg[2].x - corners_avg[0].x;
        ly = corners_avg[2].y - corners_avg[0].y;
        rect_sides[2] = sqrtf(lx * lx + ly * ly);

        lx = corners_avg[3].x - corners_avg[1].x;
        ly = corners_avg[3].y - corners_avg[1].y;
        rect_sides[3] = sqrtf(lx * lx + ly * ly);
    }

    void tare_ground_truth_calculator::minimize_x(const float* p, int s, float* f, float& x)
    {
        int ws = _hwidth - s;

        for (int i = 0; i < s; ++i)
            f[i] = 0;

        for (int j = 0; j < s; ++j)
        {
            for (int i = 0; i < s; ++i)
                f[i] += *p++;
            p += ws;
        }

        for (int i = 0; i < s; ++i)
            f[i] = -f[i];

        int mi = 0;
        float mv = f[mi];
        for (int i = 1; i < s; ++i)
        {
            if (f[i] < mv)
            {
                mi = i;
                mv = f[mi];
            }
        }

        float x0 = 0.0f;
        float y0 = 0.0f;
        if (mi == 0)
        {
            if (fit_parabola(0.0f, f[0], 1.0f, f[1], 2.0f, f[2], x0, y0) && x0 > 0)
                x += x0;
        }
        else if (mi + 1 == s)
        {
            if (fit_parabola(static_cast<float>(s - 3), f[s - 3], static_cast<float>(s - 2), f[s - 2], static_cast<float>(s - 1), f[s - 1], x0, y0))
                x += x0;
            else
                x += s - 1;
        }
        else
        {
            if (fit_parabola(static_cast<float>(mi - 1), static_cast<float>(f[mi - 1]), static_cast<float>(mi), static_cast<float>(f[mi]), static_cast<float>(mi + 1), static_cast<float>(f[mi + 1]), x0, y0) && x0 < s)
                x += x0;
            else
                x += mi;
        }
    }

    void tare_ground_truth_calculator::minimize_y(const float* p, int s, float* f, float& y)
    {
        int ws = _hwidth - s;

        for (int i = 0; i < s; ++i)
            f[i] = 0;

        for (int j = 0; j < s; ++j)
        {
            for (int i = 0; i < s; ++i)
                f[j] += *p++;
            p += ws;
        }

        for (int i = 0; i < s; ++i)
            f[i] = -f[i];

        int mi = 0;
        float mv = f[mi];
        for (int i = 1; i < s; ++i)
        {
            if (f[i] < mv)
            {
                mi = i;
                mv = f[mi];
            }
        }

        float x0 = 0.0f;
        float y0 = 0.0f;
        if (mi == 0)
        {
            if (fit_parabola(0.0f, f[0], 1.0f, f[1], 2.0f, f[2], x0, y0) && x0 > 0)
                y += x0;
        }
        else if (mi + 1 == s)
        {
            if (fit_parabola(static_cast<float>(s - 3), f[s - 3], static_cast<float>(s - 2), f[s - 2], static_cast<float>(s - 1), f[s - 1], x0, y0) && x0 < s)
                y += x0;
            else
                y += s - 1;
        }
        else
        {
            if (fit_parabola(static_cast<float>(mi - 1), static_cast<float>(f[mi - 1]), static_cast<float>(mi), static_cast<float>(f[mi]), static_cast<float>(mi + 1), static_cast<float>(f[mi + 1]), x0, y0))
                y += x0;
            else
                y += mi;
        }
    }

    bool tare_ground_truth_calculator::fit_parabola(float x1, float y1, float x2, float y2, float x3, float y3, float& x0, float& y0)
    {
        float det = x1 * x1 * x2 + x1 * x3 * x3 + x2 * x2 * x3 - x2 * x3 * x3 - x1 * x1 * x3 - x1 * x2 * x2;
        if (det == 0)
            return false;

        float m11 = (x2 - x3) / det;
        float m12 = (x3 - x1) / det;
        float m13 = (x1 - x2) / det;

        float m21 = (x3 * x3 - x2 * x2) / det;
        float m22 = (x1 * x1 - x3 * x3) / det;
        float m23 = (x2 * x2 - x1 * x1) / det;

        float m31 = (x2 * x2 * x3 - x2 * x3 * x3) / det;
        float m32 = (x1 * x3 * x3 - x1 * x1 * x3) / det;
        float m33 = (x1 * x1 * x2 - x1 * x2 * x2) / det;

        float A = m11 * y1 + m12 * y2 + m13 * y3;
        float B = m21 * y1 + m22 * y2 + m23 * y3;
        float C = m31 * y1 + m32 * y2 + m33 * y3;

        x0 = -B / (2 * A);
        y0 = C - B * B / (4 * A);

        return A > 0;
    }
}
