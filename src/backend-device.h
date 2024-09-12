// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/device.h>
#include <memory>


namespace librealsense {


namespace platform {
class backend;
}


// Common base class for all backend devices (i.e., those that require a platform backend)
//
class backend_device : public virtual device
{
    typedef device super;

protected:
    backend_device( std::shared_ptr< const device_info > const & dev_info, bool device_changed_notifications = true )
        : super( dev_info, device_changed_notifications )
    {
    }

public:
    uint16_t get_pid() const { return _pid; }
    std::shared_ptr< platform::backend > get_backend();

    virtual bool is_gyro_high_sensitivity() const { return false; }

protected:
    uint16_t _pid = 0;
};


}  // namespace librealsense
