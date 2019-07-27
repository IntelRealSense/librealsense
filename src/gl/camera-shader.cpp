// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "camera-shader.h"
#include <glad/glad.h>

#include "option.h"

using namespace rs2;

struct short3
{
    uint16_t x, y, z;
};

#include <res/d435.h>
#include <res/d415.h>
#include <res/sr300.h>
#include <res/t265.h>

static const char* vertex_shader_text =
"#version 110\n"
"\n"
"attribute vec3 position;\n"
"uniform mat4 transformationMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat4 cameraMatrix;\n"
"\n"
"void main(void) {\n"
"    vec4 worldPosition = transformationMatrix * vec4(position.xyz, 1.0);\n"
"    gl_Position = projectionMatrix * cameraMatrix * worldPosition;\n"
"}\n";

static const char* fragment_shader_text =
"#version 110\n"
"\n"
"void main(void) {\n"
"    gl_FragColor = vec4(36.0 / 1000.0, 44.0 / 1000.0, 51.0 / 1000.0, 0.3);\n"
"}\n";

using namespace rs2;

namespace librealsense
{
    namespace gl
    {
        camera_shader::camera_shader()
        {
            _shader = shader_program::load(
                vertex_shader_text,
                fragment_shader_text);

            init();
        }

        void camera_shader::init()
        {
            _shader->bind_attribute(0, "position");

            _transformation_matrix_location = _shader->get_uniform_location("transformationMatrix");
            _projection_matrix_location = _shader->get_uniform_location("projectionMatrix");
            _camera_matrix_location = _shader->get_uniform_location("cameraMatrix");
        }

        void camera_shader::begin() { _shader->begin(); }
        void camera_shader::end() { _shader->end(); }

        void camera_shader::set_mvp(const matrix4& model,
            const matrix4& view,
            const matrix4& projection)
        {
            _shader->load_uniform(_transformation_matrix_location, model);
            _shader->load_uniform(_camera_matrix_location, view);
            _shader->load_uniform(_projection_matrix_location, projection);
        }

        void camera_renderer::cleanup_gpu_resources()
        {
            _shader.reset();
            _camera_model.clear();

            glDeleteTextures(1, &color_tex);
            glDeleteTextures(1, &depth_tex);

            _viz.reset();
            _blit.reset();
            _fbo.reset();
        }

        void camera_renderer::create_gpu_resources()
        {
            if (glsl_enabled())
            {
                _shader = std::make_shared<camera_shader>(); 

                for (auto&& mesh : camera_mesh)
                {
                    _camera_model.push_back(vao::create(mesh));
                }

                _fbo = std::make_shared<fbo>(1, 1);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                _viz = std::make_shared<rs2::texture_visualizer>();
                _blit = std::make_shared<blit_shader>();

                glGenTextures(1, &color_tex);
                glGenTextures(1, &depth_tex);
            }
        }

        camera_renderer::~camera_renderer()
        {
            perform_gl_action([&]()
            {
                cleanup_gpu_resources();
            });
        }

        typedef void (*load_function)(std::vector<rs2::float3>&, 
            std::vector<rs2::float3>&, std::vector<short3>&);

        obj_mesh load_model(load_function f)
        {
            obj_mesh res;
            std::vector<short3> idx;
            f(res.positions, res.normals, idx);
            for (auto i : idx)
                res.indexes.push_back({ i.x, i.y, i.z });
            return res;
        }

        camera_renderer::camera_renderer() : stream_filter_processing_block("Camera Model Renderer")
        {
            camera_mesh.push_back(load_model(uncompress_d415_obj));
            camera_mesh.push_back(load_model(uncompress_d435_obj));
            camera_mesh.push_back(load_model(uncompress_sr300_obj));
            camera_mesh.push_back(load_model(uncompress_t265_obj));

            for (auto&& mesh : camera_mesh)
            {
                for (auto& xyz : mesh.positions)
                {
                    xyz = xyz / 1000.f;
                    xyz.x *= -1;
                    xyz.y *= -1;
                }
            }

            register_option(OPTION_SELECTED, std::make_shared<librealsense::float_option>(option_range{ 0, 1, 0, 1 }));
            _selected_opt = &get_option(OPTION_SELECTED);

            register_option(OPTION_MOUSE_PICK, std::make_shared<librealsense::float_option>(option_range{ 0, 1, 0, 1 }));
            _mouse_pick_opt = &get_option(OPTION_MOUSE_PICK);

            register_option(OPTION_WAS_PICKED, std::make_shared<librealsense::float_option>(option_range{ 0, 1, 0, 1 }));
            _was_picked_opt = &get_option(OPTION_WAS_PICKED);

            register_option(OPTION_MOUSE_X, std::make_shared<librealsense::float_option>(option_range{ 0, 10000, 0, 0 }));
            _mouse_x_opt = &get_option(OPTION_MOUSE_X);

            register_option(OPTION_MOUSE_Y, std::make_shared<librealsense::float_option>(option_range{ 0, 10000, 0, 0 }));
            _mouse_y_opt = &get_option(OPTION_MOUSE_Y);

            initialize();
        }

