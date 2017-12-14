// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "types.h"

namespace librealsense
{
    const size_t CREDIBILITY_MAP_SIZE = 256;

    class temporal_filter : public processing_block
    {
    public:
        temporal_filter();

    protected:
        void    update_configuration(const rs2::frame& f);

        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);

        void temp_jw_smooth(uint16_t * frame_data, uint16_t * _last_frame_data, uint8_t *history);

    private:
        void on_set_confidence_control(uint8_t val);
        void on_set_alpha(float val);
        void on_set_delta(float val);

        void recalc_creadibility_map();
        std::mutex _mutex;
        uint8_t                 _credibility_param;

        float                   _alpha_param;
        float                   _one_minus_alpha;
        uint8_t                 _delta_param;
        size_t                  _width, _height;
        size_t                  _current_frm_size_pixels;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
        std::map < size_t, std::vector<uint16_t> > _last_frame_map; // Hold the last frame for each size
        std::map < size_t, std::vector<uint8_t> > _history;    // represents the history over the last 8 frames, 1 bit per frame
        uint8_t                 _cur_frame_index; // mod 8
        std::array<uint8_t, CREDIBILITY_MAP_SIZE> _credibility_map;  // encodes whether a particular 8 bit history is good enough for all 8 phases of storage
    };
}
