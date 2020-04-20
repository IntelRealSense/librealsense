// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/rsutil.h"

#include "synthetic-stream-gl.h"
#include "environment.h"
#include "proc/occlusion-filter.h"
#include "pointcloud-gl.h"
#include "option.h"
#include "environment.h"
#include "context.h"

#include <iostream>
#include <chrono>

#include "opengl3.h"

using namespace rs2;
using namespace librealsense;
using namespace librealsense::gl;

static const char* project_fragment_text =
"#version 130\n"
"in vec2 textCoords;\n"
"out vec4 output_xyz;\n"
"out vec4 output_uv;\n"
"uniform sampler2D textureSampler;\n"
"uniform float opacity;\n"
"uniform mat4 extrinsics;\n"
""
"uniform vec2 focal1;\n"
"uniform vec2 principal1;\n"
"uniform float is_bc1;\n"
"uniform float coeffs1[5];\n"
""
"uniform vec2 focal2;\n"
"uniform vec2 principal2;\n"
"uniform float is_bc2;\n"
"uniform float coeffs2[5];\n"
""
"uniform float depth_scale;\n"
"uniform float width1;\n"
"uniform float height1;\n"
"uniform float width2;\n"
"uniform float height2;\n"
"\n"
"uniform float needs_projection;\n"
"\n"
"void main(void) {\n"
"    float px = textCoords.x * width1;\n"
"    float py = (1.0 - textCoords.y) * height1;\n"
"    float x = (px - principal1.x) / focal1.x;\n"
"    float y = (py - principal1.y) / focal1.y;\n"
"    if(is_bc1 > 0.0)\n"
"    {\n"
"        float r2  = x*x + y*y;\n"
"        float f = 1.0 + coeffs1[0]*r2 + coeffs1[1]*r2*r2 + coeffs1[4]*r2*r2*r2;\n"
"        float ux = x*f + 2.0*coeffs1[2]*x*y + coeffs1[3]*(r2 + 2.0*x*x);\n"
"        float uy = y*f + 2.0*coeffs1[3]*x*y + coeffs1[2]*(r2 + 2.0*y*y);\n"
"        x = ux;\n"
"        y = uy;\n"
"    }\n"
"    vec2 tex = vec2(textCoords.x, 1.0 - textCoords.y);\n"
"    vec4 dp = texture(textureSampler, tex);\n"
"    float nd = (dp.x + dp.y * 256.0) * 256.0;\n"
"    float depth = depth_scale * nd;\n"
"    vec4 xyz = vec4(x * depth, y * depth, depth, 1.0);\n"
"    output_xyz = xyz;\n"
""
"    if (needs_projection > 0) {"
"    vec4 trans = extrinsics * xyz;\n"
"    x = trans.x / trans.z;\n"
"    y = trans.y / trans.z;\n"
"\n"
"    if(is_bc2 > 0.0)\n"
"    {\n"
"        float r2  = x*x + y*y;\n"
"        float f = 1.0 + coeffs2[0]*r2 + coeffs2[1]*r2*r2 + coeffs2[4]*r2*r2*r2;\n"
"        x *= f;\n"
"        y *= f;\n"
"        float dx = x + 2.0*coeffs2[2]*x*y + coeffs2[3]*(r2 + 2.0*x*x);\n"
"        float dy = y + 2.0*coeffs2[3]*x*y + coeffs2[2]*(r2 + 2.0*y*y);\n"
"        x = dx;\n"
"        y = dy;\n"
"    }\n"
"    // TODO: Enable F-Thetha\n"
"    //if (intrin->model == RS2_DISTORTION_FTHETA)\n"
"    //{\n"
"    //    float r = sqrtf(x*x + y*y);\n"
"    //    float rd = (float)(1.0f / intrin->coeffs[0] * atan(2 * r* tan(intrin->coeffs[0] / 2.0f)));\n"
"    //    x *= rd / r;\n"
"    //    y *= rd / r;\n"
"    //}\n"
"\n"
"    float u = (x * focal2.x + principal2.x) / width2;\n"
"    float v = (y * focal2.y + principal2.y) / height2;\n"
"    output_uv = vec4(u, v, 0.0, 1.0);\n"
"    } else {\n"
"       output_uv = vec4(textCoords.x, 1.0 - textCoords.y, 0.0, 1.0);\n"
"    }\n"
"}";

