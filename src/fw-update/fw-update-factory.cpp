// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "fw-update-factory.h"
#include "fw-update-device.h"
#include "usb/usb-enumerator.h"
#include "ds5/ds5-private.h"
#include "ds5/ds5-fw-update-device.h"
#include "ivcam/sr300.h"
#include "ivcam/sr300-fw-update-device.h"
#include "l500/l500-private.h"
#include "l500/l500-fw-update-device.h"

namespace librealsense
{
    int get_product_line(uint16_t pid, platform::usb_class cls)
    {
        if (SR300_RECOVERY == pid && platform::RS2_USB_CLASS_VENDOR_SPECIFIC == cls)
            return RS2_PRODUCT_LINE_SR300;
        if(ds::RS_RECOVERY_PID == pid || ds::RS_USB2_RECOVERY_PID == pid)
            return RS2_PRODUCT_LINE_D400;
        if (L500_RECOVERY_PID == pid)
            return RS2_PRODUCT_LINE_L500;
        return 0;
    }

    std::vector<std::shared_ptr<device_info>> fw_update_info::pick_recovery_devices(
        std::shared_ptr<context> ctx,
        const std::vector<platform::usb_device_info>& usb_devices, int mask)
    {
        std::vector<std::shared_ptr<device_info>> list;
        for (auto&& usb : usb_devices)
        {
            auto pl = get_product_line(usb.pid, usb.cls);
            if(pl & mask)
                list.push_back(std::make_shared<fw_update_info>(ctx, usb));
        }
        return list;
    }

    std::shared_ptr<device_interface> fw_update_info::create(std::shared_ptr<context> ctx, bool register_device_notifications) const
    {
        auto devices = platform::usb_enumerator::query_devices_info();
        for (auto&& info : devices)
        {
            if (info.id == _dfu.id)
            {
                auto usb = platform::usb_enumerator::create_usb_device(info);
                if (!usb)
                    continue;
                if (ds::RS_RECOVERY_PID == info.pid || ds::RS_USB2_RECOVERY_PID == info.pid)
                    return std::make_shared<ds_update_device>(ctx, register_device_notifications, usb);
                if (SR300_RECOVERY == info.pid)
                    return std::make_shared<sr300_update_device>(ctx, register_device_notifications, usb);
                if (L500_RECOVERY_PID == info.pid)
                    return std::make_shared<l500_update_device>(ctx, register_device_notifications, usb);
            }
        }
        throw std::runtime_error(to_string() << "Failed to create FW update device, device id: " << _dfu.id);
    }
}
