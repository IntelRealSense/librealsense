// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "d400-device.h"

namespace librealsense
{
    class d400_nonmonochrome : public virtual d400_device
    {
    public:
        d400_nonmonochrome( std::shared_ptr< const d400_info > const & );
    };
}
