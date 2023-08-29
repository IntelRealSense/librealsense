// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "frame-header.h"

#include <map>
#include <memory>
#include <array>
#include <cstring>  // memcpy


namespace librealsense {


/**
 \brief Metadata fields that are utilized internally by librealsense
 Provides extention to the r2_frame_metadata list of attributes
*/
enum frame_metadata_internal
{
    RS2_FRAME_METADATA_HW_TYPE = RS2_FRAME_METADATA_COUNT + 1, // 8-bit Module type: RS4xx, IVCAM
    RS2_FRAME_METADATA_SKU_ID,                                 // 8-bit SKU Id
    RS2_FRAME_METADATA_FORMAT,                                 // 16-bit Frame format
    RS2_FRAME_METADATA_WIDTH,                                  // 16-bit Frame width. pixels
    RS2_FRAME_METADATA_HEIGHT,                                 // 16-bit Frame height. pixels
    RS2_FRAME_METADATA_ACTUAL_COUNT
};


class md_attribute_parser_base;


// multimap is necessary here in order to permit registration to some metadata value in multiple
// places in metadata as it is required for D405, in which exposure should be available from the
// same sensor both for depth and color frames
typedef std::multimap< rs2_frame_metadata_value, std::shared_ptr< md_attribute_parser_base > > metadata_parser_map;

#pragma pack( push, 1 )
struct metadata_array_value
{
    bool is_valid;
    rs2_metadata_type value;
};
#pragma pack( pop )

typedef std::array< metadata_array_value, RS2_FRAME_METADATA_ACTUAL_COUNT > metadata_array;

static_assert( sizeof( metadata_array_value ) == sizeof( rs2_metadata_type ) + 1,
               "unexpected size for metadata array members" );


struct frame_additional_data : frame_header
{
    uint32_t metadata_size = 0;
    bool fisheye_ae_mode = false;  // TODO: remove in future release
    std::array< uint8_t, sizeof( metadata_array ) > metadata_blob = {};
    rs2_time_t last_timestamp = 0;
    unsigned long long last_frame_number = 0;
    bool is_blocking = false;  // when running from recording, this bit indicates
                               // if the recorder was configured to realtime mode or not
                               // if true, this will force any queue receiving this frame not to drop it

    float depth_units = 0.0f;  // adding depth units to frame metadata is a temporary solution, it
                               // will be replaced by FW metadata

    uint32_t raw_size = 0;  // The frame transmitted size (payload only)

    frame_additional_data() {}

    frame_additional_data( metadata_array const & metadata )
    {
        metadata_size = (uint32_t)sizeof( metadata );
        std::memcpy( metadata_blob.data(), metadata.data(), metadata_size );
    }

    frame_additional_data( rs2_time_t in_timestamp,
                           unsigned long long in_frame_number,
                           rs2_time_t in_system_time,
                           uint32_t md_size,
                           const uint8_t * md_buf,
                           rs2_time_t backend_time,
                           rs2_time_t last_timestamp,
                           unsigned long long last_frame_number,
                           bool in_is_blocking,
                           float in_depth_units = 0,
                           uint32_t transmitted_size = 0 )
        : frame_header( in_timestamp, in_frame_number, in_system_time, backend_time )
        , metadata_size( md_size )
        , last_timestamp( last_timestamp )
        , last_frame_number( last_frame_number )
        , is_blocking( in_is_blocking )
        , depth_units( in_depth_units )
        , raw_size( transmitted_size )
    {
        if( metadata_size )
            std::copy( md_buf, md_buf + std::min( size_t( md_size ), metadata_blob.size() ), metadata_blob.begin() );
    }
};


}  // namespace librealsense
