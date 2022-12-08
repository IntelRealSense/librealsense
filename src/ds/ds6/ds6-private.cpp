//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds6-private.h"

using namespace std;

namespace librealsense
{
    namespace ds
    {
        bool ds6_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {
                    bool found = false;
                    result = *it;
                    switch (info.pid)
                    {
                    case RS_D585_PID:
                    case RS_S585_PID:
                        found = (result.mi == 6);
                        break;
                    default:
                        throw not_implemented_exception(to_string() << "USB device "
                            << std::hex << info.pid << ":" << info.vid << std::dec << " is not supported.");
                        break;
                    }

                    if (found)
                    {
                        devices.erase(it);
                        return true;
                    }
                }
            }
            return false;
        }
    } // librealsense::ds
} // namespace librealsense
