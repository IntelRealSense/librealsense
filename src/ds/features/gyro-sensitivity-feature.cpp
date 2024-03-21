// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <src/ds/features/gyro-sensitivity-feature.h>
#include <src/ds/d400/d400-options.h>
#include <src/ds/ds-private.h>
#include <src/sensor.h>
#include <librealsense2/hpp/rs_options.hpp>


namespace librealsense {

const feature_id gyro_sensitivity_feature::ID = "Gyro Sensitivity feature";

gyro_sensitivity_feature::gyro_sensitivity_feature( std::shared_ptr< hid_sensor > motion_sensor,
                                                   ds_motion_sensor & motion )
{
    option_range enable_range = { 0.f /*min*/, 4.f /*max*/, 1.f /*step*/, 1.f /*default*/ };
    auto imu_sensitivity_control = std::make_shared< gyro_sensitivity_option >( motion_sensor, enable_range );
    motion.register_option( RS2_OPTION_GYRO_SENSITIVITY, imu_sensitivity_control );
}


feature_id gyro_sensitivity_feature::get_id() const
{
    return ID;
}

}//namespace librealsense
