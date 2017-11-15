// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "source.h"
#include "core/processing.h"
#include "processing_block.h"
#include "decimation_filter.h"

namespace librealsense
{
    decimation_filter::decimation_filter()
    {
        auto on_frame = [this](rs2::frame f, const rs2::frame_source& source)
        {
            //frame_holder res = get_source().allocate_points(_stream, (frame_interface*)depth.get());
            std::cout << __FUNCTION__ << " TBD" << std::endl;
            //get_source().frame_ready(std::move(res));
        };
        auto callback = new rs2::frame_processor_callback<decltype(on_frame)>(on_frame);
        processing_block::set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(callback));
    }
}
