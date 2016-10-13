// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"
#include <vector>

namespace rsimpl
{
    class device;

    class device_info
    {
    public:
        virtual std::shared_ptr<device> create(const uvc::backend& backend) const = 0;
        virtual std::shared_ptr<device_info> clone() const = 0;

        virtual ~device_info() = default;
    };

    class context
    {
    public:
        context();

        std::vector<std::shared_ptr<device_info>> query_devices() const;
        const uvc::backend& get_backend() const { return *_backend; }
    private:
        std::shared_ptr<uvc::backend> _backend;
    };
}
