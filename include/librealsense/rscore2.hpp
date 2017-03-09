// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RSCORE2_HPP
#define LIBREALSENSE_RSCORE2_HPP

#include "rs2.h"

struct rs2_frame_callback
{
    virtual void                            on_frame(rs2_frame * f) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs2_frame_callback() {}
};

struct rs2_notifications_callback
{
    virtual void                            on_notification(rs2_notification* n) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs2_notifications_callback() {}
};

struct rs2_log_callback
{
    virtual void                            on_event(rs2_log_severity severity, const char * message) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs2_log_callback() {}
};

#endif
