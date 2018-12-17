// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "align.h"

namespace librealsense
{
    std::shared_ptr<librealsense::align> create_align(rs2_stream align_to);
}
