// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "float3.h"
#include <memory>  // shared_ptr

namespace rs2 {
    class texture_buffer;
}

class skybox
{
public:
    skybox();
    void render(rs2::float3 cam_position);
    void reset() { initialized = false; }

private:
    std::shared_ptr<rs2::texture_buffer> plus_x, minus_x;
    std::shared_ptr<rs2::texture_buffer> plus_y, minus_y;
    std::shared_ptr<rs2::texture_buffer> plus_z, minus_z;
    bool initialized = false;
};