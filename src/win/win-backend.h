// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../backend.h"

namespace rsimpl
{
    namespace uvc
    {
        class wmf_backend : public std::enable_shared_from_this<wmf_backend>, public backend
        {
        public:
            wmf_backend();
            ~wmf_backend();

            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;
        };
    }
}
