// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include <src/ds/features/auto-exposure-limit-feature.h>
#include <src/ds/d400/d400-options.h>
#include <src/ds/ds-private.h>
#include <src/sensor.h>
#include <librealsense2/hpp/rs_options.hpp>


namespace librealsense {


const feature_id auto_exposure_limit_feature::ID = "Auto Exposure Limit feature";

auto_exposure_limit_feature::auto_exposure_limit_feature( synthetic_sensor & sensor,
                                                          const std::shared_ptr< hw_monitor > & hw_monitor )
{
    auto exposure_range = sensor.get_option( RS2_OPTION_EXPOSURE ).get_range();

    auto fw_ver = firmware_version(sensor.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ));
    bool new_opcode =  fw_ver >= firmware_version( 5, 13, 0, 200 );

    option_range enable_range = { 0.f /*min*/, 1.f /*max*/, 1.f /*step*/, 0.f /*default*/ };

    // EXPOSURE Limit
    auto ae_limit_toggle_control = std::make_shared< limits_option >( RS2_OPTION_AUTO_EXPOSURE_LIMIT_TOGGLE,
                                                                      enable_range,
                                                                      "Toggle Auto-Exposure Limit",
                                                                      *hw_monitor,
                                                                      new_opcode );
    auto raw_depth_sensor = sensor.get_raw_sensor();
    auto ae_limit_value_control = std::make_shared< auto_exposure_limit_option >( *hw_monitor,
                                                                                  raw_depth_sensor,
                                                                                  exposure_range,
                                                                                  ae_limit_toggle_control,
                                                                                  new_opcode );
    sensor.register_option( RS2_OPTION_AUTO_EXPOSURE_LIMIT_TOGGLE, ae_limit_toggle_control );

    sensor.register_option(
        RS2_OPTION_AUTO_EXPOSURE_LIMIT,
        std::make_shared< auto_disabling_control >( ae_limit_value_control, ae_limit_toggle_control )) ;
   
   
}

feature_id auto_exposure_limit_feature::get_id() const
{
    return ID;
}

}  // namespace librealsense
