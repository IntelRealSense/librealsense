// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/frame.h>
#include "extension.h"


namespace librealsense {


class motion_frame : public frame
{
public:
    motion_frame()
        : frame()
    {
    }
};

MAP_EXTENSION( RS2_EXTENSION_MOTION_FRAME, librealsense::motion_frame );


}  // namespace librealsense
