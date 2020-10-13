// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../backend.h"
#include "../command_transfer.h"

namespace librealsense
{
    namespace platform
    {
        class rs_backend : public backend
        {
        public:
            rs_backend();
            ~rs_backend();

            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;


            // don't change
            std::shared_ptr<command_transfer> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;

            // Not supported
            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override;
            std::vector<hid_device_info> query_hid_devices() const override;

            // don't change
            virtual std::shared_ptr<time_service> create_time_service() const override;
        };
    }
}
