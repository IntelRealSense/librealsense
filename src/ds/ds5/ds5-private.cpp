//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "ds5-private.h"

using namespace std;

namespace librealsense
{
    namespace ds
    {
        bool ds5_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
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
                    case RS_USB2_PID:
                    case RS400_PID:
                    case RS405U_PID:
                    case RS410_PID:
                    case RS416_PID:
                    case RS460_PID:
                    case RS430_PID:
                    case RS420_PID:
                    case RS400_IMU_PID:
                        found = (result.mi == 3);
                        break;
                    case RS405_PID:
                    case RS430I_PID:
                        found = (result.mi == 4);
                        break;
                    case RS430_MM_PID:
                    case RS420_MM_PID:
                    case RS435I_PID:
                    case RS455_PID:
                        found = (result.mi == 6);
                        break;
                    case RS415_PID:
                    case RS416_RGB_PID:
                    case RS435_RGB_PID:
                    case RS465_PID:
                        found = (result.mi == 5);
                        break;
                    default:
                        throw not_implemented_exception(rsutils::string::from() << "USB device "
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

        std::vector<platform::uvc_device_info> filter_ds5_device_by_capability(const std::vector<platform::uvc_device_info>& devices,
            d400_caps caps)
        {
            std::vector<platform::uvc_device_info> results;

            switch (caps)
            {
            case d400_caps::CAP_FISHEYE_SENSOR:
                std::copy_if(devices.begin(), devices.end(), std::back_inserter(results),
                    [](const platform::uvc_device_info& info)
                    {
                        return ds5_fisheye_pid.find(info.pid) != ds5_fisheye_pid.end();
                    });
                break;
            default:
                throw invalid_value_exception(rsutils::string::from()
                    << "Capability filters are not implemented for val "
                    << std::hex << caps << std::dec);
            }
            return results;
        }
    } // librealsense::ds
} // namespace librealsense
