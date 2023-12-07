// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "fw-update-factory.h"
#include "fw-update-device.h"
#include "usb/usb-enumerator.h"
#include "ds/d400/d400-private.h"
#include "ds/d500/d500-private.h"
#include "ds/d400/d400-fw-update-device.h"
#include "ds/d500/d500-fw-update-device.h"

#include <rsutils/string/from.h>


#define FW_UPDATE_INTERFACE_NUMBER 0
#define DEFAULT_TIMEOUT 100

namespace librealsense
{
    int get_product_line(const platform::usb_device_info &usb_info)
    {
        if( ds::RS_D400_RECOVERY_PID == usb_info.pid )
            return RS2_PRODUCT_LINE_D400;
        if( ds::RS_D400_USB2_RECOVERY_PID == usb_info.pid )
            return RS2_PRODUCT_LINE_D400;
        if( ds::D555E_RECOVERY_PID == usb_info.pid )
            return RS2_PRODUCT_LINE_D500;
        return 0;
    }


    std::vector< std::shared_ptr< fw_update_info > > fw_update_info::pick_recovery_devices(
        std::shared_ptr< context > ctx, const std::vector< platform::usb_device_info > & usb_devices, int mask )
    {
        std::vector< std::shared_ptr< fw_update_info > > list;
        for (auto&& usb : usb_devices)
        {
            auto pl = get_product_line(usb);
            if(pl & mask)
                list.push_back(std::make_shared<fw_update_info>(ctx, usb));
        }
        return list;
    }


    std::shared_ptr<device_interface> fw_update_info::create_device()
    {
        auto devices = platform::usb_enumerator::query_devices_info();
        auto const & dfu_id = get_group().usb_devices.front().id;
        for (auto&& info : devices)
        {
            if( info.id == dfu_id )
            {
                auto usb = platform::usb_enumerator::create_usb_device(info);
                if (!usb)
                    continue;
                switch( info.pid )
                {
                case ds::RS_D400_RECOVERY_PID:
                case ds::RS_D400_USB2_RECOVERY_PID:
                    return std::make_shared< ds_d400_update_device >( shared_from_this(), usb );
                case ds::D555E_RECOVERY_PID:
                    return std::make_shared< ds_d500_update_device >( shared_from_this(), usb );
                default:
                    // Do nothing
                    break;
                }
            }
        }
        throw std::runtime_error( rsutils::string::from()
                                  << "Failed to create FW update device, device id: " << dfu_id );
    }
}
