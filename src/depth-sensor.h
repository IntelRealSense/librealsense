// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
class depth_sensor : public recordable< depth_sensor >
{
public:
    virtual float get_depth_scale() const = 0;
    virtual ~depth_sensor() = default;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_SENSOR, librealsense::depth_sensor );

class depth_sensor_snapshot
    : public virtual depth_sensor
    , public extension_snapshot
{
public:
    depth_sensor_snapshot( float depth_units )
        : m_depth_units( depth_units )
    {
    }
    float get_depth_scale() const override { return m_depth_units; }

    void update( std::shared_ptr< extension_snapshot > ext ) override
    {
        if( auto api = As< depth_sensor >( ext ) )
        {
            m_depth_units = api->get_depth_scale();
        }
    }
    void create_snapshot( std::shared_ptr< depth_sensor > & snapshot ) const override
    {
        snapshot = std::make_shared< depth_sensor_snapshot >( *this );
    }
    void
    enable_recording( std::function< void( const depth_sensor & ) > recording_function ) override
    {
        // empty
    }

protected:
    float m_depth_units;
};

class depth_stereo_sensor
    : public virtual depth_sensor
    , public recordable< depth_stereo_sensor >
{
public:
    virtual float get_stereo_baseline_mm() const = 0;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_STEREO_SENSOR, librealsense::depth_stereo_sensor );

class depth_stereo_sensor_snapshot
    : public depth_stereo_sensor
    , public depth_sensor_snapshot
{
public:
    depth_stereo_sensor_snapshot( float depth_units, float stereo_bl_mm )
        : depth_sensor_snapshot( depth_units )
        , m_stereo_baseline_mm( stereo_bl_mm )
    {
    }

    float get_stereo_baseline_mm() const override { return m_stereo_baseline_mm; }

    void update( std::shared_ptr< extension_snapshot > ext ) override
    {
        depth_sensor_snapshot::update( ext );

        if( auto api = As< depth_stereo_sensor >( ext ) )
        {
            m_stereo_baseline_mm = api->get_stereo_baseline_mm();
        }
    }

    void create_snapshot( std::shared_ptr< depth_stereo_sensor > & snapshot ) const override
    {
        snapshot = std::make_shared< depth_stereo_sensor_snapshot >( *this );
    }

    void enable_recording(
        std::function< void( const depth_stereo_sensor & ) > recording_function ) override
    {
        // empty
    }

private:
    float m_stereo_baseline_mm;
};
}  // namespace librealsense
