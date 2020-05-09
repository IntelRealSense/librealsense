// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "rendering.h"

class skybox
{
public:
    skybox(){}
    void render();

private:
    rs2::texture_buffer plus_x, minus_x;
    rs2::texture_buffer plus_y, minus_y;
    rs2::texture_buffer plus_z, minus_z;
    bool initialized = false;
};