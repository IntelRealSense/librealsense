/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#include "r200.h"
#include "r200-private.h"
#include "image.h"

#include <cstring>
#include <climits>
#include <algorithm>

namespace rsimpl
{
    static int amount = 0;

    template<unsigned MAGIC_NUMBER>
    int decode_dinghy_frame_number(const subdevice_mode & mode, const void * frame)
    {
        auto dinghy = reinterpret_cast<const r200::Dinghy *>(reinterpret_cast<const uint8_t *>(frame) + mode.pf->get_image_size(mode.native_dims.x, mode.native_dims.y-1));
        return dinghy->magicNumber == MAGIC_NUMBER ? dinghy->frameCount : 0;
    }

    int decode_yuy2_frame_number(const subdevice_mode & mode, const void * frame)
    {
        assert(mode.pf == &pf_yuy2);

        auto data = reinterpret_cast<const uint8_t *>(frame) + ((mode.native_dims.x * mode.native_dims.y) - 32) * 2;
        int number = 0;
        for(int i = 0; i < 32; ++i)
        {
            number |= ((*data & 1) << (i & 1 ? 32 - i : 30 - i));
            data += 2;
        }
        return number;
    }

    r200_camera::r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) : rs_device(device, info)
    {
        on_update_depth_units(get_xu_option(RS_OPTION_R200_DEPTH_UNITS));
    }
    
    r200_camera::~r200_camera()
    {

    }

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense R200");

		// Retrieve the extension unit for the R200's left/right infrared camera
        // This XU is used for all commands related to retrieving calibration information and setting depth generation settings
        const uvc::guid R200_LEFT_RIGHT_XU = {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}};
        init_controls(*device, 0, R200_LEFT_RIGHT_XU);
        
        static_device_info info;
        info.name = {"Intel RealSense R200"};
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_COLOR] = 2;
        info.stream_subdevices[RS_STREAM_INFRARED ] = 0;
        info.stream_subdevices[RS_STREAM_INFRARED2] = 0;

        auto c = r200::read_camera_info(*device);
        
        // Set up modes for left/right/z images
        for(auto fps : {30, 60, 90})
        {
            // Subdevice 0 can provide left/right infrared via four pixel formats, in three resolutions, which can either be uncropped or cropped to match Z
            for(auto pf : {&pf_y8, &pf_y8i, &pf_y16, &pf_y12i})
            {
                info.subdevice_modes.push_back({0, {640, 481}, pf, fps, c.modesLR[0], {}, {0, -6}, &decode_dinghy_frame_number<0x08070605>});  
                info.subdevice_modes.push_back({0, {640, 373}, pf, fps, c.modesLR[1], {}, {0, -6}, &decode_dinghy_frame_number<0x08070605>});  
                info.subdevice_modes.push_back({0, {640, 254}, pf, fps, c.modesLR[2], {}, {0, -6}, &decode_dinghy_frame_number<0x08070605>});  
            }

            // Subdevice 1 can provide depth, in three resolutions, which can either be unpadded or padded to match left/right
            info.subdevice_modes.push_back({1, {628, 469}, &pf_z16,  fps, pad_crop_intrinsics(c.modesLR[0], -6), {}, {0, +6}, &decode_dinghy_frame_number<0x4030201>});
            info.subdevice_modes.push_back({1, {628, 361}, &pf_z16,  fps, pad_crop_intrinsics(c.modesLR[1], -6), {}, {0, +6}, &decode_dinghy_frame_number<0x4030201>});
            info.subdevice_modes.push_back({1, {628, 242}, &pf_z16,  fps, pad_crop_intrinsics(c.modesLR[2], -6), {}, {0, +6}, &decode_dinghy_frame_number<0x4030201>});
        }

        // Subdevice 2 can provide color, in several formats and framerates
        info.subdevice_modes.push_back({2, { 320,  240}, &pf_yuy2, 60, scale_intrinsics(c.intrinsicsThird[1], 320, 240), {scale_intrinsics(c.modesThird[1][0], 320, 240)}, {0}, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, { 320,  240}, &pf_yuy2, 30, scale_intrinsics(c.intrinsicsThird[1], 320, 240), {scale_intrinsics(c.modesThird[1][0], 320, 240)}, {0}, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, { 640,  480}, &pf_yuy2, 60, c.intrinsicsThird[1], {c.modesThird[1][0]}, {0}, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, { 640,  480}, &pf_yuy2, 30, c.intrinsicsThird[1], {c.modesThird[1][0]}, {0}, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, {1920, 1080}, &pf_yuy2, 30, c.intrinsicsThird[0], {c.modesThird[0][0]}, {0}, &decode_yuy2_frame_number, true});
        info.subdevice_modes.push_back({2, {2400, 1081}, &pf_rw10, 30, c.intrinsicsThird[0], {c.modesThird[0][0]}, {0}, &decode_dinghy_frame_number<0x8A8B8C8D>, true});
		// todo - add 15 fps modes

        // Set up interstream rules for left/right/z images
        for(auto ir : {RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::width, 0, 12});
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::height, 0, 12});
            info.interstream_rules.push_back({RS_STREAM_DEPTH, ir, &stream_request::fps, 0, 0});
        }

        info.presets[RS_STREAM_INFRARED][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_BEST_QUALITY] = {true, 480, 360, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_BEST_QUALITY] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Y8,   60};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_LARGEST_IMAGE] = {true,  640,  480, RS_FORMAT_Z16,  60};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_LARGEST_IMAGE] = {true, 1920, 1080, RS_FORMAT_RGB8, 30};

        info.presets[RS_STREAM_INFRARED][RS_PRESET_HIGHEST_FRAMERATE] = {true, 320, 240, RS_FORMAT_Y8,   90};
        info.presets[RS_STREAM_DEPTH   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 320, 240, RS_FORMAT_Z16,  90};
        info.presets[RS_STREAM_COLOR   ][RS_PRESET_HIGHEST_FRAMERATE] = {true, 640, 480, RS_FORMAT_RGB8, 60};

        for(int i=0; i<RS_PRESET_COUNT; ++i) 
			info.presets[RS_STREAM_INFRARED2][i] = info.presets[RS_STREAM_INFRARED][i];

        for(int i = RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED; i <= RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD; ++i)
			info.option_supported[i] = true;

        // We select the depth/left infrared camera's viewpoint to be the origin
        info.stream_poses[RS_STREAM_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        info.stream_poses[RS_STREAM_INFRARED] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};

        // The right infrared camera is offset along the +x axis by the baseline (B)
        info.stream_poses[RS_STREAM_INFRARED2] = {{{1,0,0},{0,1,0},{0,0,1}}, {c.B * 0.001f, 0, 0}}; // Sterling comment

		// The transformation between the depth camera and third camera is described by a translation vector (T), followed by rotation matrix (Rthird)
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) 
			info.stream_poses[RS_STREAM_COLOR].orientation(i,j) = c.Rthird[i*3+j];
        for(int i=0; i<3; ++i) 
			info.stream_poses[RS_STREAM_COLOR].position[i] = c.T[i] * 0.001f;

        // Our position is added AFTER orientation is applied, not before, so we must multiply Rthird * T to compute it
        info.stream_poses[RS_STREAM_COLOR].position = info.stream_poses[RS_STREAM_COLOR].orientation * info.stream_poses[RS_STREAM_COLOR].position;
        info.nominal_depth_scale = 0.001f;
        info.serial = std::to_string(c.serial_number);
        info.firmware_version = r200::read_firmware_version(*device);

		// On LibUVC backends, the R200 should use four transfer buffers
        info.num_libuvc_transfer_buffers = 4;
        return std::make_shared<r200_camera>(device, info);
    }

    bool r200_camera::is_disparity_mode_enabled() const
    {
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        return depth.is_enabled() && depth.get_format() == RS_FORMAT_DISPARITY16;
    }

    void r200_camera::on_update_depth_units(int units)
    {
        if(is_disparity_mode_enabled()) return;
        config.depth_scale = (float)units / 1000000; // Convert from micrometers to meters
    }

    void r200_camera::on_update_disparity_multiplier(float multiplier)
    {
        if(!is_disparity_mode_enabled()) return;
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        float baseline = get_stream_interface(RS_STREAM_INFRARED2).get_extrinsics_to(depth).translation[0];
        config.depth_scale = depth.get_intrinsics().fx * baseline * multiplier;
    }

    // This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
    // Additionally, it will ensure that any fields not set in a given struct will retain their original values
    template<class T> struct struct_interface
    {
        uvc::device & dev;
        T (*reader)(const uvc::device &);
        void (*writer)(uvc::device &, T);
        T struct_;
        bool active;

        struct_interface(uvc::device & dev, T (*reader)(const uvc::device &), void (*writer)(uvc::device &, T) = nullptr) : dev(dev), reader(reader), writer(writer), active(false) {}

        template<class U> double get(U T::* field)
        {
            if(!active)
            {
                struct_ = reader(dev);
                active = true;
            }
            return struct_.*field;
        }

        template<class U> void set(U T::* field, double value)
        {
            if(!active)
            {
                struct_ = reader(dev);
                active = true;
            }
            struct_.*field = static_cast<U>(value);
        }

        void commit()
        {
            if(active)
            {
                writer(dev, struct_);
            }
        }
    };

    void r200_camera::set_options(const rs_option options[], int count, const double values[])
    {
        struct_interface<r200::range    > minmax_writer (get_device(), &r200::get_min_max_depth,  &r200::set_min_max_depth );
        struct_interface<r200::disp_mode> disp_writer   (get_device(), &r200::get_disparity_mode, &r200::set_disparity_mode);
        struct_interface<r200::dc_params> dc_writer     (get_device(), &r200::get_depth_params,   &r200::set_depth_params  );

        for(int i=0; i<count; ++i)
        {
            switch(options[i])
            {
            case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION    : 
            case RS_OPTION_COLOR_BRIGHTNESS                : 
            case RS_OPTION_COLOR_CONTRAST                  : 
            case RS_OPTION_COLOR_EXPOSURE                  : 
            case RS_OPTION_COLOR_GAIN                      : 
            case RS_OPTION_COLOR_GAMMA                     : 
            case RS_OPTION_COLOR_HUE                       : 
            case RS_OPTION_COLOR_SATURATION                : 
            case RS_OPTION_COLOR_SHARPNESS                 : 
            case RS_OPTION_COLOR_WHITE_BALANCE             : 
            case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE      : 
            case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE : 
                uvc::set_pu_control(get_device(), 2, options[i], static_cast<int>(values[i]));
                break;

            case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:                   r200::set_lr_exposure_mode(get_device(), values[i]); break;
            case RS_OPTION_R200_LR_GAIN:                                    r200::set_lr_gain(get_device(), {get_lr_framerate(), values[i]}); break; // TODO: May need to set this on start if framerate changes
            case RS_OPTION_R200_LR_EXPOSURE:                                r200::set_lr_exposure(get_device(), {get_lr_framerate(), values[i]}); break; // TODO: May need to set this on start if framerate changes
            case RS_OPTION_R200_EMITTER_ENABLED:                            r200::set_emitter_state(get_device(), !!values[i]); break;
            case RS_OPTION_R200_DEPTH_UNITS:                                r200::set_depth_units(get_device(), values[i]); on_update_depth_units(values[i]); break;

            case RS_OPTION_R200_DEPTH_CLAMP_MIN:                            minmax_writer.set(&r200::range::min, values[i]); break;
            case RS_OPTION_R200_DEPTH_CLAMP_MAX:                            minmax_writer.set(&r200::range::max, values[i]); break;

            case RS_OPTION_R200_DISPARITY_MULTIPLIER:                       disp_writer.set(&r200::disp_mode::disparity_multiplier, values[i]); break;
            case RS_OPTION_R200_DISPARITY_SHIFT:                            r200::set_disparity_shift(get_device(), values[i]); break;

            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT:    dc_writer.set(&r200::dc_params::robbins_munroe_minus_inc, values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT:    dc_writer.set(&r200::dc_params::robbins_munroe_plus_inc,  values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD:             dc_writer.set(&r200::dc_params::median_thresh,            values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD:      dc_writer.set(&r200::dc_params::score_min_thresh,         values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD:      dc_writer.set(&r200::dc_params::score_max_thresh,         values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD:      dc_writer.set(&r200::dc_params::texture_count_thresh,     values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD: dc_writer.set(&r200::dc_params::texture_diff_thresh,      values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD:        dc_writer.set(&r200::dc_params::second_peak_thresh,       values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD:           dc_writer.set(&r200::dc_params::neighbor_thresh,          values[i]); break;
            case RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD:                 dc_writer.set(&r200::dc_params::lr_thresh,                values[i]); break;

            default: LOG_WARNING("Cannot set " << options[i] << " to " << values[i] << " on " << get_name()); break;
            }
        }

        minmax_writer.commit();
        disp_writer.commit();
        if(disp_writer.active) on_update_disparity_multiplier(disp_writer.struct_.disparity_multiplier);
        dc_writer.commit();
    }

    void r200_camera::get_options(const rs_option options[], int count, double values[])
    {
        struct_interface<r200::range    > minmax_reader (get_device(), &r200::get_min_max_depth );
        struct_interface<r200::disp_mode> disp_reader   (get_device(), &r200::get_disparity_mode);
        struct_interface<r200::dc_params> dc_reader     (get_device(), &r200::get_depth_params  );    

        for(int i=0; i<count; ++i)
        {
            switch(options[i])
            {
            case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION    : 
            case RS_OPTION_COLOR_BRIGHTNESS                : 
            case RS_OPTION_COLOR_CONTRAST                  : 
            case RS_OPTION_COLOR_EXPOSURE                  : 
            case RS_OPTION_COLOR_GAIN                      : 
            case RS_OPTION_COLOR_GAMMA                     : 
            case RS_OPTION_COLOR_HUE                       : 
            case RS_OPTION_COLOR_SATURATION                : 
            case RS_OPTION_COLOR_SHARPNESS                 : 
            case RS_OPTION_COLOR_WHITE_BALANCE             : 
            case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE      : 
            case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE : 
                values[i] = uvc::get_pu_control(get_device(), 2, options[i]);
                break;

            case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:                   values[i] = r200::get_lr_exposure_mode(get_device()); break;
            
            case RS_OPTION_R200_LR_GAIN: // Gain is framerate dependent
                r200::set_lr_gain_discovery(get_device(), {get_lr_framerate()});
                values[i] = r200::get_lr_gain(get_device()).value;
                break;
            case RS_OPTION_R200_LR_EXPOSURE: // Exposure is framerate dependent
                r200::set_lr_exposure_discovery(get_device(), {get_lr_framerate()});
                values[i] = r200::get_lr_exposure(get_device()).value;
                break;
            case RS_OPTION_R200_EMITTER_ENABLED:
                values[i] = r200::get_emitter_state(get_device(), is_capturing(), get_stream_interface(RS_STREAM_DEPTH).is_enabled());
                break;

            case RS_OPTION_R200_DEPTH_UNITS:                                values[i] = r200::get_depth_units(get_device());  break;

            case RS_OPTION_R200_DEPTH_CLAMP_MIN:                            values[i] = minmax_reader.get(&r200::range::min); break;
            case RS_OPTION_R200_DEPTH_CLAMP_MAX:                            values[i] = minmax_reader.get(&r200::range::max); break;

            case RS_OPTION_R200_DISPARITY_MULTIPLIER:                       values[i] = disp_reader.get(&r200::disp_mode::disparity_multiplier); break;
            case RS_OPTION_R200_DISPARITY_SHIFT:                            values[i] = r200::get_disparity_shift(get_device()); break;

            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT:    values[i] = dc_reader.get(&r200::dc_params::robbins_munroe_minus_inc); break;
            case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT:    values[i] = dc_reader.get(&r200::dc_params::robbins_munroe_plus_inc ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD:             values[i] = dc_reader.get(&r200::dc_params::median_thresh           ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD:      values[i] = dc_reader.get(&r200::dc_params::score_min_thresh        ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD:      values[i] = dc_reader.get(&r200::dc_params::score_max_thresh        ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD:      values[i] = dc_reader.get(&r200::dc_params::texture_count_thresh    ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD: values[i] = dc_reader.get(&r200::dc_params::texture_diff_thresh     ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD:        values[i] = dc_reader.get(&r200::dc_params::second_peak_thresh      ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD:           values[i] = dc_reader.get(&r200::dc_params::neighbor_thresh         ); break;
            case RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD:                 values[i] = dc_reader.get(&r200::dc_params::lr_thresh               ); break;

            default: LOG_WARNING("Cannot set " << options[i] << " to " << values[i] << " on " << get_name()); break;
            }
        }
    }

    void r200_camera::on_before_start(const std::vector<subdevice_mode_selection> & selected_modes)
    {
        uint8_t streamIntent = 0;
        for(const auto & m : selected_modes)
        {
            switch(m.mode->subdevice)
            {
            case 0: streamIntent |= r200::STATUS_BIT_LR_STREAMING; break;
            case 2: streamIntent |= r200::STATUS_BIT_WEB_STREAMING; break;
            case 1: 
                streamIntent |= r200::STATUS_BIT_Z_STREAMING; 
                auto dm = r200::get_disparity_mode(get_device());
                switch(m.get_format(RS_STREAM_DEPTH))
                {
                default: throw std::logic_error("unsupported R200 depth format");
                case RS_FORMAT_Z16: 
                    dm.is_disparity_enabled = 0;
                    on_update_depth_units(get_xu_option(RS_OPTION_R200_DEPTH_UNITS));
                    break;
                case RS_FORMAT_DISPARITY16: 
                    dm.is_disparity_enabled = 1;
                    on_update_disparity_multiplier(static_cast<float>(dm.disparity_multiplier));
                    break;
                }
                r200::set_disparity_mode(get_device(), dm);
                break;
            }
        }
        r200::set_stream_intent(get_device(), streamIntent);
    }

    int r200_camera::convert_timestamp(int64_t timestamp) const
    { 
        int max_fps = 0;
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
        {
            if(get_stream_interface((rs_stream)i).is_enabled())
            {
                max_fps = std::max(max_fps, get_stream_interface((rs_stream)i).get_framerate());
            }
        }
        return static_cast<int>(timestamp * 1000 / max_fps);
    }

    int r200_camera::get_lr_framerate() const
    {
        for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            auto & stream = get_stream_interface(s);
            if(stream.is_enabled()) return stream.get_framerate();
        }
        return 30;
    }

    void r200_camera::set_xu_option(rs_option option, int value)
    {
        if(is_capturing())
        {
            switch(option)
            {
            case RS_OPTION_R200_DEPTH_UNITS:
            case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            case RS_OPTION_R200_DISPARITY_SHIFT:
                throw std::runtime_error("cannot set this option after rs_start_capture(...)");
            }
        }

        // todo - Range check value before write
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:
            r200::set_lr_exposure_mode(get_device(), value);
            break;
        case RS_OPTION_R200_LR_GAIN:
            r200::set_lr_gain(get_device(), {get_lr_framerate(), value}); // TODO: May need to set this on start if framerate changes
            break;
        case RS_OPTION_R200_LR_EXPOSURE:
            r200::set_lr_exposure(get_device(), {get_lr_framerate(), value}); // TODO: May need to set this on start if framerate changes
            break;
        case RS_OPTION_R200_EMITTER_ENABLED:
            r200::set_emitter_state(get_device(), !!value);
            break;
        case RS_OPTION_R200_DEPTH_UNITS:
            r200::set_depth_units(get_device(), value);
            on_update_depth_units(value);
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:
            r200::set_min_max_depth(get_device(), {value, r200::get_min_max_depth(get_device()).max});
            break;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:
            r200::set_min_max_depth(get_device(), {r200::get_min_max_depth(get_device()).min, value});
            break;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:
            r200::set_disparity_mode(get_device(), {r200::get_disparity_mode(get_device()).is_disparity_enabled, value});
            on_update_disparity_multiplier(static_cast<float>(value));
            break;
        case RS_OPTION_R200_DISPARITY_SHIFT:
            r200::set_disparity_shift(get_device(), value);
            break;
        }
    }

    void r200_camera::get_xu_range(rs_option option, int * min, int * max)
    {
        // Gain min/max is framerate dependent
        if(option == RS_OPTION_R200_LR_GAIN)
        {
            r200::set_lr_gain_discovery(get_device(), {get_lr_framerate()});
            auto disc = r200::get_lr_gain_discovery(get_device());
            if(min) *min = disc.min;
            if(max) *max = disc.max;
            return;
        }

        // Exposure min/max is framerate dependent
        if(option == RS_OPTION_R200_LR_EXPOSURE)
        {
            r200::set_lr_exposure_discovery(get_device(), {get_lr_framerate()});
            auto disc = r200::get_lr_exposure_discovery(get_device());
            if(min) *min = disc.min;
            if(max) *max = disc.max;
            return;
        }

        const struct { rs_option option; int min, max; } ranges[] = {
            {RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 0, 1},
            {RS_OPTION_R200_EMITTER_ENABLED, 0, 1},
            {RS_OPTION_R200_DEPTH_UNITS, 1, INT_MAX}, // What is the real range?
            {RS_OPTION_R200_DEPTH_CLAMP_MIN, 0, USHRT_MAX},
            {RS_OPTION_R200_DEPTH_CLAMP_MAX, 0, USHRT_MAX},
            {RS_OPTION_R200_DISPARITY_MULTIPLIER, 1, 1000},
            {RS_OPTION_R200_DISPARITY_SHIFT, 0, 0},

            {RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT,    0, 0xFF },
            {RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT,    0, 0xFF },
            {RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD,             0, 0x3FF},
            {RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD,      0, 0x3FF},
            {RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD,      0, 0x3FF},
            {RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD,      0, 0x1F },
            {RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD, 0, 0x3FF},
            {RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD,        0, 0x3FF},
            {RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD,           0, 0x3FF},
            {RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD,                 0, 0x7FF},
        };
        for(auto & r : ranges)
        {
            if(option != r.option) continue;
            if(min) *min = r.min;
            if(max) *max = r.max;
            return;
        }
        throw std::logic_error("range not specified");
    }

    int r200_camera::get_xu_option(rs_option option)
    {
        switch(option)
        {
        case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED: return r200::get_lr_exposure_mode(get_device());
        case RS_OPTION_R200_LR_GAIN: // Gain is framerate dependent
            r200::set_lr_gain_discovery(get_device(), {get_lr_framerate()});
            return r200::get_lr_gain(get_device()).value;
        case RS_OPTION_R200_LR_EXPOSURE: // Exposure is framerate dependent
            r200::set_lr_exposure_discovery(get_device(), {get_lr_framerate()});
            return r200::get_lr_exposure(get_device()).value;
        case RS_OPTION_R200_EMITTER_ENABLED:          return r200::get_emitter_state(get_device(), is_capturing(), get_stream_interface(RS_STREAM_DEPTH).is_enabled());
        case RS_OPTION_R200_DEPTH_UNITS:              return r200::get_depth_units(get_device());
        case RS_OPTION_R200_DEPTH_CLAMP_MIN:          return r200::get_min_max_depth(get_device()).min;
        case RS_OPTION_R200_DEPTH_CLAMP_MAX:          return r200::get_min_max_depth(get_device()).max;
        case RS_OPTION_R200_DISPARITY_MULTIPLIER:     return static_cast<int>(r200::get_disparity_mode(get_device()).disparity_multiplier);
        case RS_OPTION_R200_DISPARITY_SHIFT:          return r200::get_disparity_shift (get_device());
        default: return 0;
        }
    }

    // Note: The external and internal algorithm structs have been deliberately left as separate types. The internal structs are intended to be 1:1
    // representations of the exact binary data transferred to the UVC extension unit control. We want to allow for these structs to change with
    // future firmware versions or devices without affecting the API of the library.
 
    void r200_camera::set_lr_auto_exposure_parameters(const rs_r200_lr_auto_exposure_parameters & p)
    {
        r200::set_lr_auto_exposure_params(get_device(), {p.mean_intensity_set_point, p.bright_ratio_set_point, p.kp_gain, p.kp_exposure, p.kp_dark_threshold,
            p.exposure_top_edge, p.exposure_bottom_edge, p.exposure_left_edge, p.exposure_right_edge});
    }

    rs_r200_lr_auto_exposure_parameters r200_camera::get_lr_auto_exposure_parameters() const
    {
        auto p = r200::get_lr_auto_exposure_params(get_device());
        return {p.mean_intensity_set_point, p.bright_ratio_set_point, p.kp_gain, p.kp_exposure, p.kp_dark_threshold,
            p.exposure_top_edge, p.exposure_bottom_edge, p.exposure_left_edge, p.exposure_right_edge};
    }

    void r200_camera::set_depth_control_parameters(const rs_r200_depth_control_parameters & p)
    {
        r200::set_depth_params(get_device(), {p.estimate_median_decrement, p.estimate_median_increment, p.median_threshold, p.score_minimum_threshold, p.score_maximum_threshold,
            p.texture_count_threshold, p.texture_difference_threshold, p.second_peak_threshold, p.neighbor_threshold, p.lr_threshold});
    }

    rs_r200_depth_control_parameters r200_camera::get_depth_control_parameters() const
    {
        const auto p = r200::get_depth_params(get_device());
        return {p.robbins_munroe_minus_inc, p.robbins_munroe_plus_inc, p.median_thresh, p.score_min_thresh, p.score_max_thresh,
            p.texture_count_thresh, p.texture_diff_thresh, p.second_peak_thresh, p.neighbor_thresh, p.lr_thresh};
    }
}
