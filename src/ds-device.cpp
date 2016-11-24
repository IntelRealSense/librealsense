// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <climits>
#include <algorithm>
#include <iomanip>      // for std::put_time

#include "image.h"
#include "ds-private.h"
#include "ds-device.h"

using namespace rsimpl;
using namespace rsimpl::ds;
using namespace rsimpl::motion_module;

// DS4 Exposure ROI uses stream resolution as control constraints
// When stream disabled, we are supposed to use VGA as the default
#define MAX_DS_DEFAULT_X 639
#define MAX_DS_DEFAULT_Y 439

namespace rsimpl
{
    ds_device::ds_device(std::shared_ptr<uvc::device> device, const static_device_info & info, calibration_validator validator)
        : rs_device_base(device, info, validator), start_stop_pad(std::chrono::milliseconds(500))
    {
        rs_option opt[] = {RS_OPTION_R200_DEPTH_UNITS};
        double units;
        get_options(opt, 1, &units);
        on_update_depth_units(static_cast<int>(units));
    }
    
    ds_device::~ds_device()
    {

    }

    bool ds_device::is_disparity_mode_enabled() const
    {
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        return depth.is_enabled() && depth.get_format() == RS_FORMAT_DISPARITY16;
    }

    void ds_device::on_update_depth_units(uint32_t units)
    {
        if(is_disparity_mode_enabled()) return;
        config.depth_scale = (float)((double)units * 0.000001); // Convert from micrometers to meters
    }

    void ds_device::on_update_disparity_multiplier(double multiplier)
    {
        if(!is_disparity_mode_enabled()) return;
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        float baseline = get_stream_interface(RS_STREAM_INFRARED2).get_extrinsics_to(depth).translation[0];
        config.depth_scale = static_cast<float>(depth.get_intrinsics().fx * baseline * multiplier);
    }

    std::vector<supported_option> ds_device::get_ae_range_vec()
    {
        std::vector<supported_option> so_vec;
        std::vector<rs_option> ae_vector = { RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE, RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE, RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE, RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE };
        for (auto& opt : ae_vector)
        {
            double min, max, step, def;
            get_option_range(opt, min, max, step, def);
            so_vec.push_back({ opt, min, max, step, def });
        }

        return so_vec;
    }

    
    void correct_lr_auto_exposure_params(rs_device_base * device, ae_params& params)
    {
        auto& stream = device->get_stream_interface(RS_STREAM_DEPTH);
        uint16_t max_x = MAX_DS_DEFAULT_X;
        uint16_t max_y = MAX_DS_DEFAULT_Y;
        if (stream.is_enabled())
        {
            auto intrinsics = stream.get_intrinsics();
            max_x = intrinsics.width - 1;
            max_y = intrinsics.height - 1;
        }

        // first, bring to the valid range
        params.exposure_left_edge = std::max((uint16_t)0, std::min(max_x, params.exposure_left_edge));
        params.exposure_right_edge = std::max((uint16_t)0, std::min(max_x, params.exposure_right_edge));
        params.exposure_top_edge = std::max((uint16_t)0, std::min(max_y, params.exposure_top_edge));
        params.exposure_bottom_edge = std::max((uint16_t)0, std::min(max_y, params.exposure_bottom_edge));
        // now, let's take care of order:
        auto left = std::min(params.exposure_left_edge, params.exposure_right_edge);
        auto right = std::max(params.exposure_left_edge, params.exposure_right_edge);
        auto top = std::min(params.exposure_top_edge, params.exposure_bottom_edge);
        auto bottom = std::max(params.exposure_top_edge, params.exposure_bottom_edge);

        if (right == left){
            if (left == 0) right++;
            else left--;
        }
        if (bottom == top) {
            if (top == 0) bottom++;
            else top--;
        }

        params.exposure_left_edge = left;
        params.exposure_right_edge = right;
        params.exposure_top_edge = top;
        params.exposure_bottom_edge = bottom;
    }

    void ds_device::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        auto & dev = get_device();
        auto minmax_writer = make_struct_interface<ds::range    >([&dev]() { return ds::get_min_max_depth(dev);           }, [&dev](ds::range     v) { ds::set_min_max_depth(dev,v);           });
        auto disp_writer   = make_struct_interface<ds::disp_mode>([&dev]() { return ds::get_disparity_mode(dev);          }, [&dev](ds::disp_mode v) { ds::set_disparity_mode(dev,v);          });
        auto ae_writer = make_struct_interface<ds::ae_params>(
            [&dev, this]() { 
                auto ae = ds::get_lr_auto_exposure_params(dev, get_ae_range_vec());
                correct_lr_auto_exposure_params(this, ae);
                return ae; 
            }, 
            [&dev, this](ds::ae_params& v) { 
                correct_lr_auto_exposure_params(this, v);
                ds::set_lr_auto_exposure_params(dev, v); 
            }
        );
        auto dc_writer     = make_struct_interface<ds::dc_params>([&dev]() { return ds::get_depth_params(dev);            }, [&dev](ds::dc_params v) { ds::set_depth_params(dev,v);            });

