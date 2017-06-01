// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>
#include <climits>
#include <algorithm>

#include "image.h"
#include "ivcam-private.h"
#include "ivcam-device.h"

namespace rsimpl
{
    rs_intrinsics MakeDepthIntrinsics(const ivcam::camera_calib_params & c, const int2 & dims)
    {
        return{ dims.x, dims.y, (c.Kc[0][2] * 0.5f + 0.5f) * dims.x, (c.Kc[1][2] * 0.5f + 0.5f) * dims.y, c.Kc[0][0] * 0.5f * dims.x, c.Kc[1][1] * 0.5f * dims.y,
            RS_DISTORTION_INVERSE_BROWN_CONRADY, {c.Invdistc[0], c.Invdistc[1], c.Invdistc[2], c.Invdistc[3], c.Invdistc[4]} };
    }

    rs_intrinsics MakeColorIntrinsics(const ivcam::camera_calib_params & c, const int2 & dims)
    {
        rs_intrinsics intrin = { dims.x, dims.y, c.Kt[0][2] * 0.5f + 0.5f, c.Kt[1][2] * 0.5f + 0.5f, c.Kt[0][0] * 0.5f, c.Kt[1][1] * 0.5f, RS_DISTORTION_NONE, {} };
        if (dims.x * 3 == dims.y * 4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
        {
            intrin.fx *= 4.0f / 3;
            intrin.ppx *= 4.0f / 3;
            intrin.ppx -= 1.0f / 6;
        }
        intrin.fx *= dims.x;
        intrin.fy *= dims.y;
        intrin.ppx *= dims.x;
        intrin.ppy *= dims.y;
        return intrin;
    }

    void update_supported_options(uvc::device& dev,
        const uvc::extension_unit depth_xu,
        const std::vector <std::pair<rs_option, char>> options,
        std::vector<supported_option>& supported_options)
    {
        for (auto p : options)
        {
            int min, max, step, def;
            get_extension_control_range(dev, depth_xu, p.second, &min, &max, &step, &def);
            supported_option so;
            so.option = p.first;
            so.min = min;
            so.max = max;
            so.step = step;
            so.def = def;
            supported_options.push_back(so);
        }
    }

    iv_camera::iv_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib) :
        rs_device_base(device, info),
        base_calibration(calib)
    {
    }

