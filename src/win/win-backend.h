// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../backend.h"

namespace rsimpl
{
    namespace uvc
    {
        class wmf_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> enumerate_uvc_devices() const override;

            std::unique_ptr<usb_device> create_usb_device(uvc_device_info info) const override;
            std::vector<usb_device_info> enumerate_usb_devices() const override;
        };
    }
}
