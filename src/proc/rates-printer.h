// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "synthetic-stream.h"
#include <iomanip>
#include <map>

namespace librealsense
{
    class rates_printer : public generic_processing_block
    {
    public:
        rates_printer() :generic_processing_block("Rates printer"){}
        virtual ~rates_printer() = default;

    protected:
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
        bool should_process(const rs2::frame& frame) override;
    private:
        class profile
        {
        private:
            rs2::stream_profile _stream_profile;
            int _counter;
            std::vector<std::chrono::steady_clock::time_point> _time_points;
            int _last_frame_number;
            float _acctual_fps;
            std::chrono::steady_clock::time_point _last_time;
        public:
            profile();
            int last_frame_number();
            rs2::stream_profile get_stream_profile();
            float get_fps();
            void on_frame_arrival(const rs2::frame& f);
        };

        void print();

        int _render_rate = 2;
        std::map<const rs2_stream_profile*, profile> _profiles;
        std::chrono::steady_clock::time_point _last_print_time;
    };
}