        for (size_t i = 0; i<count; ++i)
        {
            if (uvc::is_pu_control(options[i]))
            {
                // Disabling auto-setting controls, if needed
                switch (options[i])
                {
                case RS_OPTION_COLOR_WHITE_BALANCE:     disable_auto_option( 2, RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE); break;
                case RS_OPTION_COLOR_EXPOSURE:          disable_auto_option( 2, RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE); break;
                default:  break;
                }

                uvc::set_pu_control_with_retry(get_device(), 2, options[i], static_cast<int>(values[i]));
                continue;
            }

            switch(options[i])
            {

            case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:                   ds::set_lr_exposure_mode(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_R200_LR_GAIN:                                    ds::set_lr_gain(get_device(), {get_lr_framerate(), static_cast<uint32_t>(values[i])}); break; // TODO: May need to set this on start if framerate changes
            case RS_OPTION_R200_LR_EXPOSURE:                                ds::set_lr_exposure(get_device(), {get_lr_framerate(), static_cast<uint32_t>(values[i])}); break; // TODO: May need to set this on start if framerate changes
            case RS_OPTION_R200_EMITTER_ENABLED:                            ds::set_emitter_state(get_device(), !!values[i]); break;
            case RS_OPTION_R200_DEPTH_UNITS:                                ds::set_depth_units(get_device(), static_cast<uint32_t>(values[i]));
                on_update_depth_units(static_cast<uint32_t>(values[i])); break;

            case RS_OPTION_R200_DEPTH_CLAMP_MIN:                            minmax_writer.set(&ds::range::min, values[i]); break;
            case RS_OPTION_R200_DEPTH_CLAMP_MAX:                            minmax_writer.set(&ds::range::max, values[i]); break;

            case RS_OPTION_R200_DISPARITY_MULTIPLIER:                       disp_writer.set(&ds::disp_mode::disparity_multiplier, values[i]); break;
            case RS_OPTION_R200_DISPARITY_SHIFT:                            ds::set_disparity_shift(get_device(), static_cast<uint32_t>(values[i])); break;

            case RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT:     ae_writer.set(&ds::ae_params::mean_intensity_set_point, values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT:       ae_writer.set(&ds::ae_params::bright_ratio_set_point,   values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN:                      ae_writer.set(&ds::ae_params::kp_gain,                  values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE:                  ae_writer.set(&ds::ae_params::kp_exposure,              values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD:            ae_writer.set(&ds::ae_params::kp_dark_threshold,        values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE:                     ae_writer.set(&ds::ae_params::exposure_top_edge,        values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE:                  ae_writer.set(&ds::ae_params::exposure_bottom_edge,     values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE:                    ae_writer.set(&ds::ae_params::exposure_left_edge,       values[i]); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE:                   ae_writer.set(&ds::ae_params::exposure_right_edge,      values[i]); break;

            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT:    dc_writer.set(&ds::dc_params::robbins_munroe_minus_inc, values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT:    dc_writer.set(&ds::dc_params::robbins_munroe_plus_inc,  values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD:             dc_writer.set(&ds::dc_params::median_thresh,            values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD:      dc_writer.set(&ds::dc_params::score_min_thresh,         values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD:      dc_writer.set(&ds::dc_params::score_max_thresh,         values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD:      dc_writer.set(&ds::dc_params::texture_count_thresh,     values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD: dc_writer.set(&ds::dc_params::texture_diff_thresh,      values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD:        dc_writer.set(&ds::dc_params::second_peak_thresh,       values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD:           dc_writer.set(&ds::dc_params::neighbor_thresh,          values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD:                 dc_writer.set(&ds::dc_params::lr_thresh,                values[i]); break;

            default: 
                base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        minmax_writer.commit();
        disp_writer.commit();
        if(disp_writer.active) on_update_disparity_multiplier(disp_writer.struct_.disparity_multiplier);
        ae_writer.commit();
        dc_writer.commit();

        //Handle common options
        if (base_opt.size())
            rs_device_base::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void ds_device::get_options(const rs_option options[], size_t count, double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<size_t>     base_opt_index;
        std::vector<double>     base_opt_val;

        auto & dev = get_device();
        auto minmax_reader = make_struct_interface<ds::range    >([&dev]() { return ds::get_min_max_depth(dev);           }, [&dev](ds::range     v) { ds::set_min_max_depth(dev,v);           });
        auto disp_reader   = make_struct_interface<ds::disp_mode>([&dev]() { return ds::get_disparity_mode(dev);          }, [&dev](ds::disp_mode v) { ds::set_disparity_mode(dev,v);          });
        auto ae_reader = make_struct_interface<ds::ae_params>(
            [&dev, this]() { 
                auto ae = ds::get_lr_auto_exposure_params(dev, get_ae_range_vec());
                correct_lr_auto_exposure_params(this, ae);
                return ae; 
            }, 
            [&dev, this](ds::ae_params& v) { 
                correct_lr_auto_exposure_params(this, v);
                ds::set_lr_auto_exposure_params(dev, v); 
            }
        );
        auto dc_reader     = make_struct_interface<ds::dc_params>([&dev]() { return ds::get_depth_params(dev);            }, [&dev](ds::dc_params v) { ds::set_depth_params(dev,v);            });

        for (size_t i = 0; i<count; ++i)
        {

            if(uvc::is_pu_control(options[i]))
            {
                values[i] = uvc::get_pu_control(get_device(), 2, options[i]);
                continue;
            }

            switch(options[i])
            {

            case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:                   values[i] = ds::get_lr_exposure_mode(get_device()); break;

            case RS_OPTION_R200_LR_GAIN: // Gain is framerate dependent
                ds::set_lr_gain_discovery(get_device(), {get_lr_framerate(), 0, 0, 0, 0});
                values[i] = ds::get_lr_gain(get_device()).value;
                break;
            case RS_OPTION_R200_LR_EXPOSURE: // Exposure is framerate dependent
                ds::set_lr_exposure_discovery(get_device(), {get_lr_framerate(), 0, 0, 0, 0});
                values[i] = ds::get_lr_exposure(get_device()).value;
                break;
            case RS_OPTION_R200_EMITTER_ENABLED:
                values[i] = ds::get_emitter_state(get_device(), is_capturing(), get_stream_interface(RS_STREAM_DEPTH).is_enabled());
                break;

            case RS_OPTION_R200_DEPTH_UNITS:                                values[i] = ds::get_depth_units(dev);  break;

            case RS_OPTION_R200_DEPTH_CLAMP_MIN:                            values[i] = minmax_reader.get(&ds::range::min); break;
            case RS_OPTION_R200_DEPTH_CLAMP_MAX:                            values[i] = minmax_reader.get(&ds::range::max); break;

            case RS_OPTION_R200_DISPARITY_MULTIPLIER:                       values[i] = disp_reader.get(&ds::disp_mode::disparity_multiplier); break;
            case RS_OPTION_R200_DISPARITY_SHIFT:                            values[i] = ds::get_disparity_shift(dev); break;

            case RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT:     values[i] = ae_reader.get(&ds::ae_params::mean_intensity_set_point); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT:       values[i] = ae_reader.get(&ds::ae_params::bright_ratio_set_point  ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN:                      values[i] = ae_reader.get(&ds::ae_params::kp_gain                 ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE:                  values[i] = ae_reader.get(&ds::ae_params::kp_exposure             ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD:            values[i] = ae_reader.get(&ds::ae_params::kp_dark_threshold       ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE:                     values[i] = ae_reader.get(&ds::ae_params::exposure_top_edge       ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE:                  values[i] = ae_reader.get(&ds::ae_params::exposure_bottom_edge    ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE:                    values[i] = ae_reader.get(&ds::ae_params::exposure_left_edge      ); break;
            case RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE:                   values[i] = ae_reader.get(&ds::ae_params::exposure_right_edge     ); break;

            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT:    values[i] = dc_reader.get(&ds::dc_params::robbins_munroe_minus_inc); break;
            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT:    values[i] = dc_reader.get(&ds::dc_params::robbins_munroe_plus_inc ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD:             values[i] = dc_reader.get(&ds::dc_params::median_thresh           ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD:      values[i] = dc_reader.get(&ds::dc_params::score_min_thresh        ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD:      values[i] = dc_reader.get(&ds::dc_params::score_max_thresh        ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD:      values[i] = dc_reader.get(&ds::dc_params::texture_count_thresh    ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD: values[i] = dc_reader.get(&ds::dc_params::texture_diff_thresh     ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD:        values[i] = dc_reader.get(&ds::dc_params::second_peak_thresh      ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD:           values[i] = dc_reader.get(&ds::dc_params::neighbor_thresh         ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD:                 values[i] = dc_reader.get(&ds::dc_params::lr_thresh               ); break;

            default:
                base_opt.push_back(options[i]); base_opt_index.push_back(i); break;
            }
        }
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            rs_device_base::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (auto i : base_opt_index)
            values[i] = base_opt_val[i];
    }

    void ds_device::stop(rs_source source)
    {
        start_stop_pad.stop();
        rs_device_base::stop(source);
    }

    void ds_device::start(rs_source source)
    {
        rs_device_base::start(source);
        start_stop_pad.start();
    }

    void ds_device::start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex)
    {
        rs_device_base::start_fw_logger(fw_log_op_code, grab_rate_in_ms, mutex);
    }

    void ds_device::stop_fw_logger()
    {
        rs_device_base::stop_fw_logger();
    }

    void ds_device::on_before_start(const std::vector<subdevice_mode_selection> & selected_modes)
    {
        rs_option depth_units_option = RS_OPTION_R200_DEPTH_UNITS;
        double depth_units;

        uint8_t streamIntent = 0;
        for(const auto & m : selected_modes)
        {
            switch(m.mode.subdevice)
            {
            case 0: streamIntent |= ds::STATUS_BIT_LR_STREAMING; break;
            case 2: streamIntent |= ds::STATUS_BIT_WEB_STREAMING; break;
            case 1: 
                streamIntent |= ds::STATUS_BIT_Z_STREAMING;
                auto dm = ds::get_disparity_mode(get_device());
                switch(m.get_format(RS_STREAM_DEPTH))
                {
                default: throw std::logic_error("unsupported R200 depth format");
                case RS_FORMAT_Z16: 
                    dm.is_disparity_enabled = 0;
                    get_options(&depth_units_option, 1, &depth_units);
                    on_update_depth_units(static_cast<int>(depth_units));
                    break;
                case RS_FORMAT_DISPARITY16: 
                    dm.is_disparity_enabled = 1;
                    on_update_disparity_multiplier(static_cast<float>(dm.disparity_multiplier));
                    break;
                }
                ds::set_disparity_mode(get_device(), dm);

                auto ae_enabled = ds::get_lr_exposure_mode(get_device()) > 0;
                if (ae_enabled)
                {
                    ds::set_lr_exposure_mode(get_device(), 0);
                    ds::set_lr_exposure_mode(get_device(), 1);
                }

                break;
            }
        }
        ds::set_stream_intent(get_device(), streamIntent);
    }

    rs_stream ds_device::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
    {
        // When all streams are enabled at an identical framerate, R200 images are delivered in the order: Z -> Third -> L/R
        // To maximize the chance of being able to deliver coherent framesets, we want to wait on the latest image coming from
        // a stream running at the fastest framerate.
        int fps[RS_STREAM_NATIVE_COUNT] = {}, max_fps = 0;
        for(const auto & m : selected_modes)
        {
            for(const auto & output : m.get_outputs())
            {
                fps[output.first] = m.mode.fps;
                max_fps = std::max(max_fps, m.mode.fps);
            }
        }

        // Select the "latest arriving" stream which is running at the fastest framerate
        for(auto s : {RS_STREAM_COLOR, RS_STREAM_INFRARED2, RS_STREAM_INFRARED})
        {
            if(fps[s] == max_fps) return s;
        }
        return RS_STREAM_DEPTH;
    }

    uint32_t ds_device::get_lr_framerate() const
    {
        for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            auto & stream = get_stream_interface(s);
            if(stream.is_enabled()) return static_cast<uint32_t>(stream.get_framerate());
        }
        return 30; // If no streams have yet been enabled, return the minimum possible left/right framerate, to allow the maximum possible exposure range
    }

    void ds_device::set_common_ds_config(std::shared_ptr<uvc::device> device, static_device_info& info, const ds::ds_info& cam_info)
    {
        auto & c = cam_info.calibration;
        info.capabilities_vector.push_back(RS_CAPABILITIES_ENUMERATION);
        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED2);

        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_COLOR] = 2;
        info.stream_subdevices[RS_STREAM_INFRARED] = 0;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 0;

        // Set up modes for left/right/z images
        for(auto fps : {30, 60, 90})
        {
            // Subdevice 0 can provide left/right infrared via four pixel formats, in three resolutions, which can either be uncropped or cropped to match Z
            for(auto pf : {pf_y8, pf_y8i, pf_y16, pf_y12i})
            {
                info.subdevice_modes.push_back({0, {640, 481}, pf, fps, c.modesLR[0], {}, {0, -6}});
                info.subdevice_modes.push_back({0, {640, 373}, pf, fps, c.modesLR[1], {}, {0, -6}});
                info.subdevice_modes.push_back({0, {640, 254}, pf, fps, c.modesLR[2], {}, {0, -6}});
            }

            // Subdevice 1 can provide depth, in three resolutions, which can either be unpadded or padded to match left/right
            info.subdevice_modes.push_back({ 1,{ 628, 469 }, pf_z16,  fps, pad_crop_intrinsics(c.modesLR[0], -6),{},{ 0, +6 } });
            info.subdevice_modes.push_back({ 1,{ 628, 361 }, pf_z16,  fps, pad_crop_intrinsics(c.modesLR[1], -6),{},{ 0, +6 } });
            info.subdevice_modes.push_back({ 1,{ 628, 242 }, pf_z16,  fps, pad_crop_intrinsics(c.modesLR[2], -6),{},{ 0, +6 } });
        }

        // Subdevice 2 can provide color, in several formats and framerates        
        info.subdevice_modes.push_back({ 2, { 640, 480 }, pf_yuy2, 60, c.intrinsicsThird[1], { c.modesThird[1][0] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 640, 480 }, pf_yuy2, 30, c.intrinsicsThird[1], { c.modesThird[1][0] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 320, 240 }, pf_yuy2, 60, scale_intrinsics(c.intrinsicsThird[1], 320, 240), { c.modesThird[1][1] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 320, 240 }, pf_yuy2, 30, scale_intrinsics(c.intrinsicsThird[1], 320, 240), { c.modesThird[1][1] }, { 0 } });
        
        info.subdevice_modes.push_back({ 2,{ 1920, 1080 }, pf_yuy2, 15, c.intrinsicsThird[0],{ c.modesThird[0][0] },{ 0 } });
        info.subdevice_modes.push_back({ 2,{ 1920, 1080 }, pf_yuy2, 30, c.intrinsicsThird[0],{ c.modesThird[0][0] },{ 0 } });


        //                             subdev native-dim      pf   fps           native_intrinsics                              rect_modes      crop-options
        //                             ------ ----------      --   ---           -----------------                              ----------      ------------
        info.subdevice_modes.push_back({ 2,{ 1280, 720 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[0], 1280, 720),  { c.modesThird[0][1] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 960, 540 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[0], 960, 540),   { c.modesThird[0][1] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 848, 480 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[0], 848, 480),   { c.modesThird[0][1] }, { 0 } });
        
        info.subdevice_modes.push_back({ 2, { 640, 480 }, pf_yuy2, 15, c.intrinsicsThird[1], { c.modesThird[1][0] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 640, 480 }, pf_rw16, 15, c.intrinsicsThird[1], { c.modesThird[1][0] }, { 0 } });
        
        info.subdevice_modes.push_back({ 2, { 640, 360 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[1], 640, 360), { c.modesThird[1][0] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 424, 240 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[1], 424, 240), { c.modesThird[1][0] }, { 0 } });

        info.subdevice_modes.push_back({ 2, { 320, 240 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[1], 320, 240), { c.modesThird[1][1] }, { 0 } });
        info.subdevice_modes.push_back({ 2, { 320, 180 }, pf_yuy2, 15, scale_intrinsics(c.intrinsicsThird[1], 320, 180), { c.modesThird[1][1] }, { 0 } });

        // Set up interstream rules for left/right/z images
        for(auto ir : {RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            info.interstream_rules.push_back({ RS_STREAM_DEPTH, ir, &stream_request::width, 0, 12, RS_STREAM_COUNT, false, false, false });
            info.interstream_rules.push_back({ RS_STREAM_DEPTH, ir, &stream_request::height, 0, 12, RS_STREAM_COUNT, false, false, false });
            info.interstream_rules.push_back({ RS_STREAM_DEPTH, ir, &stream_request::fps, 0, 0, RS_STREAM_COUNT, false, false, false });
        }
        info.interstream_rules.push_back({ RS_STREAM_DEPTH, RS_STREAM_COLOR, &stream_request::fps, 0, 0, RS_STREAM_DEPTH, true, false, false });
        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, &stream_request::fps, 0, 0, RS_STREAM_COUNT, false, false, false });
        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, &stream_request::width, 0, 0, RS_STREAM_COUNT, false, false, false });
        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, &stream_request::height, 0, 0, RS_STREAM_COUNT, false, false, false });
        info.interstream_rules.push_back({ RS_STREAM_INFRARED, RS_STREAM_INFRARED2, nullptr, 0, 0, RS_STREAM_COUNT, false, false, true });

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Y8,   60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Z16,  60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true, 640,   480, RS_FORMAT_Y8,   60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true, 640,   480, RS_FORMAT_Z16,  60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 30, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 320, 240, RS_FORMAT_Y8,   60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 320, 240, RS_FORMAT_Z16,  60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60, RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS};

        for(int i=0; i<RS_PRESET_COUNT; ++i)
            info.presets[RS_STREAM_INFRARED2][i] = info.presets[RS_STREAM_INFRARED][i];

        // Extended controls ranges cannot be retrieved from device, therefore the data is locally defined
        //Option                                                                        Min     Max     Step    Default
        info.options.push_back({ RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED,               0,      1,      1,      0 });
        info.options.push_back({ RS_OPTION_R200_EMITTER_ENABLED,                        0,      1,      1,      0 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_UNITS,                            0, INT_MAX,     1,      1000 });  // What is the real range?
        info.options.push_back({ RS_OPTION_R200_DEPTH_CLAMP_MIN,                        0, USHRT_MAX,   1,      0 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CLAMP_MAX,                        0, USHRT_MAX,   1,      USHRT_MAX });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT, 0,      4095,   1,      512 });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT,   0,      1,      1,      0 });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN,                  0,      1000,   1,      0 });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE,              0,      1000,   1,      0 });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD,        0,      1000,   1,      0 });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE,                 0,      MAX_DS_DEFAULT_Y,    1,      MAX_DS_DEFAULT_Y });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE,              0,      MAX_DS_DEFAULT_Y,    1,      MAX_DS_DEFAULT_Y});
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE,                0,      MAX_DS_DEFAULT_X,    1,      MAX_DS_DEFAULT_X });
        info.options.push_back({ RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE,               0,      MAX_DS_DEFAULT_X,    1,      MAX_DS_DEFAULT_X });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT, 0,     0xFF,   1,      5 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT, 0,     0xFF,   1,      5 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD,         0,      0x3FF,  1,      0xc0 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD,  0,      0x3FF,  1,      1 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD,  0,      0x3FF,  1,      0x200 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD,  0,      0x1F,   1,      6 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD, 0,  0x3FF,  1,      0x18 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD,    0,      0x3FF,  1,      0x1b });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD,       0,      0x3FF,  1,      0x7 });
        info.options.push_back({ RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD,             0,      0x7FF,  1,      0x18 });

        // We select the depth/left infrared camera's viewpoint to be the origin
        info.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}},{0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}},{0,0,0}};

