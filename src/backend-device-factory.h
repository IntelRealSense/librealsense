// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <functional>
#include <memory>
#include <vector>


struct rs2_device_info;


namespace librealsense {


class device_info;
class context;


namespace platform {
class device_watcher;
struct backend_device_group;
}  // namespace platform


// This factory creates "backend devices", or devices that require the backend to be detected and used. In other words,
// UVC devices.
// 
// The factory abstracts away platform-specific concepts such that all the user has to do is supply a callback to know
// when changes in the list of devices have been made.
// 
// Any devices created here will have a device-info that derives from platform::platform_device_info. This factory
// manages these device-info objects such that lifetime is tracked and updated appropriately, without the caller's
// knowledge.
//
class backend_device_factory
{
    context & _context;
    std::shared_ptr< platform::device_watcher > const _device_watcher;

    using callback = std::function< void( std::vector< rs2_device_info > & rs2_devices_info_removed,
                                          std::vector< rs2_device_info > & rs2_devices_info_added ) >;

    callback const _callback;

public:
    backend_device_factory( context &, callback );
    ~backend_device_factory();

    // Query any subset of available devices and return them as device-info objects
    // Devices will match both the requested mask and the device-mask from the context settings
    //
    std::vector< std::shared_ptr< device_info > > query_devices( unsigned mask ) const;

private:
    std::vector< std::shared_ptr< device_info > > create_devices_from_group( platform::backend_device_group,
                                                                             int mask ) const;
};


}  // namespace librealsense
