// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/software-device.h>
#include "sid_index.h"

#include <memory>
#include <vector>


namespace realdds {
class dds_device;
}  // namespace realdds


namespace librealsense {


class stream;
class dds_sensor_proxy;
class stream_profile_interface;


// This is the rs2 device; it proxies to an actual DDS device that does all the actual
// work. For example:
//     auto dev_list = ctx.query_devices();
//     auto dev1 = dev_list[0];
//     auto dev2 = dev_list[0];
// dev1 and dev2 are two different rs2 devices, but they both go to the same DDS source!
//
class dds_device_proxy : public software_device
{
    std::shared_ptr< realdds::dds_device > _dds_dev;
    std::map< std::string, std::vector< std::shared_ptr< stream_profile_interface > > > _stream_name_to_profiles;
    std::map< std::string, std::shared_ptr< librealsense::stream > > _stream_name_to_librs_stream;
    std::map< std::string, std::shared_ptr< dds_sensor_proxy > > _stream_name_to_owning_sensor;

public:
    dds_device_proxy( std::shared_ptr< context > ctx, std::shared_ptr< realdds::dds_device > const & dev );

    std::shared_ptr< dds_sensor_proxy > create_sensor( const std::string & sensor_name );
};


}  // namespace librealsense