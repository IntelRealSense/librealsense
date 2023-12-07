// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <rscore/device-factory.h>
#include <rsutils/subscription.h>


namespace librealsense {


class device_watcher_singleton;


namespace platform {
struct backend_device_group;
class platform_device_info;
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
class backend_device_factory : public device_factory
{
    typedef device_factory super;

    std::shared_ptr< device_watcher_singleton > const _device_watcher;
    rsutils::subscription const _dtor;  // raii generic code, used to automatically unsubscribe our callback

public:
    backend_device_factory( std::shared_ptr< context > const &, callback && );
    ~backend_device_factory();

    // Query any subset of available devices and return them as device-info objects
    // Devices will match both the requested mask and the device-mask from the context settings
    //
    std::vector< std::shared_ptr< device_info > > query_devices( unsigned mask ) const override;

private:
    std::vector< std::shared_ptr< platform::platform_device_info > >
        create_devices_from_group( platform::backend_device_group, int mask ) const;
};


}  // namespace librealsense
