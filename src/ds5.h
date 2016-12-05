// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

#include "device.h"
#include "context.h"
#include "backend.h"
#include "ds5-private.h"
#include "hw-monitor.h"
#include "image.h"

namespace rsimpl
{
    static const std::vector<std::uint16_t> rs4xx_sku_pid = { ds::RS400P_PID, ds::RS410A_PID, ds::RS420R_PID, ds::RS430C_PID, ds::RS450T_PID };

    class ds5_camera;

    class ds5_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        std::shared_ptr<device_info> clone() const override
        {
            return std::make_shared<ds5_info>(*this);
        }

        ds5_info(std::vector<uvc::uvc_device_info> depth,
            uvc::usb_device_info hwm,
            std::vector<uvc::hid_device_info> hid);

    private:
        std::vector<uvc::uvc_device_info> _depth;
        uvc::usb_device_info _hwm;
        std::vector<uvc::hid_device_info> _hid;
    };

    std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb,
        std::vector<uvc::hid_device_info>& hid);

    class ds5_camera final : public device
    {
    public:
        class emitter_option : public uvc_xu_option<uint8_t>
        {
        public:
            const char* get_value_description(float val) const override
            {
                switch (static_cast<int>(val))
                {
                    case 0:
                    {
                        return "Off";
                    }
                    case 1:
                    {
                        return "On";
                    }
                    case 2:
                    {
                        return "Auto";
                    }
                    default:
                        throw std::runtime_error("value not found");
                }
            }

            explicit emitter_option(uvc_endpoint& ep) 
                : uvc_xu_option(ep, ds::depth_xu, ds::DS5_DEPTH_EMITTER_ENABLED,
                                "Power of the DS5 projector, 0 meaning projector off, 1 meaning projector off, 2 meaning projector in auto mode")
            {}
        };

        std::shared_ptr<hid_endpoint> create_hid_device(const uvc::backend& backend,
                                                        const std::vector<uvc::hid_device_info>& all_hid_infos)
        {
            using namespace ds;
            // TODO: implement multiple hid devices
            auto hid_ep = std::make_shared<hid_endpoint>(backend.create_hid_device(all_hid_infos[0]));
            return hid_ep;

        }

        std::shared_ptr<uvc_endpoint> create_depth_device(const uvc::backend& backend,
                                                          const std::vector<uvc::uvc_device_info>& all_device_infos)
        {
            using namespace ds;

            std::vector<std::shared_ptr<uvc::uvc_device>> depth_devices;
            for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
                depth_devices.push_back(backend.create_uvc_device(info));

            auto depth_ep = std::make_shared<uvc_endpoint>(std::make_shared<uvc::multi_pins_uvc_device>(depth_devices));
            depth_ep->register_xu(depth_xu); // make sure the XU is initialized everytime we power the camera
            depth_ep->register_pixel_format(pf_z16); // Depth
            depth_ep->register_pixel_format(pf_y8); // Left Only - Luminance
            depth_ep->register_pixel_format(pf_yuyv); // Left Only
            depth_ep->register_pixel_format(pf_uyvyl); // Color from Depth
            depth_ep->register_pixel_format(pf_y8i); // L+R ; TODO: allow only in Advanced mode
            depth_ep->register_pixel_format(pf_y12i); // L+R - Calibration not rectified ; TODO: allow only in Advanced mode


            depth_ep->register_pu(RS_OPTION_GAIN);
            depth_ep->register_pu(RS_OPTION_ENABLE_AUTO_EXPOSURE);

            depth_ep->register_option(RS_OPTION_EXPOSURE,
                std::make_shared<uvc_xu_option<uint16_t>>(*depth_ep,
                    depth_xu,
                    DS5_EXPOSURE, "DS5 Exposure")); // TODO: Update description

            // TODO: These if conditions will be implemented as inheritance classes
            auto pid = all_device_infos.front().pid;
            if (pid == RS410A_PID || pid == RS450T_PID)
            {
                depth_ep->register_option(RS_OPTION_EMITTER_ENABLED, std::make_shared<emitter_option>(*depth_ep));

                depth_ep->register_option(RS_OPTION_LASER_POWER,
                    std::make_shared<uvc_xu_option<uint16_t>>(*depth_ep,
                        depth_xu,
                        DS5_LASER_POWER, "Manual laser power. applicable only in on mode"));
            }


            return depth_ep;
        }

        uvc_endpoint& get_depth_endpoint() { return static_cast<uvc_endpoint&>(get_endpoint(_depth_device_idx)); }
        hid_endpoint& get_hid_endpoint() { return static_cast<hid_endpoint&>(get_endpoint(_hid_device_idx)); }

        ds5_camera(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const uvc::usb_device_info& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : _hw_monitor(backend.create_usb_device(hwm_device)),
              _depth_device_idx(add_endpoint(create_depth_device(backend, dev_info), "Stereo Module")),
              _hid_device_idx(add_endpoint(create_hid_device(backend, hid_info), "Hid"))
        {
            using namespace ds;
            // create uvc-endpoint from backend uvc-device
            std::shared_ptr<uvc::uvc_device> fisheye_dev;
            std::vector<std::shared_ptr<uvc::uvc_device>> devices;
            for(auto& element : dev_info)
            {
                if (element.mi == 0) // mi 0 is relate to DS5 device
                    devices.push_back(backend.create_uvc_device(element));
                else if (element.pid == RS450T_PID && element.mi == 3) // mi 3 is relate to Fisheye device
                    fisheye_dev = backend.create_uvc_device(element);
            }

            auto fw_version = _hw_monitor.get_firmware_version_string(GVD, gvd_fw_version_offset);
            auto serial = _hw_monitor.get_module_serial_string(GVD, 48);
            auto location = get_depth_endpoint().invoke_powered([](uvc::uvc_device& dev)
            {
                return dev.get_device_location();
            });

            register_device("Intel RealSense DS5", fw_version, serial, "");


            _coefficients_table_raw = [this]() { return get_raw_calibration_table(coefficients_table_id); };

            // TODO: These if conditions will be implemented as inheritance classes
            auto pid = dev_info.front().pid;

            if (pid == RS450T_PID)
            {
                auto fisheye_infos = filter_by_mi(dev_info, 3);
                if (fisheye_infos.size() != 1)
                    throw std::runtime_error("RS450 model is expected to include a single fish-eye device!");

                auto fisheye_ep = std::make_shared<uvc_endpoint>(backend.create_uvc_device(fisheye_infos.front()));
                fisheye_ep->register_xu(fisheye_xu); // make sure the XU is initialized everytime we power the camera
                fisheye_ep->register_pixel_format(pf_raw8);
                add_endpoint(fisheye_ep, "Fisheye Camera");
                fisheye_ep->register_pu(RS_OPTION_GAIN);

                fisheye_ep->register_option(RS_OPTION_EXPOSURE,
                    std::make_shared<uvc_xu_option<uint16_t>>(*fisheye_ep,
                        fisheye_xu,
                        FISHEYE_EXPOSURE, "Fisheye Exposure")); // TODO: Update description
            }
        }

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override;
        rs_intrinsics get_intrinsics(int subdevice, stream_request profile) const override;

    private:
        hw_monitor _hw_monitor;
        
        const uint8_t _depth_device_idx;
        const uint8_t _hid_device_idx;

        lazy<std::vector<uint8_t>> _coefficients_table_raw;

        std::vector<uint8_t> get_raw_calibration_table(ds::calibration_table_id table_id) const;

    };
}
