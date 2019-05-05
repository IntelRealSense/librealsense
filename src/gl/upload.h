// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <vector>

#include "proc/synthetic-stream.h"
#include "synthetic-stream-gl.h"

#include <librealsense2/rs.hpp>
#include "opengl3.h"

#include <memory>

namespace rs2
{
    class stream_profile;
    class visualizer_2d;
}

namespace librealsense 
{
    namespace gl
    {
        class upload : public stream_filter_processing_block, public gpu_processing_object
        {
        public:
            upload();
            ~upload() override;

            void cleanup_gpu_resources() override;
            void create_gpu_resources() override;

            rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
        private:
            std::vector<int> _hist;
            std::vector<float> _fhist;
            int* _hist_data;
            float* _fhist_data;
            bool _enabled = false;
        };
    }
}
