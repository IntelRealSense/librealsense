/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

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
private:
                                                    rs_context(int);
    static bool                                     singleton_alive;
};

#endif