        bool starts_with(const std::string& s, const std::string& prefix)
        {
            auto i = s.begin(), j = prefix.begin();
            for (; i != s.end() && j != prefix.end() && *i == *j;
                i++, j++);
            return j == prefix.end();
        }

        rs2::frame camera_renderer::process_frame(const rs2::frame_source& src, const rs2::frame& f)
        {
            //scoped_timer t("camera_renderer");

            const auto& dev = ((frame_interface*)f.get())->get_sensor()->get_device();

            int index = -1;

            if (dev.supports_info(RS2_CAMERA_INFO_NAME))
            {
                auto dev_name = dev.get_info(RS2_CAMERA_INFO_NAME);
                if (starts_with(dev_name, "Intel RealSense D415")) index = 0;
                if (starts_with(dev_name, "Intel RealSense D435")) index = 1;
                if (starts_with(dev_name, "Intel RealSense SR300")) index = 2;
                if (starts_with(dev_name, "Intel RealSense T26")) index = 3;
            };

            if (index >= 0)
            {
                perform_gl_action([&]()
                {
                    //scoped_timer t("camera_renderer.gl");

                    if (glsl_enabled())
                    {
                        int32_t vp[4];
                        glGetIntegerv(GL_VIEWPORT, vp);
                        check_gl_error();

                        _fbo->set_dims(vp[2], vp[3]);

                        glBindFramebuffer(GL_FRAMEBUFFER, _fbo->get());
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);

                        _fbo->createTextureAttachment(color_tex);
                        _fbo->createDepthTextureAttachment(depth_tex);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        _fbo->bind();

                        glDrawBuffer(GL_COLOR_ATTACHMENT0);

                        glClearColor(0, 0, 0, 0);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        _shader->begin();
                        _shader->set_mvp(get_matrix(RS2_GL_MATRIX_TRANSFORMATION), 
                            get_matrix(RS2_GL_MATRIX_CAMERA), 
                            get_matrix(RS2_GL_MATRIX_PROJECTION)
                        );
                        _camera_model[index]->draw();
                        _shader->end();

                        _shader->end();

                        _fbo->unbind();

                        _was_picked_opt->set(0.f);

                        if (_mouse_pick_opt->query() > 0.f)
                        {
                            auto x = _mouse_x_opt->query() - vp[0];
                            auto y = vp[3] + vp[1] - _mouse_y_opt->query();

                            glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo->get());
                            glReadBuffer(GL_COLOR_ATTACHMENT0);

                            uint8_t rgba[4];
                            glReadPixels(x, y, 1, 1, GL_RGBA, GL_BYTE, &rgba);

                            if (rgba[3] > 0)
                            {
                                _was_picked_opt->set(1.f);
                            }

                            glReadBuffer(GL_NONE);
                            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

                            _mouse_pick_opt->set(0.f);
                        }

                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, depth_tex);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, color_tex);

                        _blit->begin();
                        _blit->set_image_size(vp[2], vp[3]);
                        _blit->set_selected(_selected_opt->query() > 0.f);
                        _blit->end();

                        _viz->draw(*_blit, color_tex);

                        glActiveTexture(GL_TEXTURE0);
                    }
                    else
                    {
                        glMatrixMode(GL_MODELVIEW);
                        glPushMatrix();

                        auto t = get_matrix(RS2_GL_MATRIX_TRANSFORMATION);
                        auto v = get_matrix(RS2_GL_MATRIX_CAMERA);

                        glLoadMatrixf(v * t);

                        glBegin(GL_TRIANGLES);
                        auto& mesh = camera_mesh[index];
                        for (auto& i : mesh.indexes)
                        {
                            auto v0 = mesh.positions[i.x];
                            auto v1 = mesh.positions[i.y];
                            auto v2 = mesh.positions[i.z];
                            glVertex3fv(&v0.x);
                            glVertex3fv(&v1.x);
                            glVertex3fv(&v2.x);
                            glColor4f(0.036f, 0.044f, 0.051f, 0.3f);
                        }
                        glEnd();

                        glPopMatrix();
                    }
                }); 
            }

            return f;
        }
    }
}