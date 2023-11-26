// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include <src/ds/features/auto-exposure-roi-feature.h>
#include <src/ds/ds-device-common.h>
#include <src/ds/ds-private.h>
#include <src/sensor.h>


namespace librealsense {


/* static */ const feature_id auto_exposure_roi_feature::ID = "Auto exposure ROI feature";

auto_exposure_roi_feature::auto_exposure_roi_feature( synthetic_sensor & sensor,
                                                      std::shared_ptr< hw_monitor > hwm,
                                                      bool rgb )
{
    roi_sensor_interface * roi_sensor = dynamic_cast< roi_sensor_interface * >( &sensor );
    if( ! roi_sensor )
        throw std::runtime_error( "Sensor is not a roi_sensor_interface. Can't support auto_exposure_roi_feature" );

    ds::fw_cmd cmd = rgb ? ds::fw_cmd::SETRGBAEROI : ds::fw_cmd::SETAEROI;
    roi_sensor->set_roi_method( std::make_shared< ds_auto_exposure_roi_method >( *hwm, cmd ) );
}

feature_id auto_exposure_roi_feature::get_id() const
{
    return ID;
}


}  // namespace librealsense
