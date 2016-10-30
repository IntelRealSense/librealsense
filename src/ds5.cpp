//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
//
//#include <algorithm>
//
//#include "image.h"
//#include "ds-private.h"
//#include "ds5.h"
//#include "ds5-private.h"
//
//namespace rsimpl
//{
//    ds5_camera::ds5_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) :
//        rs_device_base(device, info)
//    {
//    }
//
//    void ds5_camera::set_options(const rs_option options[], size_t count, const double values[])
//    {
//        for (size_t i = 0; i<count; ++i)
//        {
//            if(uvc::is_pu_control(options[i]))
//            {
//                if (supports(RS_CAPABILITIES_COLOR))
//                    uvc::set_pu_control_with_retry(get_device(), 2, options[i], static_cast<int>(values[i]));
//                else
//                    throw std::logic_error(to_string() << get_name() << " has no CCD sensor, the following is not supported: " << options[i]);
//            }
//
//            switch (options[i])
//            {
//            case RS_OPTION_RS400_LASER_POWER:    ds5::set_laser_power(get_device(), static_cast<uint8_t>(values[i]));break;
//            case RS_OPTION_R200_LR_EXPOSURE:    ds5::set_lr_exposure(get_device(), static_cast<uint16_t>(values[i])); break;
//
//            default:
//                LOG_WARNING("Set " << options[i] << " for " << get_name() << " is not supported");
//                throw std::logic_error("Option unsupported");
//                break;
//            }
//        }
//    }
//
//    void ds5_camera::get_options(const rs_option options[], size_t count, double values[])
//    {
//        for (size_t i = 0; i < count; ++i)
//        {
//            LOG_INFO("Reading option " << options[i]);
//
//            if (uvc::is_pu_control(options[i]))
//                throw std::logic_error(to_string() << __FUNCTION__ << " Option " << options[i] << " must be processed by a concrete class");
//
//            uint8_t val = 0;
//            switch (options[i])
//            {
//                case RS_OPTION_RS400_LASER_POWER:           ds5::get_laser_power(get_device(), val); values[i] = val; break;
//
//                default:
//                    LOG_WARNING("Get " << options[i] << " for " << get_name() << " is not supported");
//                    throw std::logic_error("Option unsupported");
//                    break;
//            }
//        }
//    }
//
//    void ds5_camera::on_before_start(const std::vector<subdevice_mode_selection> & /*selected_modes*/)
//    {
//
//    }
//
//    rs_stream ds5_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
//    {
//        // DS5 may have a different behaviour here. This is a placeholder
//        int fps[RS_STREAM_NATIVE_COUNT] = {}, max_fps = 0;
//        for (const auto & m : selected_modes)
//        {
//            for (const auto & output : m.get_outputs())
//            {
//                fps[output.first] = m.mode.fps;
//                max_fps = std::max(max_fps, m.mode.fps);
//            }
//        }
//
//        // Prefer to sync on depth or infrared, but select the stream running at the fastest framerate
//        for (auto s : { RS_STREAM_DEPTH, RS_STREAM_INFRARED2, RS_STREAM_INFRARED, RS_STREAM_COLOR })
//        {
//            if (fps[s] == max_fps) return s;
//        }
//        return RS_STREAM_DEPTH;
//    }
//
//    // Sequential frame counter, Using host-arrival timestamp
//    class ds5_timestamp_reader : public frame_timestamp_reader
//    {
//        mutable int serial_frame_number;
//        std::chrono::system_clock::time_point initial_ts;
//
//    public:
//        ds5_timestamp_reader() : serial_frame_number(0), initial_ts(std::chrono::system_clock::now()) {}
//
//        bool validate_frame(const subdevice_mode & mode, const void * frame) const override
//        {
//            // Validate that at least one byte of the image is nonzero
//            for (const uint8_t * it = (const uint8_t *)frame, *end = it + mode.pf.get_image_size(mode.native_dims.x, mode.native_dims.y); it != end; ++it)
//            {
//                if (*it)
//                {
//                    return true;
//                }
//            }
//
//            // DS5 can sometimes produce empty frames shortly after starting, ignore them
//            LOG_INFO("Subdevice " << mode.subdevice << " produced empty frame");
//            return false;
//        }
//
//        double get_frame_timestamp(const subdevice_mode &, const void *) override
//        {
//            return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - initial_ts).count()*0.000001;
//        }
//        unsigned long long get_frame_counter(const subdevice_mode &, const void *) const override
//        {
//            return serial_frame_number++;
//        }
//    };
//
//    std::vector<std::shared_ptr<rsimpl::frame_timestamp_reader>>  ds5_camera::create_frame_timestamp_readers() const
//    {
//        return { 
//            std::make_shared<ds5_timestamp_reader>(),
//            std::make_shared<ds5_timestamp_reader>(),
//            std::make_shared<ds5_timestamp_reader>(),
//            std::make_shared<ds5_timestamp_reader>(),
//        };
//    }
//
//} // namespace rsimpl::ds5

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5.h"

namespace rsimpl
{
    std::shared_ptr<rsimpl::device> ds5_info::create(const uvc::backend& backend) const
    {
        return std::make_shared<ds5_camera>(backend, _depth, _hwm);
    }

    ds5_info::ds5_info(std::vector<uvc::uvc_device_info> depth, uvc::usb_device_info hwm)
        : _depth(std::move(depth)),
        _hwm(std::move(hwm))
    {
    }

    std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb)
    {
        std::vector<uvc::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto right_pid = filter_by_product(uvc, DS5_PID);
        auto group_devices = group_by_unique_id(right_pid);
        for (auto& group : group_devices)
        {
            if (group.size() != 0 &&
                mi_present(group, 0))
            {
                uvc::usb_device_info hwm;

                if (ds::try_fetch_usb_device(usb, group[0], hwm))
                {
                    auto info = std::make_shared<ds5_info>(group, hwm);
                    chosen.insert(chosen.end(), group.begin(), group.end());
                    results.push_back(info);
                }
                else
                {
                    //TODO: Log
                }
            }
            else
            {
                // TODO: LOG
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }


    // "Get Version and Date"
    // Reference: Commands.xml in IVCAM_DLL
}
