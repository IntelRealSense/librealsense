// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"
#include <librealsense2/h/rs_internal.h>

#include <memory>
#include <functional>

namespace librealsense {
namespace dds {


namespace topics {
class device_info;
}  // namespace topics
class dds_participant;

class dds_device
{
public:
    static std::shared_ptr< dds_device > find( dds::dds_guid const & guid );

    static std::shared_ptr< dds_device > create( std::shared_ptr< dds::dds_participant > const & participant,
                                                 dds::dds_guid const & guid,
                                                 dds::topics::device_info const & info );

    topics::device_info const & device_info() const;

    size_t num_of_sensors() const;

    size_t foreach_sensor( std::function< void( const std::string& name ) > fn );

    size_t foreach_video_profile( size_t sensor_index, std::function< void( const rs2_video_stream& profile, bool def_prof ) > fn );
    size_t foreach_motion_profile( size_t sensor_index, std::function< void( const rs2_motion_stream& profile, bool def_prof ) > fn );

private:
    class impl;
    std::shared_ptr< impl > _impl;

    // Ctor is private: use find() or create() instead. Same for dtor -- it should be automatic
    dds_device( std::shared_ptr< impl > );
    ~dds_device();
};  // class dds_device


}  // namespace dds
}  // namespace librealsense
