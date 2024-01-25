// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once


#include <src/feature-interface.h>

#include <memory>
#include <librealsense2/hpp/rs_options.hpp>

namespace librealsense {

class synthetic_sensor;
class hw_monitor;
class limits_option;

class gain_limit_feature : public feature_interface
{
public:
    static const feature_id ID;

    gain_limit_feature( synthetic_sensor & depth_sensor, std::shared_ptr< hw_monitor > hw_monitor );

    feature_id get_id() const override;
};

}  // namespace librealsense
