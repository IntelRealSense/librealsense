// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame.h"

namespace librealsense {

class vertices_frame : public frame
{
public:
    virtual float3* get_vertices();
    virtual size_t get_vertex_count() const;
};
MAP_EXTENSION( RS2_EXTENSION_VERTICES_FRAME, librealsense::vertices_frame );

class points : public vertices_frame
{
public:
    void export_to_ply( const std::string & fname, const frame_holder & texture );
    float2 * get_texture_coordinates();
};
MAP_EXTENSION( RS2_EXTENSION_POINTS, librealsense::points );

class attributes_frame : public vertices_frame
{
public:
    float3* get_vertices() override;
    byte* get_attributes();
    size_t get_vertex_count() const override;
};
MAP_EXTENSION( RS2_EXTENSION_ATTRIBUTES_FRAME, librealsense::attributes_frame );

}  // namespace librealsense
