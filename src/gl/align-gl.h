// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <vector>

#include "proc/align.h"
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
        class align_gl : public align, public gpu_processing_object
        {
        public:
            ~align_gl() override;
            align_gl(rs2_stream to_stream);

        protected:
            void align_z_to_other(rs2::video_frame& aligned, 
                const rs2::video_frame& depth, 
                const rs2::video_stream_profile& other_profile, 
                float z_scale) override;

            void align_other_to_z(rs2::video_frame& aligned, 
                const rs2::video_frame& depth, 
                const rs2::video_frame& other, 
                float z_scale) override;

            void cleanup_gpu_resources() override;
            void create_gpu_resources() override;

            void render(const rs2::points& model, 
                        const rs2::video_frame& tex, 
                        const rs2_intrinsics& camera,
                        const rs2_extrinsics& extr,
                        uint32_t output_texture);

            rs2_extension select_extension(const rs2::frame& input) override;

        private:
            int _enabled = 0;

            std::shared_ptr<rs2::gl::pointcloud> _pc;
            std::shared_ptr<rs2::gl::pointcloud_renderer> _renderer;
            std::shared_ptr<rs2::gl::uploader> _upload;
            std::shared_ptr<rs2::texture_buffer> _other_texture;
        };
    }
}
