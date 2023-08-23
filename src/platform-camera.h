// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "device.h"


namespace librealsense {


class platform_camera : public device
{
public:
    platform_camera( const std::shared_ptr< context > & ctx,
                     const std::vector< platform::uvc_device_info > & uvc_infos,
                     const platform::backend_device_group & group,
                     bool register_device_notifications );

    virtual rs2_intrinsics get_intrinsics( unsigned int, const stream_profile & ) const { return rs2_intrinsics{}; }

    std::vector< tagged_profile > get_profiles_tags() const override;
};


class platform_camera_info : public device_info
{
    std::vector< platform::uvc_device_info > _uvcs;

public:
    explicit platform_camera_info( std::shared_ptr< context > const & ctx,
                                   std::vector< platform::uvc_device_info > && uvcs )
        : device_info( ctx )
        , _uvcs( std::move( uvcs ) )
    {
    }

    std::shared_ptr< device_interface > create( std::shared_ptr< context > ctx,
                                                bool register_device_notifications ) const override
    {
        return std::make_shared< platform_camera >( ctx,
                                                    _uvcs,
                                                    this->get_device_data(),
                                                    register_device_notifications );
    }

    static std::vector< std::shared_ptr< device_info > >
    pick_uvc_devices( const std::shared_ptr< context > & ctx,
                      const std::vector< platform::uvc_device_info > & uvc_devices );

    platform::backend_device_group get_device_data() const override { return platform::backend_device_group(); }
};


}  // namespace librealsense
