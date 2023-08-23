// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/software-device.h>
#include "sid_index.h"

#include <memory>
#include <vector>


namespace realdds {
class dds_device;
class dds_stream;
class dds_video_stream;
class dds_motion_stream;
class dds_stream_profile;
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

    int get_index_from_stream_name( const std::string & name ) const;
    void set_profile_intrinsics( std::shared_ptr< stream_profile_interface > & profile,
                                 const std::shared_ptr< realdds::dds_stream > & stream ) const;
    void set_video_profile_intrinsics( std::shared_ptr< stream_profile_interface > profile,
                                       std::shared_ptr< realdds::dds_video_stream > stream ) const;
    void set_motion_profile_intrinsics( std::shared_ptr< stream_profile_interface > profile,
                                       std::shared_ptr< realdds::dds_motion_stream > stream ) const;

public:
    dds_device_proxy( std::shared_ptr< const device_info > const &, std::shared_ptr< realdds::dds_device > const & dev );

    void tag_default_profile_of_stream( const std::shared_ptr< stream_profile_interface > & profile,
                                        const std::shared_ptr< const realdds::dds_stream > & stream ) const;

    std::shared_ptr< dds_sensor_proxy > create_sensor( std::string const & sensor_name, rs2_stream sensor_type );

    void tag_profiles( stream_profiles profiles ) const override;
};


}  // namespace librealsense