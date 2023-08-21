// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "platform/backend-device-group.h"

#include <memory>
#include <vector>


namespace librealsense {


class context;


class device_info
{
public:
    virtual std::shared_ptr< device_interface > create_device() const { return create( _ctx, true ); }

    virtual ~device_info() = default;

    virtual platform::backend_device_group get_device_data() const = 0;

    virtual bool operator==( const device_info & other ) const { return other.get_device_data() == get_device_data(); }

protected:
    explicit device_info( std::shared_ptr< context > backend )
        : _ctx( std::move( backend ) )
    {
    }

    virtual std::shared_ptr< device_interface > create( std::shared_ptr< context > ctx,
                                                        bool register_device_notifications ) const = 0;

    std::shared_ptr< context > _ctx;
};


}  // namespace librealsense
