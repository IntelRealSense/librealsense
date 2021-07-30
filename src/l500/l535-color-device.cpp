// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "l535-color-device.h"
#include "../proc/y411-converter.h"

using librealsense::ivcam2::l535::color_device;

color_device::color_device( std::shared_ptr< librealsense::context > ctx,
                            const librealsense::platform::backend_device_group & group )
    : device( ctx, group )
    , l500_device( ctx, group )
    , l500_color( ctx, group )
{
    get_color_sensor()->register_processing_block(
        librealsense::processing_block_factory::create_id_pbf( RS2_FORMAT_Y411, RS2_STREAM_COLOR ) );
    get_color_sensor()->register_processing_block(
        { { RS2_FORMAT_Y411 } },
        { { RS2_FORMAT_RGB8, RS2_STREAM_COLOR } },
        []() { return std::make_shared< librealsense::y411_converter >( RS2_FORMAT_RGB8 ); } );
}

