// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame.h"

namespace librealsense {

class points : public frame
{
public:
    float3 * get_vertices();
    void export_to_ply( const std::string & fname, const frame_holder & texture );
    size_t get_vertex_count() const;
    float2 * get_texture_coordinates();
};

MAP_EXTENSION( RS2_EXTENSION_POINTS, librealsense::points );

}  // namespace librealsense
