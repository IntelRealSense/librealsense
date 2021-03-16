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

#define FW_UPDATE_INTERFACE_NUMBER 0
#define DEFAULT_TIMEOUT 100

namespace librealsense
{
    bool is_l500_recovery(platform::rs_usb_device usb, bool &is_l500_device)
    {
        dfu_fw_status_payload payload;

        auto messenger = usb->open(FW_UPDATE_INTERFACE_NUMBER);
        if (!messenger)
        {
            LOG_ERROR("Unable to open USB device: " << usb->get_info().id);
            return false;
        }

        // Step 1 - Detach device - mandatory before communicating with the DFU
        {
            auto timeout = 1000;
            uint32_t transferred = 0;
            auto res = messenger->control_transfer(0x21, RS2_DFU_DETACH, timeout, 0, NULL, 0, transferred, timeout);
            if (res != platform::RS2_USB_STATUS_SUCCESS)
                LOG_WARNING("DFU - failed to detach device: " << platform::usb_status_to_string.at(res));
        }

        // Step 2 - Verify device entered DFU idle state and ready to communicate with the host
        {
            uint8_t state = RS2_DFU_STATE_DFU_ERROR;
            uint32_t transferred = 0;
            auto res = messenger->control_transfer(0xa1, RS2_DFU_GET_STATE, 0, 0, &state, 1, transferred, DEFAULT_TIMEOUT);
            if (res != platform::RS2_USB_STATUS_SUCCESS)
            {
                LOG_ERROR("Failed to get DFU device state! status is : " << platform::usb_status_to_string.at(res) << ", device ID is : " << usb->get_info().id);
                return false;
            }

            if (state != RS2_DFU_STATE_DFU_IDLE)
            {
                LOG_ERROR("DFU is not on detach state! state is : " << state << ", device ID is : " << usb->get_info().id);
                return false;
            }
        }
        // Step 3 - Query DFU information
        {
            uint32_t transferred = 0;
            auto sts = messenger->control_transfer(0xa1, RS2_DFU_UPLOAD, 0, 0, reinterpret_cast<uint8_t*>(&payload), sizeof(payload), transferred, DEFAULT_TIMEOUT);
            if (sts != platform::RS2_USB_STATUS_SUCCESS)
            {
                LOG_ERROR("Failed to read info from DFU device! , status is :" << platform::usb_status_to_string.at(sts));
                return false;
            }
        }

        is_l500_device = ((payload.dfu_version & l500_update_device::DFU_VERSION_MASK) == l500_update_device::DFU_VERSION_VALUE);
        return true;
    }

    int get_product_line(const platform::usb_device_info &usb_info)
    {
        if( SR300_RECOVERY == usb_info.pid && platform::RS2_USB_CLASS_VENDOR_SPECIFIC == usb_info.cls )
            return RS2_PRODUCT_LINE_SR300;
        if( ds::RS_RECOVERY_PID == usb_info.pid )
            return RS2_PRODUCT_LINE_D400;
        if( L500_RECOVERY_PID == usb_info.pid || L535_RECOVERY_PID == usb_info.pid)
            return RS2_PRODUCT_LINE_L500;
        if( ds::RS_USB2_RECOVERY_PID == usb_info.pid || L500_USB2_RECOVERY_PID_OLD == usb_info.pid )
        {
            bool is_l500 = false;
            {
                auto usb = platform::usb_enumerator::create_usb_device(usb_info);
                if (usb)
                {
                    // This function is a SW patch that overcome a problem found on FW of old L515 devices.
                    // The L515 units declare DS-5 PID while on DFU and connected as USB2
                    // Using the DFU version we identify L515 VS DS-5 devices.
                    // This issue is fixed on newer unit 
                    if (!is_l500_recovery(usb, is_l500))
                        return 0;
                }
            }

            if (is_l500)
                return RS2_PRODUCT_LINE_L500;
            else
                return RS2_PRODUCT_LINE_D400;
        }
        return 0;
    }


    std::vector<std::shared_ptr<device_info>> fw_update_info::pick_recovery_devices(
        std::shared_ptr<context> ctx,
        const std::vector<platform::usb_device_info>& usb_devices, int mask)
    {
        std::vector<std::shared_ptr<device_info>> list;
        for (auto&& usb : usb_devices)
        {
            auto pl = get_product_line(usb);
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
                if (ds::RS_RECOVERY_PID == info.pid)
                    return std::make_shared<ds_update_device>(ctx, register_device_notifications, usb);                   
                if (SR300_RECOVERY == info.pid)
                    return std::make_shared<sr300_update_device>(ctx, register_device_notifications, usb);
                if (L500_RECOVERY_PID == info.pid || L535_RECOVERY_PID == info.pid)
                    return std::make_shared<l500_update_device>(ctx, register_device_notifications, usb);
                if (ds::RS_USB2_RECOVERY_PID == info.pid || L500_USB2_RECOVERY_PID_OLD == info.pid)
                {
                    bool dev_is_l500 = false;
                    if (is_l500_recovery(usb, dev_is_l500))
                    {
                        if (dev_is_l500)
                            return std::make_shared<l500_update_device>(ctx, register_device_notifications, usb);
                        else
                            return std::make_shared<ds_update_device>(ctx, register_device_notifications, usb);
                    }
                }
            }
        }
        throw std::runtime_error(to_string() << "Failed to create FW update device, device id: " << _dfu.id);
    }
}
