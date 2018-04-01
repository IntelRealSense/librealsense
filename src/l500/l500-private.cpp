//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-private.h"

using namespace std;

namespace librealsense
{
    namespace ivcam2
    {
        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
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
    } // librealsense::ivcam2
} // namespace librealsense
