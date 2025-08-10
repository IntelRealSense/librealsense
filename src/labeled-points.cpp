// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "labeled-points.h"
#include "core/video.h"
#include "core/video-frame.h"
#include "core/frame-holder.h"
#include "librealsense-exception.h"
#include <rsutils/string/from.h>

namespace librealsense {

static constexpr size_t LOW_RES_WIDTH = 320;  
static constexpr size_t LOW_RES_HEIGHT = 180;  
    
static constexpr size_t HIGH_RES_WIDTH = 640; 
static constexpr size_t HIGH_RES_HEIGHT = 360; 

// bytes per pixel = 1 vertex + 1 label
static constexpr size_t BYTES_PER_PIXEL = 3 * sizeof(float) + sizeof(uint8_t);

static constexpr size_t LOW_RES_BUFFER_SIZE = LOW_RES_WIDTH * LOW_RES_HEIGHT * BYTES_PER_PIXEL;   // 320 x 180 x 13 
static constexpr size_t HIGH_RES_BUFFER_SIZE = HIGH_RES_WIDTH * HIGH_RES_HEIGHT * BYTES_PER_PIXEL; // 640 x 360 x 13


size_t labeled_points::get_vertex_count() const
{
    return data.size() / BYTES_PER_PIXEL;
}

float3* labeled_points::get_vertices()
{
    get_frame_data();  // call GetData to ensure data is in main memory
    return (float3 *)data.data();
}

const uint8_t* labeled_points::get_labels() const
{
    get_frame_data();  // call GetData to ensure data is in main memory
    return data.data() + 3 * sizeof(float) * get_vertex_count();
}

unsigned int labeled_points::get_width() const
{
    unsigned int width = 0;
    auto buffer_size = data.size();
    switch( buffer_size )
    {
    case LOW_RES_BUFFER_SIZE: 
        width = LOW_RES_WIDTH;
        break;
    case HIGH_RES_BUFFER_SIZE: 
        width = HIGH_RES_WIDTH;
        break;
    default:
        throw wrong_api_call_sequence_exception( rsutils::string::from()
                                                 << "unsupported buffer size of " << buffer_size
                                                 << " received for labeled point cloud" );
        break;
    }
    return width;
}

unsigned int labeled_points::get_height() const
{
    unsigned int height = 0;
    auto buffer_size = data.size();
    switch( buffer_size )
    {
    case LOW_RES_BUFFER_SIZE:
        height = LOW_RES_HEIGHT;
        break;
    case HIGH_RES_BUFFER_SIZE:
        height = HIGH_RES_HEIGHT;
        break;
    default:
        throw wrong_api_call_sequence_exception( rsutils::string::from()
            << "unsupported buffer size of " << buffer_size
            << " received for labeled point cloud" );        
        break;
    }
    return height;
}

size_t labeled_points::get_bpp() const
{
    return BYTES_PER_PIXEL * 8;
}  
} // namespace librealsense
