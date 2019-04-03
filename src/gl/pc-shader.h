// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "rendering.h"
#include "opengl3.h"
#include "synthetic-stream-gl.h"
#include "proc/synthetic-stream.h"

namespace librealsense
{
    namespace gl
    {
        class pointcloud_shader
        {
        public:
            pointcloud_shader();

            void begin();
            void end();

            void set_mvp(const rs2::matrix4& model,
                         const rs2::matrix4& view,
                         const rs2::matrix4& projection);


            int texture_slot() const { return 0; }
            int geometry_slot() const { return 1; }
            int uvs_slot() const { return 2; }

            void set_image_size(int width, int height);
            void set_min_delta_z(float min_delta_z);
        protected:
            pointcloud_shader(std::unique_ptr<rs2::shader_program> shader);

            std::unique_ptr<rs2::shader_program> _shader;

        private:
            void init();

            uint32_t _transformation_matrix_location;
            uint32_t _projection_matrix_location;
            uint32_t _camera_matrix_location;

            uint32_t _width_location, _height_location;
            uint32_t _min_delta_z_location;
        };

        class pointcloud_renderer : public stream_filter_processing_block, 
                                    public gpu_rendering_object,
                                    public matrix_container
        {
        public:
            pointcloud_renderer();
            ~pointcloud_renderer() override;

            static const auto OPTION_FILLED = rs2_option(RS2_OPTION_COUNT + 1);

            void cleanup_gpu_resources() override;
            void create_gpu_resources() override;

            rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        private:
            std::shared_ptr<pointcloud_shader> _shader;
            std::shared_ptr<rs2::vao> _model;
            std::shared_ptr<rs2::texture_buffer> _vertex_texture;
            std::shared_ptr<rs2::texture_buffer> _uvs_texture;
            int _width = 0;
            int _height = 0;
            option* _filled_opt;
        };
    }
}