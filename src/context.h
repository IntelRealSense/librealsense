// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_CONTEXT_H
#define LIBREALSENSE_CONTEXT_H

#include "types.h"
#include "uvc.h"

#include <map>

struct rs_context
{
    std::shared_ptr<rsimpl::uvc::context>                       context;
    std::map<std::string, std::shared_ptr<rsimpl::uvc::device>> id_to_device;
    std::vector<std::shared_ptr<rs_device>>                     devices;
    
                                                                rs_context();
                                                                ~rs_context();

    void                                                        enumerate_devices();
    rsimpl::uvc::device *                                       get_device(const std::string & id);
    void                                                        flush_device(const std::string & id);
private:
                                                                rs_context(int);
    static bool                                                 singleton_alive;
};

#endif
