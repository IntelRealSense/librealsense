// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "ivcam-private.h"


namespace librealsense
{
    namespace ivcam
    {
        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                auto pid = it->pid;
                if (it->unique_id == info.unique_id)
                {

                    result = *it;
                    if (result.mi == 4)
                    {
                        devices.erase(it);
                        return true;
                    }
                }
            }
            return false;
        }

        bool try_fetch_usb_depth_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                auto pid = it->pid;
                if (it->unique_id == info.unique_id)
                {

                    result = *it;
                    if ((pid == 0x0B48 &&result.mi == 4) || (pid == 0x0aa3 && result.mi == 2))
                    {
                        devices.erase(it);
                        return true;
                    }
                }
            }
            return false;
        }
    } // namespace librealsense::ivcam
} // namespace librealsense
