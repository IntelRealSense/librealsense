// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "pc-shader.h"
#include "synthetic-stream-gl.h"
#include <glad/glad.h>

#include "option.h"

static const char* vertex_shader_text =
"#version 110\n"
"\n"
"attribute vec3 position;\n"
"attribute vec2 textureCoords;\n"
"\n"
"varying float valid;\n"
"varying vec2 sampledUvs;\n"
"\n"
"uniform mat4 transformationMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 cameraMatrix;\n"
"\n"
"uniform sampler2D uvsSampler;\n"
"uniform sampler2D positionsSampler;\n"
"\n"
"uniform float imageWidth;\n"
"uniform float imageHeight;\n"
"uniform float minDeltaZ;\n"
"\n"
"void main(void) {\n"
"    float pixelWidth = 1.0 / imageWidth;\n"
"    float pixelHeight = 1.0 / imageHeight;\n"
"    vec2 tex = vec2(textureCoords.x, textureCoords.y);\n"
"    vec4 pos = texture2D(positionsSampler, tex);\n"
"    vec4 uvs = texture2D(uvsSampler, tex);\n"
"\n"
"    vec2 tex_left = vec2(max(textureCoords.x - pixelWidth, 0.0), textureCoords.y);\n"
"    vec2 tex_right = vec2(min(textureCoords.x + pixelWidth, 1.0), textureCoords.y);\n"
"    vec2 tex_top = vec2(textureCoords.x, max(textureCoords.y - pixelHeight, 0.0));\n"
"    vec2 tex_buttom = vec2(textureCoords.x, min(textureCoords.y + pixelHeight, 1.0));\n"
"\n"
"    vec4 pos_left = texture2D(positionsSampler, tex_left);\n"
"    vec4 pos_right = texture2D(positionsSampler, tex_right);\n"
"    vec4 pos_top = texture2D(positionsSampler, tex_top);\n"
"    vec4 pos_buttom = texture2D(positionsSampler, tex_buttom);\n"
"\n"
"    valid = 0.0;\n"
"    if (uvs.x < 0.0) valid = 1.0;\n"
"    if (uvs.y < 0.0) valid = 1.0;\n"
"    if (uvs.x >= 1.0) valid = 1.0;\n"
"    if (uvs.y >= 1.0) valid = 1.0;\n"
"    if (abs(pos_left.z - pos.z) > minDeltaZ) valid = 1.0;\n"
"    if (abs(pos_right.z - pos.z) > minDeltaZ) valid = 1.0;\n"
"    if (abs(pos_top.z - pos.z) > minDeltaZ) valid = 1.0;\n"
"    if (abs(pos_buttom.z - pos.z) > minDeltaZ) valid = 1.0;\n"
"    if (abs(pos.z) < 0.01) valid = 1.0;\n"
"    if (valid > 0.0) pos = vec4(1.0, 1.0, 1.0, 0.0);\n"
"    else pos = vec4(pos.xyz, 1.0);\n"
"    vec4 worldPosition = transformationMatrix * pos;\n"
"    gl_Position = projectionMatrix * cameraMatrix * worldPosition;\n"
"\n"
"    sampledUvs = uvs.xy;\n"
"}\n";

static const char* fragment_shader_text =
"#version 110\n"
"\n"
"varying float valid;\n"
"varying vec2 sampledUvs;\n"
"\n"
"uniform sampler2D textureSampler;\n"
"\n"
"void main(void) {\n"
"    if (valid > 0.0) discard;\n"
"    vec4 color = texture2D(textureSampler, sampledUvs);\n"
"\n"
"    gl_FragColor = vec4(color.xyz, 1.0);\n"
"}\n";

using namespace rs2;

namespace librealsense
{
    namespace gl
    {
        pointcloud_shader::pointcloud_shader(std::unique_ptr<shader_program> shader)
            : _shader(std::move(shader))
        {
            init();
        }

