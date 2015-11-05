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