    void iv_camera::start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex)
    {
        rs_device_base::start_fw_logger(fw_log_op_code, grab_rate_in_ms, mutex);
    }

    void iv_camera::stop_fw_logger()
    {
        rs_device_base::stop_fw_logger();
    }

    void iv_camera::on_before_start(const std::vector<subdevice_mode_selection> & /*selected_modes*/)
    {
    }

    rs_stream iv_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
    {
        int fps[RS_STREAM_NATIVE_COUNT] = {}, max_fps = 0;
        for (const auto & m : selected_modes)
        {
            for (const auto & output : m.get_outputs())
            {
                fps[output.first] = m.mode.fps;
                max_fps = std::max(max_fps, m.mode.fps);
            }
        }

        // Prefer to sync on depth or infrared, but select the stream running at the fastest framerate
        for (auto s : { RS_STREAM_DEPTH, RS_STREAM_INFRARED2, RS_STREAM_INFRARED, RS_STREAM_COLOR })
        {
            if (fps[s] == max_fps) return s;
        }
        return RS_STREAM_DEPTH;
    }

    void iv_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i < count; ++i)
        {

            if (uvc::is_pu_control(options[i]))
            {
                // Disabling auto-setting controls, if needed
                switch (options[i])
                {
                case RS_OPTION_COLOR_WHITE_BALANCE:     disable_auto_option( 0, RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE); break;
                case RS_OPTION_COLOR_EXPOSURE:          disable_auto_option( 0, RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE); break;
                default:  break;
                }

                uvc::set_pu_control_with_retry(get_device(), 0, options[i], static_cast<int>(values[i]));
                continue;
            }

            switch (options[i])
            {
            case RS_OPTION_F200_LASER_POWER:          ivcam::set_laser_power(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_F200_ACCURACY:             ivcam::set_accuracy(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_F200_MOTION_RANGE:         ivcam::set_motion_range(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_F200_FILTER_OPTION:        ivcam::set_filter_option(get_device(), static_cast<uint8_t>(values[i])); break;
            case RS_OPTION_F200_CONFIDENCE_THRESHOLD: ivcam::set_confidence_threshold(get_device(), static_cast<uint8_t>(values[i])); break;

            default: 
                base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        //Set common options
        if (!base_opt.empty())
            rs_device_base::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void iv_camera::get_options(const rs_option options[], size_t count, double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<size_t>     base_opt_index;
        std::vector<double>     base_opt_val;

        for (size_t i = 0; i < count; ++i)
        {
            LOG_INFO("Reading option " << options[i]);

            if (uvc::is_pu_control(options[i]))
                throw std::logic_error(to_string() << __FUNCTION__ << " Option " << options[i] << " must be processed by a concrete class");

            uint8_t val = 0;
            switch (options[i])
            {
            case RS_OPTION_F200_LASER_POWER:          ivcam::get_laser_power(get_device(), val); values[i] = val; break;
            case RS_OPTION_F200_ACCURACY:             ivcam::get_accuracy(get_device(), val); values[i] = val; break;
            case RS_OPTION_F200_MOTION_RANGE:         ivcam::get_motion_range(get_device(), val); values[i] = val; break;
            case RS_OPTION_F200_FILTER_OPTION:        ivcam::get_filter_option(get_device(), val); values[i] = val; break;
            case RS_OPTION_F200_CONFIDENCE_THRESHOLD: ivcam::get_confidence_threshold(get_device(), val); values[i] = val; break;

            default: 
                base_opt.push_back(options[i]); base_opt_index.push_back(i);
               /* LOG_WARNING("Cannot get " << options[i] << " on " << get_name());
                throw std::logic_error("Option unsupported");*/
                break;
            }
        }
        //Retrieve the common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            rs_device_base::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (auto i : base_opt_index)
            values[i] = base_opt_val[i];
    }

    // TODO: This may need to be modified for thread safety
    class rolling_timestamp_reader : public frame_timestamp_reader
    {
        bool started;
        int64_t total;
        int last_timestamp;
        int64_t counter = 0;
    public:
        rolling_timestamp_reader() : started(), total() {}

        bool validate_frame(const subdevice_mode & mode, const void * frame) override
        {
            // Validate that at least one byte of the image is nonzero
            for (const uint8_t * it = (const uint8_t *)frame, *end = it + mode.pf.get_image_size(mode.native_dims.x, mode.native_dims.y); it != end; ++it)
            {
                if (*it)
                {
                    return true;
                }
            }

            // F200 and SR300 can sometimes produce empty frames shortly after starting, ignore them
            LOG_INFO("Subdevice " << mode.subdevice << " produced empty frame");
            return false;
        }

        double get_frame_timestamp(const subdevice_mode & /*mode*/, const void * frame, double /*actual_fps*/) override
        {
            // Timestamps are encoded within the first 32 bits of the image
            int rolling_timestamp = *reinterpret_cast<const int32_t *>(frame);

            if (!started)
            {
                last_timestamp = rolling_timestamp;
                started = true;
            }

            const int delta = rolling_timestamp - last_timestamp; // NOTE: Relies on undefined behavior: signed int wraparound
            last_timestamp = rolling_timestamp;
            total += delta;
            const int timestamp = static_cast<int>(total / 100000);
            return timestamp;
        }
        unsigned long long get_frame_counter(const subdevice_mode & /*mode*/, const void * /*frame*/) override
        {
            return ++counter;
        }
    };

    std::vector<std::shared_ptr<frame_timestamp_reader>> iv_camera::create_frame_timestamp_readers() const
    {
        auto the_reader = std::make_shared<rolling_timestamp_reader>(); // single shared timestamp reader for all subdevices
        return { the_reader, the_reader }; // clone the reference for color and depth
    }

} // namespace rsimpl::f200
