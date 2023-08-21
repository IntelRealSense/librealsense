// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/device-info.h>

#include "playback_device.h"


namespace librealsense {


class playback_device_info : public device_info
{
    std::shared_ptr<playback_device> _dev;

public:
    explicit playback_device_info( std::shared_ptr<playback_device> dev )
        : device_info( nullptr ), _dev( dev )
    {

    }

    std::shared_ptr<device_interface> create_device() const override
    {
        return _dev;
    }
    platform::backend_device_group get_device_data() const override
    {
        return platform::backend_device_group( { platform::playback_device_info{ _dev->get_file_name() } } );
    }

    std::shared_ptr<device_interface> create( std::shared_ptr<context>, bool ) const override
    {
        return _dev;
    }
};


}  // namespace librealsense
