// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "opengl3.h"
#include "synthetic-stream-gl.h"
#include "proc/synthetic-stream.h"

namespace librealsense
{
    namespace gl
    {
        class camera_shader
        {
        public:
            camera_shader();

            void begin();
            void end();

            void set_mvp(const rs2::matrix4& model,
                        const rs2::matrix4& view,
                        const rs2::matrix4& projection);
        protected:
            std::unique_ptr<rs2::shader_program> _shader;

        private:
            void init();

            uint32_t _transformation_matrix_location;
            uint32_t _projection_matrix_location;
            uint32_t _camera_matrix_location;
        };

        class camera_renderer : public stream_filter_processing_block, 
                                public gpu_rendering_object,
                                public matrix_container
        {
        public:
            camera_renderer();
            ~camera_renderer() override;

            void cleanup_gpu_resources() override;
            void create_gpu_resources() override;

            rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;
            
        private:
            std::vector<rs2::obj_mesh> camera_mesh;
            std::shared_ptr<camera_shader> _shader;
            std::vector<std::unique_ptr<rs2::vao>> _camera_model;
        };

    }
}