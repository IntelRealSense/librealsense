// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "rs-dds-device-info.h"
#include "rs-dds-device-proxy.h"

#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>


namespace librealsense {


std::shared_ptr< device_interface > dds_device_info::create( std::shared_ptr< context > ctx,
                                                             bool register_device_notifications ) const
{
    return std::make_shared< dds_device_proxy >( ctx, _dev );
}


platform::backend_device_group dds_device_info::get_device_data() const
{
    return platform::backend_device_group{ { platform::playback_device_info{ _dev->device_info().topic_root } } };
}


}  // namespace librealsense
