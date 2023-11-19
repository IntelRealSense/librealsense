// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/feature-interface.h>

#include <string>


namespace librealsense {

class synthetic_sensor;
class hw_monitor;
class ds_auto_exposure_roi_method;

class auto_exposure_roi_feature : public feature_interface
{
public:
    static constexpr const char * ID = "Auto exposure ROI feature";

    auto_exposure_roi_feature( synthetic_sensor & raw_sensor, std::shared_ptr< hw_monitor > hwm, bool rgb = false );

private:
    bool _rgb;
    synthetic_sensor & _sensor;
    std::shared_ptr< hw_monitor > _hwm;
    std::shared_ptr< ds_auto_exposure_roi_method > _roi_method;
};

}  // namespace librealsense
