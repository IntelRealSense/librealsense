//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
//
//#pragma once
//#ifndef LIBREALSENSE_DS5_H
//#define LIBREALSENSE_DS5_H
//
//#include "device.h"
//
//namespace rsimpl
//{
//    // This is a base class for the various SKUs of the DS5 camera
//    class ds5_camera : public rs_device_base
//    {
//
//    public:
//        ds5_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
//        ~ds5_camera() {};
//
//        void set_options(const rs_option options[], size_t count, const double values[]) override;
//        void get_options(const rs_option options[], size_t count, double values[]) override;
//
//        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
//        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
//        std::vector<std::shared_ptr<rsimpl::frame_timestamp_reader>> create_frame_timestamp_readers() const override;
//
//    };
//}
//
//#endif


// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS5_H
#define LIBREALSENSE_DS5_H

#include <vector>

#include "device.h"
#include "context.h"
#include "backend.h"
#include "ds5-private.h"
#include "hw-monitor.h"
#include "image.h"

namespace rsimpl
{
    class ds5_camera;

    class ds5_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        std::shared_ptr<device_info> clone() const override
        {
            return std::make_shared<ds5_info>(*this);
        }

        ds5_info(uvc::uvc_device_info depth,
            uvc::usb_device_info hwm);

    private:
        uvc::uvc_device_info _depth;
        uvc::usb_device_info _hwm;
    };

    std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb);

    class ds5_camera final : public device
    {
    public:
        ds5_camera(const uvc::backend& backend,
            const uvc::uvc_device_info& depth,
            const uvc::usb_device_info& hwm_device)
            : _hw_monitor(backend.create_usb_device(hwm_device))
        {
            using namespace ds;

            // create uvc-endpoint from backend uvc-device
            auto depth_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(depth));
            depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera
            depth_ep->register_pixel_format(pf_z16); // Depth
            depth_ep->register_pixel_format(pf_y8); // Left Only - Luminance
            depth_ep->register_pixel_format(pf_yuyvl); // Left Only
            depth_ep->register_pixel_format(pf_y12i); // L+R - Calibration not rectified

            auto fw_version = _hw_monitor.get_firmware_version_string(GVD, gvd_fw_version_offset);
            auto serial = _hw_monitor.get_module_serial_string(GVD);
            auto location = depth_ep->invoke_powered([](uvc::uvc_device& dev)
            {
                return dev.get_device_location();
            });

            register_device("Intel RealSense DS5", fw_version, serial, "");

            // map subdevice to endpoint
            assign_endpoint(RS_SUBDEVICE_DEPTH, depth_ep);

            register_pu(RS_SUBDEVICE_DEPTH, RS_OPTION_GAIN);
            register_depth_xu<uint8_t>(RS_OPTION_LASER_POWER, DS5_DEPTH_LASER_POWER,
                "Power of the DS5 projector, with 0 meaning projector off");

            register_depth_xu<uint16_t>(RS_OPTION_EXPOSURE, DS5_EXPOSURE,
                "DS5 Exposure");
        }


    private:
        hw_monitor _hw_monitor;

        template<class T>
        void register_depth_xu(rs_option opt, uint8_t id, std::string desc)
        {
            register_option(opt, RS_SUBDEVICE_DEPTH,
                std::make_shared<uvc_xu_option<T>>(
                    get_uvc_endpoint(RS_SUBDEVICE_DEPTH),
                    ds::depth_xu, id, std::move(desc)));
        }
    };
}
#endif
