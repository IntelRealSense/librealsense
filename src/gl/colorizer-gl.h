// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <vector>

#include "proc/synthetic-stream.h"
#include "synthetic-stream-gl.h"

#include <librealsense2/rs.hpp>
#include "opengl3.h"

#include "proc/colorizer.h"

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
        class colorizer : public librealsense::colorizer, 
            public gpu_processing_object
        {
        public:
            colorizer();
            ~colorizer() override;

            void cleanup_gpu_resources() override;
            void create_gpu_resources() override;

            static void populate_floating_histogram(float* f, int* hist);
            
            rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
        private:
            int _enabled = 0;

            int _width, _height;

            uint32_t _cm_texture;
            int _last_selected_cm = -1;

            std::vector<float> _fhist;
            float* _fhist_data;

            std::shared_ptr<rs2::visualizer_2d> _viz;
            std::shared_ptr<rs2::fbo> _fbo;
        };
    }
}
