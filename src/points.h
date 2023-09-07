// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame.h"
#include "core/extension.h"
#include "types.h"

#include <string>


namespace librealsense {


class frame_holder;


class points : public frame
{
public:
    float3 * get_vertices();
    void export_to_ply( const std::string & fname, const frame_holder & texture );
    size_t get_vertex_count() const;
    float2 * get_texture_coordinates();

};
MAP_EXTENSION( RS2_EXTENSION_POINTS, librealsense::points );

class labeled_points : public frame
{
public:
    float3* get_vertices();
    size_t get_vertex_count() const;
    uint8_t* get_labels() const;
    // bytes per pixel = 1 vertex + 1 label
    static const size_t BYTES_PER_PIXEL = 3 * sizeof(float) + sizeof(uint8_t);
private:
    static const size_t LABELS_RESOLUTION = 320 * 180;
    static const size_t OFFSET_TO_LABELS = 3 * sizeof(float) * LABELS_RESOLUTION;
};
MAP_EXTENSION( RS2_EXTENSION_LABELED_POINTS, librealsense::labeled_points );

}  // namespace librealsense
