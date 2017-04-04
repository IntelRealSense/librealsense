// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "ivcam-private.h"


namespace rsimpl2
{
    namespace ivcam
    {
        bool try_fetch_usb_device(std::vector<uvc::usb_device_info>& devices,
            const uvc::uvc_device_info& info, uvc::usb_device_info& result)
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
    } // namespace rsimpl::ivcam
} // namespace rsimpl
