#pragma once
#ifndef LIBREALSENSE_CONTEXT_H
#define LIBREALSENSE_CONTEXT_H

#include "types.h"
#include "uvc.h"

struct rs_context
{
    rsimpl::uvc::context                            context;
    std::vector<std::shared_ptr<rs_device>>         devices;

                                                    rs_context();
};

#endif
