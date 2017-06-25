// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-factory.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"

namespace rsimpl2
{
    // PSR
    class rs400_device : public ds5_rolling_shutter
    {
    public:
        rs400_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_rolling_shutter(backend, dev_info, hwm_device, hid_info) {}
    };

    // ASR
    class rs410_device : public ds5_rolling_shutter,
                         public ds5_active
    {
    public:
        rs410_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_rolling_shutter(backend, dev_info, hwm_device, hid_info),
              ds5_active(backend, dev_info, hwm_device, hid_info) {}
    };

    // ASRC
    class rs415_device : public ds5_rolling_shutter,
                         public ds5_active,
                         public ds5_color
    {
    public:
        rs415_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_rolling_shutter(backend, dev_info, hwm_device, hid_info),
              ds5_active(backend, dev_info, hwm_device, hid_info),
              ds5_color(backend, dev_info, hwm_device, hid_info) {}
    };

    // PWGT
    class rs420_mm_device : public ds5_motion
    {
    public:
        rs420_mm_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_motion(backend, dev_info, hwm_device, hid_info) {}
    };

    // AWG
    class rs430_device : public ds5_active
    {
    public:
        rs430_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_active(backend, dev_info, hwm_device, hid_info) {}
    };

    // AWGT
    class rs430_mm_device : public ds5_active,
                            public ds5_motion
    {
    public:
        rs430_mm_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_active(backend, dev_info, hwm_device, hid_info),
              ds5_motion(backend, dev_info, hwm_device, hid_info) {}
    };

    // AWGC
    class rs435_device : public ds5_active,
                         public ds5_color
    {
    public:
        rs435_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_active(backend, dev_info, hwm_device, hid_info),
              ds5_color(backend, dev_info, hwm_device, hid_info) {}
    };

    // AWGCT
    class rs430_rgb_mm_device : public ds5_active,
                                public ds5_color,
                                public ds5_motion
    {
    public:
        rs430_rgb_mm_device(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info)
            : ds5_device(backend, dev_info, hwm_device, hid_info),
              ds5_active(backend, dev_info, hwm_device, hid_info),
              ds5_color(backend, dev_info, hwm_device, hid_info),
              ds5_motion(backend, dev_info, hwm_device, hid_info) {}
    };

    std::shared_ptr<rsimpl2::device> ds5_info::create(const uvc::backend& backend) const
    {
        using namespace ds;

        if (_depth.size() == 0) throw std::runtime_error("Depth Camera not found!");
        auto pid = _depth.front().pid;
        switch(pid)
        {
        case RS400_PID:
            return std::make_shared<rs400_device>(backend, _depth, _hwm, _hid);
        case RS410_PID:
            return std::make_shared<rs410_device>(backend, _depth, _hwm, _hid);
        case RS415_PID:
            return std::make_shared<rs415_device>(backend, _depth, _hwm, _hid);
        case RS420_PID:
            return std::make_shared<ds5_device>(backend, _depth, _hwm, _hid);
        case RS420_MM_PID:
            return std::make_shared<rs420_mm_device>(backend, _depth, _hwm, _hid);
        case RS430_PID:
            return std::make_shared<rs430_device>(backend, _depth, _hwm, _hid);
        case RS430_MM_PID:
            return std::make_shared<rs430_mm_device>(backend, _depth, _hwm, _hid);
        case RS430_MM_RGB_PID:
            return std::make_shared<rs430_rgb_mm_device>(backend, _depth, _hwm, _hid);
        case RS435_RGB_PID:
            return std::make_shared<rs435_device>(backend, _depth, _hwm, _hid);
        default:
            throw std::runtime_error("Unsupported RS400 model!");
        }
    }

    std::vector<std::shared_ptr<device_info>> ds5_info::pick_ds5_devices(
        std::shared_ptr<uvc::backend> backend,
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb,
        std::vector<uvc::hid_device_info>& hid)
    {
        std::vector<uvc::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto valid_pid = filter_by_product(uvc, ds::rs4xx_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), hid);
        for (auto& group : group_devices)
        {
            auto& devices = group.first;
            auto& hids = group.second;

            if((group.first[0].pid == ds::RS430_MM_PID || group.first[0].pid == ds::RS420_MM_PID) &&  hids.size()==0)
                continue;

            if (!devices.empty() &&
                mi_present(devices, 0))
            {
                uvc::usb_device_info hwm;

                std::vector<uvc::usb_device_info> hwm_devices;
                if (ds::try_fetch_usb_device(usb, devices.front(), hwm))
                {
                    hwm_devices.push_back(hwm);
                }
                else
                {
                    LOG_DEBUG("try_fetch_usb_device(...) failed.");
                }

                auto info = std::make_shared<ds5_info>(backend, devices, hwm_devices, hids);
                chosen.insert(chosen.end(), devices.begin(), devices.end());
                results.push_back(info);

            }
            else
            {
                LOG_WARNING("DS5 group_devices is empty.");
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }
}
