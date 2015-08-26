#pragma once
#ifndef LIBREALSENSE_CONTEXT_H
#define LIBREALSENSE_CONTEXT_H

#include "types.h"

struct rs_camera;

struct rs_context
{
    uvc_context_t *                                 context;
    std::vector<std::shared_ptr<rs_camera>>         cameras;

                                                    rs_context();
                                                    ~rs_context();

    void                                            query_device_list();
};

#endif
