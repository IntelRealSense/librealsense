// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#include <src/feature-interface.h>


namespace librealsense {

class d500_depth_sensor;

// Mirrors temporal_filter_feature exactly (see temporal-filter-feature.h) - wires the HKR Improved Close Range
// Control composite-option embedded filter (hdrd_embedded_filter) onto a d500_depth_sensor.
class hdrd_filter_feature : public feature_interface
{
public:
    static const feature_id ID;

    explicit hdrd_filter_feature( d500_depth_sensor & depth_sensor );

    feature_id get_id() const override;
};

}  // namespace librealsense