        // The right infrared camera is offset along the +x axis by the baseline (B)
        info.stream_poses[RS_STREAM_INFRARED2] = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, {c.B * 0.001f, 0, 0}}; // Sterling comment

        // The transformation between the depth camera and third camera is described by a translation vector (T), followed by rotation matrix (Rthird)
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j)
            info.stream_poses[RS_STREAM_COLOR].orientation(i,j) = c.Rthird[i*3+j];
        for(int i=0; i<3; ++i)
            info.stream_poses[RS_STREAM_COLOR].position[i] = c.T[i] * 0.001f;

        // Our position is added AFTER orientation is applied, not before, so we must multiply Rthird * T to compute it
        info.stream_poses[RS_STREAM_COLOR].position = info.stream_poses[RS_STREAM_COLOR].orientation * info.stream_poses[RS_STREAM_COLOR].position;
        info.nominal_depth_scale = 0.001f;
        info.serial = std::to_string(c.serial_number);
        info.firmware_version = ds::read_firmware_version(*device);

        auto &h = cam_info.head_content;
        info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = info.firmware_version;
        info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = info.serial;
        info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = info.name;

        info.camera_info[RS_CAMERA_INFO_ISP_FW_VERSION]         = ds::read_isp_firmware_version(*device);
        info.camera_info[RS_CAMERA_INFO_IMAGER_MODEL_NUMBER]    = to_string() << h.imager_model_number;
        info.camera_info[RS_CAMERA_INFO_CAMERA_TYPE]            = to_string() << h.prq_type;
        info.camera_info[RS_CAMERA_INFO_OEM_ID]                 = to_string() << h.oem_id;
        info.camera_info[RS_CAMERA_INFO_MODULE_VERSION]         = to_string() << (int)h.module_version << "." << (int)h.module_major_version << "." << (int)h.module_minor_version << "." << (int)h.module_skew_version;
        info.camera_info[RS_CAMERA_INFO_FOCUS_VALUE]            = to_string() << h.platform_camera_focus;
        info.camera_info[RS_CAMERA_INFO_CONTENT_VERSION]        = to_string() << h.camera_head_contents_version;
        info.camera_info[RS_CAMERA_INFO_LENS_TYPE]              = to_string() << h.lens_type;
        info.camera_info[RS_CAMERA_INFO_LENS_COATING__TYPE]     = to_string() << h.lens_coating_type;
        info.camera_info[RS_CAMERA_INFO_3RD_LENS_TYPE]          = to_string() << h.lens_type_third_imager;
        info.camera_info[RS_CAMERA_INFO_3RD_LENS_COATING_TYPE]  = to_string() << h.lens_coating_type_third_imager;
        info.camera_info[RS_CAMERA_INFO_NOMINAL_BASELINE]       = to_string() << h.nominal_baseline << " mm";
        info.camera_info[RS_CAMERA_INFO_3RD_NOMINAL_BASELINE]   = to_string() << h.nominal_baseline_third_imager << " mm";
        info.camera_info[RS_CAMERA_INFO_EMITTER_TYPE]           = to_string() << h.emitter_type;

        if (std::isnormal(h.calibration_date))
            info.camera_info[RS_CAMERA_INFO_CALIBRATION_DATE] = time_to_string(h.calibration_date);
        if (std::isnormal(h.first_program_date))
            info.camera_info[RS_CAMERA_INFO_PROGRAM_DATE] = time_to_string(h.first_program_date);
        if (std::isnormal(h.focus_alignment_date))
            info.camera_info[RS_CAMERA_INFO_FOCUS_ALIGNMENT_DATE] = time_to_string(h.focus_alignment_date);
        if (std::isnormal(h.build_date))
            info.camera_info[RS_CAMERA_INFO_BUILD_DATE] = time_to_string(h.build_date);

        // On LibUVC backends, the R200 should use four transfer buffers
        info.num_libuvc_transfer_buffers = 4;

        rs_device_base::update_device_info(info);
    }

    bool ds_device::supports_option(rs_option option) const
    {
        std::vector<rs_option> auto_exposure_options = { 
            RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE, 
            RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE,
            RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE,
            RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD,
            RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT,
            RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT
        };

        if (std::find(auto_exposure_options.begin(), auto_exposure_options.end(), option) != auto_exposure_options.end())
        {
            return ds::get_lr_exposure_mode(get_device()) > 0;
        }

        std::vector<rs_option> only_when_not_streaming = { 
            RS_OPTION_R200_DEPTH_UNITS,
            RS_OPTION_R200_DEPTH_CLAMP_MIN,
            RS_OPTION_R200_DEPTH_CLAMP_MAX,
            RS_OPTION_R200_DISPARITY_MULTIPLIER,
            RS_OPTION_R200_DISPARITY_SHIFT,
        };

        if (std::find(only_when_not_streaming.begin(), only_when_not_streaming.end(), option) != only_when_not_streaming.end())
        {
            if (is_capturing()) return false;
        }

        // We have special logic to implement LR gain and exposure, so they do not belong to the standard option list
        return option == RS_OPTION_R200_LR_GAIN || option == RS_OPTION_R200_LR_EXPOSURE || rs_device_base::supports_option(option);
    }

    void ds_device::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
    {
        // Gain min/max is framerate dependent
        if(option == RS_OPTION_R200_LR_GAIN)
        {
            ds::set_lr_gain_discovery(get_device(), {get_lr_framerate(), 0, 0, 0, 0});
            auto disc = ds::get_lr_gain_discovery(get_device());
            min = disc.min;
            max = disc.max;
            step = 1;
            def = disc.default_value;
            return;
        }

        // Exposure min/max is framerate dependent
        if(option == RS_OPTION_R200_LR_EXPOSURE)
        {
            ds::set_lr_exposure_discovery(get_device(), {get_lr_framerate(), 0, 0, 0, 0});
            auto disc = ds::get_lr_exposure_discovery(get_device());
            min = disc.min;
            max = disc.max;
            step = 1;
            def = disc.default_value;
            return;
        }

        // Exposure values converted from [0..3] to [0..1] range
        if(option == RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE)
        {
            rs_device_base::get_option_range(option, min, max, step, def);
            max = 1;
            step = 1;
            return;
        }

        std::vector<rs_option> auto_exposure_options = { 
            RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE, 
            RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE,
            RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE,
            RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE,
        };
        if (std::find(auto_exposure_options.begin(), auto_exposure_options.end(), option) != auto_exposure_options.end())
        {
            auto& stream = get_stream_interface(rs_stream::RS_STREAM_DEPTH);
            if (option == RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE)
            {
                auto max_x = MAX_DS_DEFAULT_X; 
                if (stream.is_enabled()) max_x = stream.get_intrinsics().width - 1;
                min = 1; max = max_x; step = 1; def = max_x;
                return;
            }
            else if (option == RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE)
            {
                auto max_x = MAX_DS_DEFAULT_X; 
                if (stream.is_enabled()) max_x = stream.get_intrinsics().width - 1;
                min = 1; max = max_x - 1; step = 1; def = 0;
                return;
            }
            else if (option == RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE)
            {
                auto max_y = MAX_DS_DEFAULT_Y; 
                if (stream.is_enabled()) max_y = stream.get_intrinsics().height - 1;
                min = 1; max = max_y; step = 1; def = max_y;
                return;
            }
            else if (option == RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE)
            {
                auto max_y = MAX_DS_DEFAULT_Y; 
                if (stream.is_enabled()) max_y = stream.get_intrinsics().height - 1;
                min = 0; max = max_y - 1; step = 1; def = 0;
                return;
            }
        }

        // Default to parent implementation
        rs_device_base::get_option_range(option, min, max, step, def);
    }

    // All R200 images which are not in YUY2 format contain an extra row of pixels, called the "dinghy", which contains useful information
    const ds::dinghy & get_dinghy(const subdevice_mode & mode, const void * frame)
    {
        return *reinterpret_cast<const ds::dinghy *>(reinterpret_cast<const uint8_t *>(frame) + mode.pf.get_image_size(mode.native_dims.x, mode.native_dims.y-1));
    }

    class dinghy_timestamp_reader : public frame_timestamp_reader
    {
        int fps;
        wraparound_mechanism<double> timestamp_wraparound;
        wraparound_mechanism<unsigned long long> frame_counter_wraparound;
        double last_timestamp;

    public:
        dinghy_timestamp_reader(int fps) : fps(fps), last_timestamp(0), timestamp_wraparound(1, std::numeric_limits<uint32_t>::max()), frame_counter_wraparound(1, std::numeric_limits<uint32_t>::max()) {}

        bool validate_frame(const subdevice_mode & mode, const void * frame) override
        {
            // Check magic number for all subdevices
            auto & dinghy = get_dinghy(mode, frame);
            const uint32_t magic_numbers[] = {0x08070605, 0x04030201, 0x8A8B8C8D};
            if(dinghy.magicNumber != magic_numbers[mode.subdevice])
            {
                LOG_WARNING("Subdevice " << mode.subdevice << " bad magic number 0x" << std::hex << dinghy.magicNumber);
                return false;
            }

            // Check frame status for left/right/Z subdevices only
            if(dinghy.frameStatus != 0)
            {
                LOG_WARNING("Subdevice " << mode.subdevice << " frame status 0x" << std::hex << dinghy.frameStatus);
                return false;
            }
            // Check VDF error status for all subdevices
            if(dinghy.VDFerrorStatus != 0)
            {
                LOG_WARNING("Subdevice " << mode.subdevice << " VDF error status 0x" << std::hex << dinghy.VDFerrorStatus);
                return false;
            }
            // Check CAM module status for left/right subdevice only
            if (dinghy.CAMmoduleStatus != 0 && mode.subdevice == 0)
            {
                LOG_WARNING("Subdevice " << mode.subdevice << " CAM module status 0x" << std::hex << dinghy.CAMmoduleStatus);
                return false;
            }        
            
            // TODO: Check for missing or duplicate frame numbers
            return true;
        }

        double get_frame_timestamp(const subdevice_mode & mode, const void * frame, double /*actual_fps*/) override
        {
          auto new_ts = timestamp_wraparound.fix(last_timestamp + 1000. / fps);
          last_timestamp = new_ts;
          return new_ts;
        }

        unsigned long long get_frame_counter(const subdevice_mode & mode, const void * frame) override
        {
            auto frame_number = 0;
        
           frame_number = get_dinghy(mode, frame).frameCount; // All other formats can use the frame number in the dinghy row
           return frame_counter_wraparound.fix(frame_number);
        }
    };

    class fisheye_timestamp_reader : public frame_timestamp_reader
    {
        int get_embedded_frame_counter(const void * frame) const
        {
            int embedded_frame_counter = 0;
            firmware_version firmware(fw_version);
            if (firmware >= firmware_version("1.27.2.90")) // Frame counter is exposed at first LSB bit in first 4-pixels from version 1.27.2.90
            {
                auto data = static_cast<const char*>(frame);

                for (int i = 0, j = 0; i < 4; ++i, ++j)
                    embedded_frame_counter |= ((data[i] & 0x01) << j);
            }
            else if (firmware < firmware_version("1.27.2.90")) // Frame counter is exposed by the 4 LSB bits of the first pixel from all versions under 1.27.2.90
            {
                embedded_frame_counter = reinterpret_cast<byte_wrapping&>(*((unsigned char*)frame)).lsb;
            }

            return embedded_frame_counter;
        }

        std::string fw_version;
        std::mutex mutex;
        int configured_fps;
        unsigned last_fisheye_counter;
        double last_fisheye_timestamp;
        wraparound_mechanism<double> timestamp_wraparound;
        wraparound_mechanism<unsigned long long> frame_counter_wraparound;
        mutable bool validate;

    public:
        fisheye_timestamp_reader(int in_configured_fps, const char* fw_ver) : configured_fps(in_configured_fps), last_fisheye_timestamp(0), last_fisheye_counter(0), timestamp_wraparound(1, std::numeric_limits<uint32_t>::max()), frame_counter_wraparound(0, std::numeric_limits<uint32_t>::max()), validate(true), fw_version(fw_ver){}

        bool validate_frame(const subdevice_mode & /*mode*/, const void * frame) override
        {
            if (!validate)
                return true;

            bool sts;
            auto pixel_lsb = get_embedded_frame_counter(frame);
            if ((sts = (pixel_lsb != 0)))
                validate = false;

            return sts;
        }

        struct byte_wrapping{
            unsigned char lsb : 4;
            unsigned char msb : 4;
        };

        unsigned long long get_frame_counter(const subdevice_mode & /*mode*/, const void * frame) override
        {
            std::lock_guard<std::mutex> guard(mutex);

            auto last_counter_lsb = reinterpret_cast<byte_wrapping&>(last_fisheye_counter).lsb;
            auto pixel_lsb = get_embedded_frame_counter(frame);
            if (last_counter_lsb == pixel_lsb)
                return last_fisheye_counter;

            auto last_counter_msb = (last_fisheye_counter >> 4);
            auto wrap_around = reinterpret_cast<byte_wrapping&>(last_fisheye_counter).lsb;
            if (wrap_around == 15 || pixel_lsb < last_counter_lsb)
            {
                ++last_counter_msb;
            }

            auto fixed_counter = (last_counter_msb << 4) | (pixel_lsb & 0xff);

            last_fisheye_counter = fixed_counter;
            return frame_counter_wraparound.fix(fixed_counter);
        }

        double get_frame_timestamp(const subdevice_mode & mode, const void * frame, double actual_fps) override
        {
            auto new_ts = timestamp_wraparound.fix(last_fisheye_timestamp + 1000. / actual_fps);
            last_fisheye_timestamp = new_ts;
            return new_ts;
        }
    };

    class color_timestamp_reader : public frame_timestamp_reader
    {
        int fps, scale;
        wraparound_mechanism<double> timestamp_wraparound;
        wraparound_mechanism<unsigned long long> frame_counter_wraparound;
        bool first_frames = true;
        double last_timestamp;
    public:
        color_timestamp_reader(int fps, int scale) : fps(fps), scale(scale), last_timestamp(0), timestamp_wraparound(0, std::numeric_limits<uint32_t>::max()), frame_counter_wraparound(0, std::numeric_limits<uint32_t>::max()) {}

        bool validate_frame(const subdevice_mode & mode, const void * frame) override
        {
            auto counter = get_frame_counter(mode, frame);

            if (counter == 0 && first_frames) return false;
            first_frames = false;
            return true;
        }

        unsigned long long get_frame_counter(const subdevice_mode & mode, const void * frame) override
        {
            auto frame_number = 0;
            
            // YUY2 images encode the frame number in the low order bits of the final 32 bytes of the image
            auto data = reinterpret_cast<const uint8_t *>(frame)+((mode.native_dims.x * mode.native_dims.y) - 32) * 2;
            for (auto i = 0; i < 32; ++i)
            {
                frame_number |= ((*data & 1) << (i & 1 ? 32 - i : 30 - i));
                data += 2;
            }

            frame_number /= scale;
            
            return frame_counter_wraparound.fix(frame_number);
        }

        double get_frame_timestamp(const subdevice_mode & mode, const void * frame, double /*actual_fps*/) override
        {
            auto new_ts = timestamp_wraparound.fix(last_timestamp + 1000. / fps);
            last_timestamp = new_ts;
            return new_ts;
        }
    };

    class serial_timestamp_generator : public frame_timestamp_reader
    {
        int fps, serial_frame_number;
        double last_timestamp;
        double ts_step;
        wraparound_mechanism<double> timestamp_wraparound;
        wraparound_mechanism<unsigned long long> frame_counter_wraparound;

    public:
        serial_timestamp_generator(int fps) : fps(fps), serial_frame_number(), last_timestamp(0), timestamp_wraparound(0, std::numeric_limits<uint32_t>::max()), frame_counter_wraparound(0, std::numeric_limits<uint32_t>::max())
        {
            assert(fps > 0);
            ts_step = 1000. / fps;
        }

        bool validate_frame(const subdevice_mode & /*mode*/, const void * /*frame*/) override { return true; }
        double get_frame_timestamp(const subdevice_mode &, const void *, double /*actual_fps*/) override
        {
            auto new_ts = timestamp_wraparound.fix(last_timestamp + ts_step);
            last_timestamp = new_ts;
            return new_ts;
        }
        unsigned long long get_frame_counter(const subdevice_mode &, const void *) override
        {
            return frame_counter_wraparound.fix(++serial_frame_number);
        }
    };

    // TODO refactor supported streams list to derived
    std::shared_ptr<frame_timestamp_reader> ds_device::create_frame_timestamp_reader(int subdevice) const
    {
        auto & stream_depth = get_stream_interface(RS_STREAM_DEPTH);
        auto & stream_infrared = get_stream_interface(RS_STREAM_INFRARED);
        auto & stream_infrared2 = get_stream_interface(RS_STREAM_INFRARED2);
        auto & stream_fisheye = get_stream_interface(RS_STREAM_FISHEYE);
        auto & stream_color = get_stream_interface(RS_STREAM_COLOR);

        switch (subdevice)
        {
        case SUB_DEVICE_DEPTH:
            if (stream_depth.is_enabled())
                return std::make_shared<dinghy_timestamp_reader>(stream_depth.get_framerate());
            break;
        case SUB_DEVICE_INFRARED: 
            if (stream_infrared.is_enabled())
                return std::make_shared<dinghy_timestamp_reader>(stream_infrared.get_framerate());

            if (stream_infrared2.is_enabled())
                return std::make_shared<dinghy_timestamp_reader>(stream_infrared2.get_framerate());
            break;
        case SUB_DEVICE_FISHEYE:
            if (stream_fisheye.is_enabled())
                return std::make_shared<fisheye_timestamp_reader>(stream_fisheye.get_framerate(), get_camera_info(RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION));
            break;
        case SUB_DEVICE_COLOR:
            if (stream_color.is_enabled())
            {
                // W/A for DS4 issue: when running at Depth 60 & Color 30 it seems that the frame counter is being incremented on every
                // depth frame (or ir/ir2). This means we need to reduce color frame number accordingly
                // In addition, the frame number is embedded only in YUYV format
                bool depth_streams_active = stream_depth.is_enabled() | stream_infrared.is_enabled() | stream_infrared2.is_enabled();

                if (depth_streams_active && (stream_color.get_format() != rs_format::RS_FORMAT_RAW16))
                {
                    auto master_fps = stream_depth.is_enabled() ? stream_depth.get_framerate() : 0;
                    master_fps = stream_infrared.is_enabled() ? stream_infrared.get_framerate() : master_fps;
                    master_fps = stream_infrared2.is_enabled() ? stream_infrared2.get_framerate() : master_fps;
                    auto scale = master_fps / stream_color.get_framerate();

                    return std::make_shared<color_timestamp_reader>(stream_color.get_framerate(), scale);
                }
                else
                {
                    return std::make_shared<serial_timestamp_generator>(stream_color.get_framerate());
                }
            }
            break;
        }

        // No streams enabled, so no need for a timestamp converter
        return nullptr;
    }

    std::vector<std::shared_ptr<frame_timestamp_reader>> ds_device::create_frame_timestamp_readers() const 
    {
        return { create_frame_timestamp_reader(SUB_DEVICE_INFRARED),
                 create_frame_timestamp_reader(SUB_DEVICE_DEPTH),
                 create_frame_timestamp_reader(SUB_DEVICE_COLOR),
                 create_frame_timestamp_reader(SUB_DEVICE_FISHEYE) };
    }
}
