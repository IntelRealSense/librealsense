// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"

struct rs_device_info_list
{
    std::vector<std::shared_ptr<rs_device_info>> list;
};

struct rs_context
{
    rs_context();

    rs_device_info_list* query_devices() const;
    const rsimpl::uvc::backend& get_backend() const { return *_backend; }
private:
    std::shared_ptr<rsimpl::uvc::backend> _backend;
};
