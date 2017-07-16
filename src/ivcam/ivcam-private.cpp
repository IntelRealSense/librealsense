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
                if (it->unique_id == info.unique_id)
                {

                    result = *it;
                    if(result.mi == 4)
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
