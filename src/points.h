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
    const uint8_t* get_labels() const;
    unsigned int get_width() const;
    unsigned int get_height() const;
    // bytes per pixel = 1 vertex + 1 label
    static const size_t BYTES_PER_PIXEL = 3 * sizeof(float) + sizeof(uint8_t);
private:
    static const size_t LOW_RES_BUFFER_SIZE = 748800;   // 320 x 180 x 13 
    static const size_t HIGH_RES_BUFFER_SIZE = 2995200; // 640 x 360 x 13
    
    static const size_t LOW_RES_WIDTH = 320;  
    static const size_t LOW_RES_HEIGHT = 180;  
    
    static const size_t HIGH_RES_WIDTH = 640; 
    static const size_t HIGH_RES_HEIGHT = 360; 
};
MAP_EXTENSION( RS2_EXTENSION_LABELED_POINTS, librealsense::labeled_points );

}  // namespace librealsense
