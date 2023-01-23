// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs2 {

struct plane {
    float a;
    float b;
    float c;
    float d;
};

inline bool operator==(const plane& lhs, const plane& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d;
}

inline float evaluate_plane(const plane& plane, const float3& point) {
    return plane.a * point.x + plane.b * point.y + plane.c * point.z + plane.d;
}

}

