// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RSCORE_HPP
#define LIBREALSENSE_RSCORE_HPP

#include "rs.h"

struct rs_frame_callback
{
    virtual void                            on_frame(const rs_stream_lock * lock, rs_frame_ref * f) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs_frame_callback() {}
};

struct rs_log_callback
{
    virtual void                            on_event(rs_log_severity severity, const char * message) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs_log_callback() {}
};

#endif