class project_shader : public texture_2d_shader
{
public:
    project_shader()
        : texture_2d_shader(shader_program::load(
            texture_2d_shader::default_vertex_shader(), 
            project_fragment_text, 
            "position", "textureCoords", 
            "output_xyz", "output_uv"))
    {
        _focal_location[0] = _shader->get_uniform_location("focal1");
        _principal_location[0] = _shader->get_uniform_location("principal1");
        _is_bc_location[0] = _shader->get_uniform_location("is_bc1");
        _coeffs_location[0] = _shader->get_uniform_location("coeffs1");

        _focal_location[1] = _shader->get_uniform_location("focal2");
        _principal_location[1] = _shader->get_uniform_location("principal2");
        _is_bc_location[1] = _shader->get_uniform_location("is_bc2");
        _coeffs_location[1] = _shader->get_uniform_location("coeffs2");

        _depth_scale_location = _shader->get_uniform_location("depth_scale");
        _width_location[0] = _shader->get_uniform_location("width1");
        _height_location[0] = _shader->get_uniform_location("height1");
        _width_location[1] = _shader->get_uniform_location("width2");
        _height_location[1] = _shader->get_uniform_location("height2");
        _extrinsics_location = _shader->get_uniform_location("extrinsics");

        _requires_projection_location = _shader->get_uniform_location("needs_projection");
    }

    void requires_projection(bool val)
    {
        _shader->load_uniform(_requires_projection_location, val ? 1.f : 0.f);
    }

    void set_size(int id, int w, int h)
    {
        _shader->load_uniform(_width_location[id], (float)w);
        _shader->load_uniform(_height_location[id], (float)h);
    }

    void set_intrinsics(int idx, const rs2_intrinsics& intr)
    {
        rs2::float2 focal{ intr.fx, intr.fy };
        rs2::float2 principal{ intr.ppx, intr.ppy };
        float is_bc = (intr.model == RS2_DISTORTION_INVERSE_BROWN_CONRADY ? 1.f : 0.f);
        _shader->load_uniform(_focal_location[idx], focal);
        _shader->load_uniform(_principal_location[idx], principal);
        _shader->load_uniform(_is_bc_location[idx], is_bc);
        glUniform1fv(_coeffs_location[idx], 5, intr.coeffs);
    }

    void set_depth_scale(float depth_scale)
    {
        _shader->load_uniform(_depth_scale_location, depth_scale);
    }

    void set_extrinsics(const rs2_extrinsics& extr)
    {
        rs2::matrix4 m;
        for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            if (i < 3 && j < 3) m.mat[i][j] = extr.rotation[i * 3 + j];
            else if (i == 3 && j < 3) m.mat[i][j] = extr.translation[j];
            else if (i < 3 && j == 3) m.mat[i][j] = 0.f;
            else m.mat[i][j] = 1.f;
        }
        _shader->load_uniform(_extrinsics_location, m);
    }
private:
    uint32_t _focal_location[2];
    uint32_t _principal_location[2];
    uint32_t _is_bc_location[2];
    uint32_t _coeffs_location[2];
    uint32_t _depth_scale_location;

    uint32_t _width_location[2];
    uint32_t _height_location[2];

    uint32_t _extrinsics_location;

    uint32_t _requires_projection_location;
};

void pointcloud_gl::cleanup_gpu_resources()
{
    _projection_renderer.reset();
    _enabled = 0;
}
void pointcloud_gl::create_gpu_resources()
{
    if (glsl_enabled())
    {
        _projection_renderer = std::make_shared<visualizer_2d>(std::make_shared<project_shader>());
    }
    _enabled = glsl_enabled() ? 1 : 0;
}

