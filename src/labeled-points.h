// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame.h"
#include "core/extension.h"
#include "float3.h"

namespace librealsense {

class labeled_points : public frame
{
public:
    float3* get_vertices();
    size_t get_vertex_count() const;
    const uint8_t* get_labels() const;
    unsigned int get_width() const;
    unsigned int get_height() const;
    size_t get_bpp() const; // bits per pixel
 
private:    

};
MAP_EXTENSION( RS2_EXTENSION_LABELED_POINTS, librealsense::labeled_points );

}  // namespace librealsense
