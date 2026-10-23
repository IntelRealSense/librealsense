// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

namespace rs2
{
    // Where the object-detection overlay takes each detection's distance from.
    // The camera's Detection Distance control only turns the FW calculation on or off, so the
    // viewer keeps the full choice itself; Off and Host both leave the FW calculation off.
    enum class detection_distance_mode
    {
        fallback,   // no Detection Distance control: the FW distance when it reports one, else the viewer's estimate
        off,        // no distance
        host,       // the viewer's estimate from the depth stream
        on_camera   // the FW distance only
    };

    // The value the camera's Detection Distance control should hold for a mode
    inline float detection_distance_fw_value( detection_distance_mode mode )
    {
        return mode == detection_distance_mode::on_camera ? 1.f : 0.f;
    }

    // Whether the viewer has to estimate the distance itself, given the distance the FW reported
    inline bool needs_host_detection_distance( detection_distance_mode mode, float fw_distance )
    {
        return mode == detection_distance_mode::host
            || ( mode == detection_distance_mode::fallback && fw_distance <= 0.f );
    }

    // The distance to show for a detection; 0 shows none
    inline float pick_detection_distance( detection_distance_mode mode, float fw_distance, float host_distance )
    {
        switch( mode )
        {
        case detection_distance_mode::off:
            return 0.f;
        case detection_distance_mode::host:
            return host_distance;
        case detection_distance_mode::on_camera:
            return fw_distance;
        default:
            return fw_distance > 0.f ? fw_distance : host_distance;
        }
    }
}
