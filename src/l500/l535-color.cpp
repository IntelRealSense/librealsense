// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-color.h"
#include "l535-color.h"

#include <cstddef>
#include <mutex>



#include "l500-private.h"
#include "proc/color-formats-converter.h"
#include "algo/thermal-loop/l500-thermal-loop.h"


namespace librealsense
{
    using namespace ivcam2;

    l535_color::l535_color( std::shared_ptr< context > ctx,
                            const platform::backend_device_group & group )
        : device( ctx, group )
        , l500_device( ctx, group )
        , l500_color( ctx, group )
    {
        get_color_sensor()->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_Y411, RS2_STREAM_COLOR));
        get_color_sensor()->register_processing_block(
            { { RS2_FORMAT_Y411 } },
            { { RS2_FORMAT_RGB8, RS2_STREAM_COLOR } },
            []() { return std::make_shared<y411_converter>(RS2_FORMAT_RGB8); });
    }
}