pointcloud_gl::~pointcloud_gl()
{
    perform_gl_action([&]()
    {
        cleanup_gpu_resources();
    }, []{});
}

pointcloud_gl::pointcloud_gl()
    : pointcloud("Pointcloud (GLSL)"), _depth_data(rs2::frame{})
{
    _source.add_extension<gl::gpu_points_frame>(RS2_EXTENSION_VIDEO_FRAME_GL);

    auto opt = std::make_shared<librealsense::ptr_option<int>>(
        0, 1, 0, 1, &_enabled, "GLSL enabled"); 
    register_option(RS2_OPTION_COUNT, opt);

    initialize();
}

const librealsense::float3* pointcloud_gl::depth_to_points(
        rs2::points output,
        const rs2_intrinsics &depth_intrinsics, 
        const rs2::depth_frame& depth_frame,
        float depth_scale)
{
    perform_gl_action([&]{
        _depth_data = depth_frame;
        _depth_scale = depth_scale;
        _depth_intr = depth_intrinsics;
    }, [&]{
        _enabled = false;
    });
    return nullptr;
}

void pointcloud_gl::get_texture_map(
    rs2::points output,
    const librealsense::float3* points,
    const unsigned int width,
    const unsigned int height,
    const rs2_intrinsics &other_intrinsics,
    const rs2_extrinsics& extr,
    librealsense::float2* pixels_ptr)
{
    perform_gl_action([&]{
        auto viz = _projection_renderer;
        auto frame_ref = (frame_interface*)output.get();

        auto gf = dynamic_cast<gpu_addon_interface*>(frame_ref);

        uint32_t depth_texture;

        if (auto input_frame = _depth_data.as<rs2::gl::gpu_frame>())
        {
            depth_texture = input_frame.get_texture_id(0);
        }
        else
        {
            glGenTextures(1, &depth_texture);
            glBindTexture(GL_TEXTURE_2D, depth_texture);
            auto depth_data = _depth_data.get_data();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, depth_data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        fbo fbo(width, height);

        uint32_t output_xyz;
        gf->get_gpu_section().output_texture(0, &output_xyz, TEXTYPE_XYZ);
        glBindTexture(GL_TEXTURE_2D, output_xyz);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_xyz, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        uint32_t output_uv;
        gf->get_gpu_section().output_texture(1, &output_uv, TEXTYPE_UV);
        glBindTexture(GL_TEXTURE_2D, output_uv);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, output_uv, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        gf->get_gpu_section().set_size(width, height);

        fbo.bind();
        
        GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        auto& shader = (project_shader&)viz->get_shader();
        shader.begin();

        shader.requires_projection(!(_depth_intr == other_intrinsics && 
            extr == identity_matrix()));

        shader.set_depth_scale(_depth_scale);
        shader.set_intrinsics(0, _depth_intr);
        shader.set_intrinsics(1, other_intrinsics);
        shader.set_extrinsics(extr);
        shader.set_size(0, width, height);
        shader.set_size(1, other_intrinsics.width, other_intrinsics.height);
        
        viz->draw_texture(depth_texture);
        shader.end();

        fbo.unbind();

        glBindTexture(GL_TEXTURE_2D, 0);
        if (!_depth_data.is<rs2::gl::gpu_frame>())
        {
            glDeleteTextures(1, &depth_texture);
        }
    }, [&]{
        _enabled = false;
    });
}

rs2::points pointcloud_gl::allocate_points(
    const rs2::frame_source& source, 
    const rs2::frame& f)
{
    auto prof = std::dynamic_pointer_cast<librealsense::stream_profile_interface>(
        _output_stream.get()->profile->shared_from_this());
    auto frame_ref = _source_wrapper.allocate_points(prof, (frame_interface*)f.get(),
        RS2_EXTENSION_VIDEO_FRAME_GL);
    rs2::frame res { (rs2_frame*)frame_ref };
    return res.as<rs2::points>();
}
