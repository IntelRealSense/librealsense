// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once


#include <src/feature-interface.h>
#include <src/hid-sensor.h>
#include <src/ds/ds-motion-common.h>

#include <memory>

namespace librealsense {


class gyro_sensitivity_feature : public feature_interface
{

public:
    static const feature_id ID;

    gyro_sensitivity_feature( std::shared_ptr< hid_sensor > motion_sensor, ds_motion_sensor & motion );

    feature_id get_id() const override;


};

}  // namespace librealsense
