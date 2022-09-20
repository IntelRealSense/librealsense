// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"
#include <librealsense2/h/rs_internal.h>

#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace librealsense {

class stream_profile_interface;
using stream_profiles = std::vector<std::shared_ptr<stream_profile_interface>>;

namespace dds {

namespace topics {
class device_info;
}  // namespace topics


class dds_participant;


// Represents a device via the DDS system. Such a device exists as of its identification by the device-watcher, and
// always contains a device-info and GUID of the remote DataWriter to which it belongs.
// 
// The device may not be ready for use (will not contain sensors, profiles, etc.) until it is "run".
//
class dds_device
{
public:
    static std::shared_ptr< dds_device > find( dds_guid const & guid );

    static std::shared_ptr< dds_device > create( std::shared_ptr< dds_participant > const & participant,
                                                 dds_guid const & guid,
                                                 topics::device_info const & info );

    topics::device_info const & device_info() const;

    // The device GUID is that of the DataWriter which declares it!
    //
    dds_guid const & guid() const;

    bool is_running() const;

    // Make the device ready for use. This may take time! Better to do it in the background...
    void run();

    //----------- below this line, a device must be running!

    size_t num_of_sensors() const;

    size_t foreach_sensor( std::function< void( size_t sensor_index, const std::string & name ) > fn ) const;

    size_t foreach_video_profile( size_t sensor_index, std::function< void( const rs2_video_stream& profile, bool def_prof ) > fn ) const;
    size_t foreach_motion_profile( size_t sensor_index, std::function< void( const rs2_motion_stream& profile, bool def_prof ) > fn ) const;

    void sensor_open( size_t sensor_index, const stream_profiles & profiles );
    void sensor_close( size_t sensor_index );

private:
    class impl;
    std::shared_ptr< impl > _impl;

    // Ctor is private: use find() or create() instead. Same for dtor -- it should be automatic
    dds_device( std::shared_ptr< impl > );
};  // class dds_device


}  // namespace dds
}  // namespace librealsense
