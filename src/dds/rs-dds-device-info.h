// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/context.h>  // device_info, context

#include <memory>


namespace realdds {
class dds_device;
}  // namespace realdds


namespace librealsense {


class dds_device_proxy;
class device_interface;


// Factory class for dds_device_proxy.
// 
// This is what's created when context::query_devices() is used, or when a watcher detects a new DDS device: it just
// points to a (possibly not running) DDS device that is only instantiated when create() is called.
//
class dds_device_info : public device_info
{
    std::shared_ptr< realdds::dds_device > _dev;

public:
    dds_device_info( std::shared_ptr< context > const & ctx, std::shared_ptr< realdds::dds_device > const & dev )
        : device_info( ctx )
        , _dev( dev )
    {
    }

    std::shared_ptr< device_interface > create( std::shared_ptr< context > ctx,
                                                bool register_device_notifications ) const override;

    platform::backend_device_group get_device_data() const override;
};


}  // namespace librealsense
