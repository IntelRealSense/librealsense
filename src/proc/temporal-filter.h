//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "types.h"

namespace librealsense
{
    class temporal_filter : public processing_block
    {
    public:
        temporal_filter();

    private:
        void on_set_temporal_magnitude(int val);
        void save_frames(rs2::frame new_frame);
        void smooth(rs2::frame result);

        std::mutex _mutex;
        std::vector<rs2::frame> _frames;
        std::vector<uint16_t> _values;
        uint8_t _num_of_frames;

    };
}
