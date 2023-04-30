// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame.h"

namespace librealsense {

class points : public frame
{
public:
    float3* get_vertices() const;
    size_t get_vertex_count() const;
    void export_to_ply( const std::string & fname, const frame_holder & texture );
    float2 * get_texture_coordinates();
};
MAP_EXTENSION( RS2_EXTENSION_POINTS, librealsense::points );

class labeled_points : public frame
{
public:
    float3* get_vertices() const;
    size_t get_vertex_count() const;
    uint8_t* get_labels() const;
private:
    static const int LABELS_RESOLUTION = 320 * 180;
    static const int OFFSET_TO_LABELS = 3 * sizeof(float) * LABELS_RESOLUTION;
};
MAP_EXTENSION( RS2_EXTENSION_LABELED_POINTS, librealsense::labeled_points );

}  // namespace librealsense
