// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
class color_sensor : public recordable< color_sensor >
{
public:
    virtual ~color_sensor() = default;

    void create_snapshot( std::shared_ptr< color_sensor > & snapshot ) const override;
    void
    enable_recording( std::function< void( const color_sensor & ) > recording_function ) override{};
};

MAP_EXTENSION( RS2_EXTENSION_COLOR_SENSOR, librealsense::color_sensor );

class color_sensor_snapshot
    : public virtual color_sensor
    , public extension_snapshot
{
public:
    color_sensor_snapshot() {}

    void update( std::shared_ptr< extension_snapshot > ext ) override {}

    void create_snapshot( std::shared_ptr< color_sensor > & snapshot ) const override
    {
        snapshot = std::make_shared< color_sensor_snapshot >( *this );
    }
    void
    enable_recording( std::function< void( const color_sensor & ) > recording_function ) override
    {
        // empty
    }
};
}  // namespace librealsense
