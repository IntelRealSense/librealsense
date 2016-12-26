// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5.h"

namespace rsimpl
{
    std::shared_ptr<rsimpl::device> ds5_info::create(const uvc::backend& backend) const
    {
        return std::make_shared<ds5_camera>(backend, _depth, _hwm, _hid);
    }

    // DS5 has 2 pins of video streaming
    ds5_info::ds5_info(std::vector<uvc::uvc_device_info> depth, uvc::usb_device_info hwm, std::vector<uvc::hid_device_info> hid)
        : _depth(std::move(depth)),
          _hwm(std::move(hwm)),
          _hid(std::move(hid))
    {
    }

    std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb,
        std::vector<uvc::hid_device_info>& hid)
    {
        std::vector<uvc::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto valid_pid = filter_by_product(uvc, rs4xx_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), hid);
        for (auto& group : group_devices)
        {
            auto& devices = group.first;
            auto& hids = group.second;
            if (!devices.empty() &&
                mi_present(devices, 0))
            {
                uvc::usb_device_info hwm;

                if (ds::try_fetch_usb_device(usb, devices.front(), hwm))
                {
                    auto info = std::make_shared<ds5_info>(devices, hwm, hids);
                    chosen.insert(chosen.end(), devices.begin(), devices.end());
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

    std::vector<uint8_t> ds5_camera::send_receive_raw_data(const std::vector<uint8_t>& input)
    {
        return _hw_monitor->send(input);
    }

    rs_intrinsics ds5_camera::get_intrinsics(int subdevice, stream_profile profile) const
    {
        if (subdevice >= get_endpoints_count()) throw std::runtime_error("Requested subdevice is unsupported.");

        if (subdevice == _depth_device_idx)
        {
            return get_intrinsic_by_resolution(
                *_coefficients_table_raw,
                ds::calibration_table_id::coefficients_table_id,
                profile.width, profile.height);
        }

        throw std::runtime_error("Not Implemented");
    }

    std::vector<uint8_t> ds5_camera::get_raw_calibration_table(ds::calibration_table_id table_id) const
    {
        command cmd(ds::GETINTCAL, table_id);
        return _hw_monitor->send(cmd);
    }
}
