// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_CONTEXT_H
#define LIBREALSENSE_CONTEXT_H

#include "types.h"
#include "uvc.h"

struct rs_context_base : rs_context
{
    std::shared_ptr<rsimpl::uvc::context>           context;
    std::vector<std::shared_ptr<rs_device>>         devices;

                                                    rs_context_base();
                                                    ~rs_context_base();

    static rs_context*                              acquire_instance();
    static void                                     release_instance();

    size_t                                          get_device_count() const override;
    rs_device *                                     get_device(int index) const override;
private:
    static int                                      ref_count;
    static std::mutex                               instance_lock;
    static rs_context*                              instance;
    static std::string                              api_version;
};

#endif
