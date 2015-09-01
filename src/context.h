#pragma once
#ifndef LIBREALSENSE_CONTEXT_H
#define LIBREALSENSE_CONTEXT_H

#include "types.h"
#include "uvc.h"

struct rs_camera;

struct rs_context
{
    rsimpl::uvc::context                            context;
    std::vector<std::shared_ptr<rs_camera>>         cameras;

                                                    rs_context();
                                                    ~rs_context();

    void                                            query_device_list();
};

#endif
