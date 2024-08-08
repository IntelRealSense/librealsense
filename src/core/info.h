// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "info-interface.h"

#include <librealsense2/h/rs_sensor.h>
#include <map>
#include <string>
#include <memory>


namespace librealsense {


// Simple implementation of info_interface
//
class LRS_EXTENSION_API info_container
    : public virtual info_interface
    , public extension_snapshot
{
public:
    void register_info( rs2_camera_info info, const std::string & val );
    void update_info( rs2_camera_info info, const std::string & val );

    // info_interface
    const std::string & get_info( rs2_camera_info info ) const override;
    bool supports_info( rs2_camera_info info ) const override;

    // extension_snapshot
    void create_snapshot( std::shared_ptr< info_interface > & snapshot ) const override;
    void enable_recording( std::function< void( const info_interface & ) > record_action ) override;
    void update( std::shared_ptr< extension_snapshot > ext ) override;

private:
    std::map< rs2_camera_info, std::string > _camera_info;
};


}  // namespace librealsense
