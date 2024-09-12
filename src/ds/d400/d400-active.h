// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "d400-device.h"
#include "ds/ds-active-common.h"

namespace librealsense
{
    // Active means the HW includes an active projector
    class d400_active : public virtual d400_device
    {
    public:
        d400_active( std::shared_ptr< const d400_info > const & );
    private:
        std::shared_ptr<ds_active_common> _ds_active_common;
    };
}
