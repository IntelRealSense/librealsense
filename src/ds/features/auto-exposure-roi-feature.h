// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once


#include <src/feature-interface.h>

#include <memory>


namespace librealsense {

class synthetic_sensor;
class hw_monitor;

class auto_exposure_roi_feature : public feature_interface
{
public:
    static const feature_id ID;

    auto_exposure_roi_feature( synthetic_sensor & sensor, std::shared_ptr< hw_monitor > hwm, bool rgb = false );

    feature_id get_id() const override;
};

}  // namespace librealsense
