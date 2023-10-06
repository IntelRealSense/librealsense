// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/subscription.h>
#include <memory>
#include <vector>


struct rs2_device_info;


namespace realdds {
class dds_participant;
}


namespace librealsense {


class device_info;
class context;
class rsdds_watcher_singleton;


// This factory creates "rsdds devices", or RealSense device proxies around a realdds device core.
//
// The factory abstracts away platform-specific concepts such that all the user has to do is supply a callback to know
// when changes in the list of devices have been made.
//
// Any devices created here will have a device-info that derives from dds_device_info.
//
class rsdds_device_factory
{
    context & _context;
    std::shared_ptr< realdds::dds_participant > _participant;
    std::shared_ptr< rsdds_watcher_singleton > _watcher_singleton;
    rsutils::subscription _subscription;

public:
    using callback = std::function< void( std::vector< rs2_device_info > & rs2_devices_info_removed,
                                          std::vector< rs2_device_info > & rs2_devices_info_added ) >;

    rsdds_device_factory( context &, callback && );
    ~rsdds_device_factory();

    // Query any subset of available devices and return them as device-info objects
    // Devices will match both the requested mask and the device-mask from the context settings
    //
    std::vector< std::shared_ptr< device_info > > query_devices( unsigned mask ) const;
};


}  // namespace librealsense
