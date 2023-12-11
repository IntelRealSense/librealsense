// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <rscore/device-factory.h>
#include <rsutils/subscription.h>


namespace realdds {
class dds_participant;
}


namespace librealsense {


class rsdds_watcher_singleton;


// This factory creates "rsdds devices", or RealSense device proxies around a realdds device core.
//
// The factory abstracts away platform-specific concepts such that all the user has to do is supply a callback to know
// when changes in the list of devices have been made.
//
// Any devices created here will have a device-info that derives from dds_device_info.
//
class rsdds_device_factory : public device_factory
{
    typedef device_factory super;

    std::shared_ptr< realdds::dds_participant > _participant;
    std::shared_ptr< rsdds_watcher_singleton > _watcher_singleton;
    rsutils::subscription _subscription;

public:
    rsdds_device_factory( std::shared_ptr< context > const &, callback && );
    ~rsdds_device_factory();

    // Query any subset of available devices and return them as device-info objects
    // Devices will match both the requested mask and the device-mask from the context settings
    //
    std::vector< std::shared_ptr< device_info > > query_devices( unsigned mask ) const override;
};


}  // namespace librealsense
