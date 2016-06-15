// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <cstring>
#include <climits>
#include <algorithm>
#include <iostream>

#include "image.h"
#include "ds-private.h"
#include "ds-camera.h"

using namespace rsimpl;
using namespace rsimpl::motion_module;

namespace rsimpl
{
    ds_camera::ds_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) 
    : rs_device_base(device, info)
    {
        rs_option opt[] = {RS_OPTION_R200_DEPTH_UNITS};
        double units;
        get_options(opt, 1, &units);
        on_update_depth_units(static_cast<int>(units));
    }
    
    ds_camera::~ds_camera()
    {

    }

    bool ds_camera::is_disparity_mode_enabled() const
    {
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        return depth.is_enabled() && depth.get_format() == RS_FORMAT_DISPARITY16;
    }

    void ds_camera::on_update_depth_units(uint32_t units)
    {
        if(is_disparity_mode_enabled()) return;
        config.depth_scale = (float)units / 1000000; // Convert from micrometers to meters
    }

    void ds_camera::on_update_disparity_multiplier(double multiplier)
    {
        if(!is_disparity_mode_enabled()) return;
        auto & depth = get_stream_interface(RS_STREAM_DEPTH);
        float baseline = get_stream_interface(RS_STREAM_INFRARED2).get_extrinsics_to(depth).translation[0];
        config.depth_scale = static_cast<float>(depth.get_intrinsics().fx * baseline * multiplier);
    }

    void ds_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        auto & dev = get_device();
        auto minmax_writer = make_struct_interface<ds::range    >([&dev]() { return ds::get_min_max_depth(dev);           }, [&dev](ds::range     v) { ds::set_min_max_depth(dev,v);           });
        auto disp_writer   = make_struct_interface<ds::disp_mode>([&dev]() { return ds::get_disparity_mode(dev);          }, [&dev](ds::disp_mode v) { ds::set_disparity_mode(dev,v);          });
        auto ae_writer     = make_struct_interface<ds::ae_params>([&dev]() { return ds::get_lr_auto_exposure_params(dev); }, [&dev](ds::ae_params v) { ds::set_lr_auto_exposure_params(dev,v); });
        auto dc_writer     = make_struct_interface<ds::dc_params>([&dev]() { return ds::get_depth_params(dev);            }, [&dev](ds::dc_params v) { ds::set_depth_params(dev,v);            });

        for(int i=0; i<count; ++i)
        {
            if(uvc::is_pu_control(options[i]))
            {
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

            default: LOG_WARNING("Cannot set " << options[i] << " to " << values[i] << " on " << get_name()); break;
            }
        }

        minmax_writer.commit();
        disp_writer.commit();
        if(disp_writer.active) on_update_disparity_multiplier(disp_writer.struct_.disparity_multiplier);
        ae_writer.commit();
        dc_writer.commit();
    }

    void ds_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        auto & dev = get_device();
        auto minmax_reader = make_struct_interface<ds::range    >([&dev]() { return ds::get_min_max_depth(dev);           }, [&dev](ds::range     v) { ds::set_min_max_depth(dev,v);           });
        auto disp_reader   = make_struct_interface<ds::disp_mode>([&dev]() { return ds::get_disparity_mode(dev);          }, [&dev](ds::disp_mode v) { ds::set_disparity_mode(dev,v);          });
        auto ae_reader     = make_struct_interface<ds::ae_params>([&dev]() { return ds::get_lr_auto_exposure_params(dev); }, [&dev](ds::ae_params v) { ds::set_lr_auto_exposure_params(dev,v); });
        auto dc_reader     = make_struct_interface<ds::dc_params>([&dev]() { return ds::get_depth_params(dev);            }, [&dev](ds::dc_params v) { ds::set_depth_params(dev,v);            });

        for(int i=0; i<count; ++i)
        {

            if(uvc::is_pu_control(options[i]))
            {
                values[i] = uvc::get_pu_control(get_device(), 2, options[i]);
                continue;
            }

            switch(options[i])
            {

            //case RS_OPTION_FISHEYE_STROBE:             values[i] = r200::get_strobe            (get_device()); break;
            //case RS_OPTION_FISHEYE_EXT_TRIG:           values[i] = r200::get_ext_trig          (get_device()); break;

            case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED:                   values[i] = ds::get_lr_exposure_mode(get_device()); break;
            
            case RS_OPTION_R200_LR_GAIN: // Gain is framerate dependent
                ds::set_lr_gain_discovery(get_device(), {get_lr_framerate()});
                values[i] = ds::get_lr_gain(get_device()).value;
                break;
            case RS_OPTION_R200_LR_EXPOSURE: // Exposure is framerate dependent
                ds::set_lr_exposure_discovery(get_device(), {get_lr_framerate()});
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

            default: LOG_WARNING("Cannot get " << options[i] << " on " << get_name()); break;
            }
        }
    }

    //// The following four APIs are due to C interface conformance - TODO refactoring
    //void ds_camera::toggle_motion_module_power(bool on)
    //{        
    //    throw std::logic_error(to_string() << __FUNCTION__ << " Operation not allowed");
    //}

    //void ds_camera::toggle_motion_module_events(bool on)
    //{
    //    throw std::logic_error(to_string() << __FUNCTION__ << " Operation not allowed");
    //}

    // Power on motion module (mmpwr)
    //void ds_camera::start_motion_tracking()
    //{
    //    throw std::logic_error(to_string() << __FUNCTION__ << " Operation not allowed");
    //}

    //// Power down Motion Module
    //void ds_camera::stop_motion_tracking()
    //{
    //    throw std::logic_error(to_string() << __FUNCTION__ << " Operation not allowed");
    //}

    void ds_camera::on_before_start(const std::vector<subdevice_mode_selection> & selected_modes)
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
                break;
            }
        }
        ds::set_stream_intent(get_device(), streamIntent);
    }

    rs_stream ds_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
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

    uint32_t ds_camera::get_lr_framerate() const
    {
        for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            auto & stream = get_stream_interface(s);
            if(stream.is_enabled()) return static_cast<uint32_t>(stream.get_framerate());
        }
        return 30; // If no streams have yet been enabled, return the minimum possible left/right framerate, to allow the maximum possible exposure range
    }

    /*void ds_camera::set_xu_option(rs_option option, int value)
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
    }*/
    
    bool ds_camera::supports_option(rs_option option) const
    {
        // We have special logic to implement LR gain and exposure, so they do not belong to the standard option list
        return option == RS_OPTION_R200_LR_GAIN || option == RS_OPTION_R200_LR_EXPOSURE || rs_device_base::supports_option(option);
    }

    void ds_camera::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
    {
        // Gain min/max is framerate dependent
        if(option == RS_OPTION_R200_LR_GAIN)
        {
            ds::set_lr_gain_discovery(get_device(), {get_lr_framerate()});
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
            ds::set_lr_exposure_discovery(get_device(), {get_lr_framerate()});
            auto disc = ds::get_lr_exposure_discovery(get_device());
            min = disc.min;
            max = disc.max;
            step = 1;
            def = disc.default_value;
            return;
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
        int max_fps;
        std::mutex mutex;
        unsigned last_fisheye_counter;
    public:
        dinghy_timestamp_reader(int max_fps) : max_fps(max_fps),last_fisheye_counter(0) {}

        bool validate_frame(const subdevice_mode & mode, const void * frame) const override 
        {
            if (mode.subdevice == 3) // Fisheye camera
                return true;

            // No dinghy available on YUY2 images
            if(mode.pf.fourcc == pf_yuy2.fourcc) return true;

            // Check magic number for all subdevices
            auto & dinghy = get_dinghy(mode, frame);
            const uint32_t magic_numbers[] = {0x08070605, 0x04030201, 0x8A8B8C8D};
            if(dinghy.magicNumber != magic_numbers[mode.subdevice])
            {
                LOG_WARNING("Subdevice " << mode.subdevice << " bad magic number 0x" << std::hex << dinghy.magicNumber);
                return false;
            }

            // Check frame status for left/right/Z subdevices only
            if(dinghy.frameStatus != 0 && mode.subdevice != 2)
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

        struct byte_wrapping{
             unsigned char lsb : 4;
             unsigned char msb : 4;
         };

        unsigned fix_fisheye_counter(unsigned char pixel)
         {
             std::lock_guard<std::mutex> guard(mutex);

             auto last_counter_lsb = reinterpret_cast<byte_wrapping&>(last_fisheye_counter).lsb;
             auto pixel_lsb = reinterpret_cast<byte_wrapping&>(pixel).lsb;
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
             return fixed_counter;
         }

        int get_frame_timestamp(const subdevice_mode & mode, const void * frame) override 
        { 
            int frame_number = 0;
            if (mode.subdevice == 3) // Fisheye
            {
                frame_number = fix_fisheye_counter(*((unsigned char*)frame));
            }
            else if(mode.pf.fourcc == pf_yuy2.fourcc)
            {
                // YUY2 images encode the frame number in the low order bits of the final 32 bytes of the image
                auto data = reinterpret_cast<const uint8_t *>(frame) + ((mode.native_dims.x * mode.native_dims.y) - 32) * 2;
                for(int i = 0; i < 32; ++i)
                {
                    frame_number |= ((*data & 1) << (i & 1 ? 32 - i : 30 - i));
                    data += 2;
                }
            }
            else frame_number = get_dinghy(mode, frame).frameCount; // All other formats can use the frame number in the dinghy row
            return frame_number * 1000 / max_fps;
        }
        int get_frame_counter(const subdevice_mode & mode, const void * frame) override
        {
            int frame_number = 0;
            if (mode.subdevice == 3) // Fisheye
            {
                frame_number = fix_fisheye_counter(*((unsigned char*)frame));
            }
            else if (mode.pf.fourcc == pf_yuy2.fourcc)
            {
                // YUY2 images encode the frame number in the low order bits of the final 32 bytes of the image
                auto data = reinterpret_cast<const uint8_t *>(frame) + ((mode.native_dims.x * mode.native_dims.y) - 32) * 2;
                for (int i = 0; i < 32; ++i)
                {
                    frame_number |= ((*data & 1) << (i & 1 ? 32 - i : 30 - i));
                    data += 2;
                }
            }
            else frame_number = get_dinghy(mode, frame).frameCount; // All other formats can use the frame number in the dinghy row
            return frame_number;
        }
    };

    class serial_timestamp_generator : public frame_timestamp_reader
    {
        int fps, serial_frame_number;
    public:
        serial_timestamp_generator(int fps) : fps(fps), serial_frame_number() {}

        bool validate_frame(const subdevice_mode & mode, const void * frame) const override { return true; }
        int get_frame_timestamp(const subdevice_mode &, const void *) override 
        { 
            ++serial_frame_number;
            return serial_frame_number * 1000 / fps;
        }
        int get_frame_counter(const subdevice_mode &, const void *) override
        {
            return serial_frame_number;
        }
    };

	// TODO refactor supported streams list to derived
    std::shared_ptr<frame_timestamp_reader> ds_camera::create_frame_timestamp_reader() const
    {
        // If left, right, or Z streams are enabled, convert frame numbers to millisecond timestamps based on LRZ framerate
        for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2, RS_STREAM_FISHEYE})
        {
            auto & si = get_stream_interface(s);
            if(si.is_enabled()) return std::make_shared<dinghy_timestamp_reader>(si.get_framerate());
        }

        // If only color stream is enabled, generate serial frame timestamps (no HW frame numbers available)
        auto & si = get_stream_interface(RS_STREAM_COLOR);
        if(si.is_enabled()) return std::make_shared<serial_timestamp_generator>(si.get_framerate());

        // No streams enabled, so no need for a timestamp converter
        return nullptr;
    }
}