        pointcloud_shader::pointcloud_shader()
        {
            _shader = shader_program::load(
                vertex_shader_text,
                fragment_shader_text,
                "position", "textureCoords");

            init();
        }

        void pointcloud_shader::init()
        {
            _transformation_matrix_location = _shader->get_uniform_location("transformationMatrix");
            _projection_matrix_location = _shader->get_uniform_location("projectionMatrix");
            _camera_matrix_location = _shader->get_uniform_location("cameraMatrix");

            _width_location = _shader->get_uniform_location("imageWidth");
            _height_location = _shader->get_uniform_location("imageHeight");
            _min_delta_z_location = _shader->get_uniform_location("minDeltaZ");

            auto texture0_sampler_location = _shader->get_uniform_location("textureSampler");
            auto texture1_sampler_location = _shader->get_uniform_location("positionsSampler");
            auto texture2_sampler_location = _shader->get_uniform_location("uvsSampler");

            _shader->begin();
            _shader->load_uniform(_min_delta_z_location, 0.2f);
            _shader->load_uniform(texture0_sampler_location, texture_slot());
            _shader->load_uniform(texture1_sampler_location, geometry_slot());
            _shader->load_uniform(texture2_sampler_location, uvs_slot());
            _shader->end();
        }

        void pointcloud_shader::begin() { _shader->begin(); }
        void pointcloud_shader::end() { _shader->end(); }

        void pointcloud_shader::set_mvp(const matrix4& model,
            const matrix4& view,
            const matrix4& projection)
        {
            _shader->load_uniform(_transformation_matrix_location, model);
            _shader->load_uniform(_camera_matrix_location, view);
            _shader->load_uniform(_projection_matrix_location, projection);
        }

        void pointcloud_shader::set_image_size(int width, int height)
        {
            _shader->load_uniform(_width_location, (float)width);
            _shader->load_uniform(_height_location, (float)height);
        }

        void pointcloud_shader::set_min_delta_z(float min_delta_z)
        {
            _shader->load_uniform(_min_delta_z_location, min_delta_z);
        }

        void pointcloud_renderer::cleanup_gpu_resources()
        {
            _shader.reset();
            _model.reset();
            _vertex_texture.reset();
            _uvs_texture.reset();
        }

        pointcloud_renderer::~pointcloud_renderer()
        {
            perform_gl_action([&]()
            {
                cleanup_gpu_resources();
            });
        }

        void pointcloud_renderer::create_gpu_resources()
        {
            if (glsl_enabled())
            {
                _shader = std::make_shared<pointcloud_shader>();

                _vertex_texture = std::make_shared<rs2::texture_buffer>();
                _uvs_texture = std::make_shared<rs2::texture_buffer>();

                obj_mesh mesh = make_grid(_width, _height);
                _model = vao::create(mesh);
            }
        }

        pointcloud_renderer::pointcloud_renderer() 
            : stream_filter_processing_block("Pointcloud Renderer")
        {
            std::shared_ptr<option> opt = std::make_shared<librealsense::float_option>(option_range{ 0, 1, 0, 1 });
            register_option(OPTION_FILLED, opt);
            _filled_opt = &get_option(OPTION_FILLED);

            initialize();
        }

