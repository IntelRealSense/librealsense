// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "usb/usb-enumerator.h"
#include "libusb/device-libusb.h"
#include "types.h"
#include "ds5/ds5-private.h"
#include "l500/l500-private.h"
#include "ivcam/sr300.h"

#ifdef USE_SYSTEM_LIBUSB
    #include <libusb.h>
#else
    #include "libusb/libusb.h"
#endif

namespace librealsense
{
    namespace platform
    {
        bool usb_enumerator::is_device_connected(const std::shared_ptr<usb_device> device)
        {
            return false;
        }

        std::vector<std::shared_ptr<usb_device>> usb_enumerator::query_devices()
        {
            std::vector<std::shared_ptr<usb_device>> rv;
            ssize_t count = 0;
//            libusb_context* ctx;
            libusb_device **list = NULL;            

//            libusb_init(&ctx);
//            count = libusb_get_device_list(ctx, &list);
            libusb_init(NULL);
            count = libusb_get_device_list(NULL, &list);

            for (ssize_t idx = 0; idx < count; ++idx)
            {
                libusb_device *device = list[idx];
                libusb_device_descriptor desc = { 0 };

                auto rc = libusb_get_device_descriptor(device, &desc);
                if (desc.idVendor != 0x8086)
                    continue;
                if ((std::find(ds::rs400_sku_recovery_pid.begin(), ds::rs400_sku_recovery_pid.end(), desc.idProduct) == ds::rs400_sku_recovery_pid.end()) &&
                    (std::find(ds::rs400_sku_pid.begin(), ds::rs400_sku_pid.end(), desc.idProduct) == ds::rs400_sku_pid.end()) &&
                    (desc.idProduct != L500_PID) && (desc.idProduct != SR300_PID) && (desc.idProduct != SR300v2_PID))
                    continue;
                rv.push_back(std::make_shared<usb_device_libusb>(device, desc));
            }
            libusb_free_device_list(list, 0);
            return rv;
        }
    }
}
