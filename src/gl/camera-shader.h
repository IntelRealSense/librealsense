// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "opengl3.h"
#include "synthetic-stream-gl.h"
#include "proc/synthetic-stream.h"
#include "pc-shader.h"

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

            static const auto OPTION_MOUSE_X = rs2_option(RS2_OPTION_COUNT + 1);
            static const auto OPTION_MOUSE_Y = rs2_option(RS2_OPTION_COUNT + 2);
            static const auto OPTION_MOUSE_PICK = rs2_option(RS2_OPTION_COUNT + 3);
            static const auto OPTION_WAS_PICKED = rs2_option(RS2_OPTION_COUNT + 4);
            static const auto OPTION_SELECTED = rs2_option(RS2_OPTION_COUNT + 5);
            
        private:
            std::vector<rs2::obj_mesh> camera_mesh;
            std::shared_ptr<camera_shader> _shader;
            std::vector<std::unique_ptr<rs2::vao>> _camera_model;

            std::shared_ptr<blit_shader> _blit;
            std::shared_ptr<rs2::texture_visualizer> _viz;
            std::shared_ptr<rs2::fbo> _fbo;
            option *_mouse_x_opt, *_mouse_y_opt, *_mouse_pick_opt,
                   *_was_picked_opt, *_selected_opt;
            uint32_t color_tex;
            uint32_t depth_tex;
        };

    }
}