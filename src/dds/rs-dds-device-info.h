// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/context.h>
#include <src/device-info.h>

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

    std::string get_address() const override;
    void to_stream( std::ostream & ) const override;

    std::shared_ptr< device_interface > create_device() override;

    bool is_same_as( std::shared_ptr< const device_info > const & other ) const override;
};


}  // namespace librealsense
