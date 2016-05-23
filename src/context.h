// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_CONTEXT_H
#define LIBREALSENSE_CONTEXT_H

#include "types.h"
#include "uvc.h"

struct rs_context
{
    std::shared_ptr<rsimpl::uvc::context>           context;
    std::vector<std::shared_ptr<rs_device>>         devices;

                                                    rs_context();
                                                    ~rs_context();

    static rs_context*                              acquire_instance();
    static void                                     release_instance();
private:
    static int                                      ref_count;
    static std::mutex                               instance_lock;
    static rs_context*                              instance;
};

#endif