        rs2::frame pointcloud_renderer::process_frame(const rs2::frame_source& src, const rs2::frame& f)
        {
            //scoped_timer t("pointcloud_renderer");
            if (auto points = f.as<rs2::points>())
            {
                perform_gl_action([&]()
                {
                    //scoped_timer t("pointcloud_renderer.gl");

                    GLint curr_tex;
                    glGetIntegerv(GL_TEXTURE_BINDING_2D, &curr_tex);

                    auto vf_profile = f.get_profile().as<video_stream_profile>();
                    int width = vf_profile.width();
                    int height = vf_profile.height();
                    
                    if (glsl_enabled())
                    {
                        if (_width != width || _height != height)
                        {
                            obj_mesh mesh = make_grid(width, height);
                            _model = vao::create(mesh);

                            _width = width;
                            _height = height;
                        }

                        auto points_f = (frame_interface*)points.get();

                        uint32_t vertex_tex_id = 0;
                        uint32_t uv_tex_id = 0;

                        bool error = false;
                        
                        if (auto g = dynamic_cast<gpu_points_frame*>(points_f))
                        {
                            if (!g->get_gpu_section().input_texture(0, &vertex_tex_id) ||
                                !g->get_gpu_section().input_texture(1, &uv_tex_id)
                                ) error = true;
                        }
                        else
                        {
                            _vertex_texture->upload(points, RS2_FORMAT_XYZ32F);
                            vertex_tex_id = _vertex_texture->get_gl_handle();

                            _uvs_texture->upload(points, RS2_FORMAT_Y16);
                            uv_tex_id = _uvs_texture->get_gl_handle();
                        }

                        if (!error)
                        {
                            _shader->begin();
                            _shader->set_mvp(get_matrix(
                                RS2_GL_MATRIX_TRANSFORMATION),
                                get_matrix(RS2_GL_MATRIX_CAMERA),
                                get_matrix(RS2_GL_MATRIX_PROJECTION)
                            );
                            _shader->set_image_size(vf_profile.width(), vf_profile.height());

                            glActiveTexture(GL_TEXTURE0 + _shader->texture_slot());
                            glBindTexture(GL_TEXTURE_2D, curr_tex);

                            glActiveTexture(GL_TEXTURE0 + _shader->geometry_slot());
                            glBindTexture(GL_TEXTURE_2D, vertex_tex_id);

                            glActiveTexture(GL_TEXTURE0 + _shader->uvs_slot());
                            glBindTexture(GL_TEXTURE_2D, uv_tex_id);

                            if (_filled_opt->query() > 0.f) _model->draw();
                            else _model->draw_points();

                            glActiveTexture(GL_TEXTURE0 + _shader->texture_slot());

                            _shader->end();
                        }
                    }
                    else
                    {
                        glMatrixMode(GL_MODELVIEW);
                        glPushMatrix();

                        auto t = get_matrix(RS2_GL_MATRIX_TRANSFORMATION);
                        auto v = get_matrix(RS2_GL_MATRIX_CAMERA);

                        glLoadMatrixf(v * t);

                        auto vertices = points.get_vertices();
                        auto tex_coords = points.get_texture_coordinates();

                        glBindTexture(GL_TEXTURE_2D, curr_tex);

                        if (_filled_opt->query() > 0.f)
                        {
                            glBegin(GL_QUADS);

                            const auto threshold = 0.05f;
                            for (int x = 0; x < width - 1; ++x) {
                                for (int y = 0; y < height - 1; ++y) {
                                    auto a = y * width + x, b = y * width + x + 1, c = (y + 1)*width + x, d = (y + 1)*width + x + 1;
                                    if (vertices[a].z && vertices[b].z && vertices[c].z && vertices[d].z
                                        && abs(vertices[a].z - vertices[b].z) < threshold && abs(vertices[a].z - vertices[c].z) < threshold
                                        && abs(vertices[b].z - vertices[d].z) < threshold && abs(vertices[c].z - vertices[d].z) < threshold) {
                                        glVertex3fv(vertices[a]); glTexCoord2fv(tex_coords[a]);
                                        glVertex3fv(vertices[b]); glTexCoord2fv(tex_coords[b]);
                                        glVertex3fv(vertices[d]); glTexCoord2fv(tex_coords[d]);
                                        glVertex3fv(vertices[c]); glTexCoord2fv(tex_coords[c]);
                                    }
                                }
                            }
                            glEnd();
                        }
                        else
                        {
                            glBegin(GL_POINTS);
                            for (int i = 0; i < points.size(); i++)
                            {
                                if (vertices[i].z)
                                {
                                    glVertex3fv(vertices[i]);
                                    glTexCoord2fv(tex_coords[i + 1]);
                                }
                            }
                            glEnd();
                        }

                        glPopMatrix();
                    }
                }); 
            }

            return f;
        }
    }
}