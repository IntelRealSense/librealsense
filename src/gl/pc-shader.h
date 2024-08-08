// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <matrix4.h>  // common/
#include <opengl3.h>  // common/
#include "synthetic-stream-gl.h"
#include "../proc/synthetic-stream.h"

namespace rs2 {
    class texture_buffer;
}


namespace librealsense
{
    namespace gl
    {
#pragma pack(push, 1)
        struct rgba8 { uint8_t r, g, b, a; };
        struct half4 { uint16_t x, y, z, w; };
#pragma pack(pop)

        class pointcloud_shader
        {
        public:
            pointcloud_shader(const char* vertex_shader, const char* fragment_shader);

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
            void set_picked_id(float pid);
            void set_shaded(bool shaded);
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
            uint32_t _picked_id_location;
            uint32_t _shaded_location;
        };

        class pointcloud_renderer : public stream_filter_processing_block, 
                                    public gpu_rendering_object,
                                    public matrix_container
        {
        public:
            pointcloud_renderer();
            ~pointcloud_renderer() override;

            static const auto OPTION_FILLED = rs2_option(RS2_OPTION_COUNT + 1);
            static const auto OPTION_SHADED = rs2_option(RS2_OPTION_COUNT + 2);

            static const auto OPTION_MOUSE_X = rs2_option(RS2_OPTION_COUNT + 3);
            static const auto OPTION_MOUSE_Y = rs2_option(RS2_OPTION_COUNT + 4);
            static const auto OPTION_MOUSE_PICK = rs2_option(RS2_OPTION_COUNT + 5);

            static const auto OPTION_PICKED_X = rs2_option(RS2_OPTION_COUNT + 6);
            static const auto OPTION_PICKED_Y = rs2_option(RS2_OPTION_COUNT + 7);
            static const auto OPTION_PICKED_Z = rs2_option(RS2_OPTION_COUNT + 8);

            static const auto OPTION_PICKED_ID = rs2_option(RS2_OPTION_COUNT + 9);

            static const auto OPTION_SELECTED = rs2_option(RS2_OPTION_COUNT + 10);
            static const auto OPTION_ORIGIN_PICKED = rs2_option(RS2_OPTION_COUNT + 11);

            static const auto OPTION_NORMAL_X = rs2_option(RS2_OPTION_COUNT + 12);
            static const auto OPTION_NORMAL_Y = rs2_option(RS2_OPTION_COUNT + 13);
            static const auto OPTION_NORMAL_Z = rs2_option(RS2_OPTION_COUNT + 14);

            static const auto OPTION_SCALE_FACTOR = rs2_option(RS2_OPTION_COUNT + 15);

            void cleanup_gpu_resources() override;
            void create_gpu_resources() override;

            rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        private:
            std::shared_ptr<pointcloud_shader> _shader;
            std::shared_ptr<pointcloud_shader> _pick_shader;
            std::shared_ptr<rs2::vao> _model;
            std::shared_ptr<rs2::texture_buffer> _vertex_texture;
            std::shared_ptr<rs2::texture_buffer> _uvs_texture;
            std::shared_ptr<rs2::texture_visualizer> _viz;
            std::shared_ptr<rs2::fbo> _fbo;
            int _width = 0;
            int _height = 0;
            option *_filled_opt, *_mouse_x_opt, *_mouse_y_opt, *_mouse_pick_opt,
                *_picked_id_opt, *_picked_x_opt, *_picked_y_opt, *_picked_z_opt, 
                *_selected_opt, *_shaded_opt, *_origin_picked_opt,
                *_normal_x_opt, *_normal_y_opt, *_normal_z_opt, *_scale_factor_opt;
            uint32_t color_tex;
            uint32_t xyz_tex;

            pbo<half4> _xyz_pbo;
            pbo<rgba8> _rgba_pbo;
            pbo<rgba8> _origin_rgba_pbo;
            std::deque<std::chrono::high_resolution_clock::time_point> _durations;
        };
    }
}