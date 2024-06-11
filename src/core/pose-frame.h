// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/frame.h>
#include <src/types.h>
#include <src/float3.h>

#include "extension.h"

namespace librealsense {


class pose_frame : public frame
{
public:
    // pose frame data buffer is pose info struct
    struct pose_info
    {
        float3 translation;          // X, Y, Z values of translation, in meters (relative to initial position)
        float3 velocity;             // X, Y, Z values of velocity, in meter/sec
        float3 acceleration;         // X, Y, Z values of acceleration, in meter/sec^2
        float4 rotation;             // Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position)
        float3 angular_velocity;     // X, Y, Z values of angular velocity, in radians/sec
        float3 angular_acceleration; // X, Y, Z values of angular acceleration, in radians/sec^2
        uint32_t tracker_confidence; // pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High
        uint32_t mapper_confidence;  // pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High
    };

    pose_frame()
        : frame()
    {
    }

    const pose_info * get_pose_frame_data() const { return reinterpret_cast< const pose_info * >( get_frame_data() ); }

    float3 get_translation() const { return get_pose_frame_data()->translation; }
    float3 get_velocity() const { return get_pose_frame_data()->velocity; }
    float3 get_acceleration() const { return get_pose_frame_data()->acceleration; }
    float4 get_rotation() const { return get_pose_frame_data()->rotation; }
    float3 get_angular_velocity() const { return get_pose_frame_data()->angular_velocity; }
    float3 get_angular_acceleration() const { return get_pose_frame_data()->angular_acceleration; }
    uint32_t get_tracker_confidence() const { return get_pose_frame_data()->tracker_confidence; }
    uint32_t get_mapper_confidence() const { return get_pose_frame_data()->mapper_confidence; }
};

MAP_EXTENSION( RS2_EXTENSION_POSE_FRAME, librealsense::pose_frame );


}  // namespace librealsense
