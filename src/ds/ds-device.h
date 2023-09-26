// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/device.h>


namespace librealsense {


// Common base class for all Stereo devices
//
class ds_device : public virtual device
{
    typedef device super;

protected:
    ds_device( std::shared_ptr< const device_info > const & dev_info, bool device_changed_notifications = true )
        : super( dev_info, device_changed_notifications )
    {
    }

public:
    uint16_t get_pid() const { return _pid; }

protected:
    uint16_t _pid = 0;
};


}  // namespace librealsense
