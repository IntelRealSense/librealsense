#include "opengl3.h"

#include <glad/glad.h>
#include <assert.h>

using namespace rs2;

int vbo::convert_type(vbo_type type)
{
    switch (type) {
    case vbo_type::array_buffer: return GL_ARRAY_BUFFER;
    case vbo_type::element_array_buffer: return GL_ELEMENT_ARRAY_BUFFER;
    default: throw std::runtime_error("Not supported VBO type!");
    }
}

vbo::vbo(vbo_type type)
    : _type(type)
{
    glGenBuffers(1, &_id);
}

void vbo::bind()
{
    glBindBuffer(convert_type(_type), _id);
}

void vbo::unbind()
{
    glBindBuffer(convert_type(_type), 0);
}

void vbo::upload(int attribute, const float* xyz, int size, int count, bool dynamic)
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glBufferData(convert_type(_type), count * size * sizeof(float), xyz, 
        dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    glVertexAttribPointer(attribute, size, GL_FLOAT, GL_FALSE, 0, 0);
    _size = count;
    unbind();
}

void vbo::upload(const int3* indx, int count)
{
    assert(_type == vbo_type::element_array_buffer);
    bind();
    glBufferData(convert_type(_type), count * sizeof(int3), indx, GL_STATIC_DRAW);
    _size = count;
}

void vbo::draw_points()
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glDrawArrays(GL_POINTS, 0, _size);
    unbind();
}

void vbo::draw_triangles()
{
    assert(_type == vbo_type::array_buffer);
    bind();
    glDrawArrays(GL_TRIANGLES, 0, _size);
    unbind();
}

void vbo::draw_indexed_triangles()
{
    assert(_type == vbo_type::element_array_buffer);
    glDrawElements(GL_TRIANGLES, _size * (sizeof(int3) / sizeof(int)), GL_UNSIGNED_INT, 0);
}

vbo::vbo(vbo&& other)
    : _id(other._id), _type(other._type), _size(other._size)
{
    other._id = 0;
}

vbo::~vbo()
{
    if (_id) glDeleteBuffers(1, &_id);
}

vao::vao(const float3* vert, const float2* uvs, const float3* normals,
    const float3* tangents, int vert_count, const int3* indx, int indx_count)
    : _vertexes(vbo_type::array_buffer),
    _uvs(vbo_type::array_buffer),
    _indexes(vbo_type::element_array_buffer),
    _tangents(vbo_type::array_buffer),
    _vertex_count(vert_count)
{
    glGenVertexArrays(1, &_id);
    bind();
    _indexes.upload(indx, indx_count);
    _vertexes.upload(0, (float*)vert, 3, vert_count, true);
    if (normals) _normals.upload(2, (float*)normals, 3, vert_count);
    if (tangents) _tangents.upload(3, (float*)tangents, 3, vert_count);
    if (uvs) _uvs.upload(1, (float*)uvs, 2, vert_count);
    unbind();
}

std::unique_ptr<vao> vao::create(const obj_mesh& mesh)
{
    return std::unique_ptr<vao>(new vao(mesh.positions.data(),
        mesh.uvs.data(),
        mesh.normals.data(),
        mesh.tangents.data(),
        mesh.positions.size(),
        mesh.indexes.data(),
        mesh.indexes.size()));
}

vao::vao(vao&& other)
    : _id(other._id),
    _indexes(std::move(other._indexes)),
    _vertexes(std::move(other._vertexes)),
    _uvs(std::move(other._uvs)),
    _normals(std::move(other._normals)),
    _tangents(std::move(other._tangents))
{
    other._id = 0;
}

vao::~vao()
{
    if (_id) glDeleteVertexArrays(1, &_id);
}

void vao::bind()
{
    glBindVertexArray(_id);
}

void vao::unbind()
{
    glBindVertexArray(0);
}

void vao::draw_points()
{
    bind();

    glEnableVertexAttribArray(0); // vertex
    if (_uvs.size())        glEnableVertexAttribArray(1); // uv
    if (_normals.size())    glEnableVertexAttribArray(2); // normals
    if (_tangents.size())   glEnableVertexAttribArray(3); // tangents

    _vertexes.draw_points();

    glDisableVertexAttribArray(0);
    if (_uvs.size())        glDisableVertexAttribArray(1);
    if (_normals.size())    glDisableVertexAttribArray(2);
    if (_tangents.size())   glDisableVertexAttribArray(3);

    unbind();
}

void vao::draw()
{
    bind();

    glEnableVertexAttribArray(0); // vertex
    if (_uvs.size())        glEnableVertexAttribArray(1); // uv
    if (_normals.size())    glEnableVertexAttribArray(2); // normals
    if (_tangents.size())   glEnableVertexAttribArray(3); // tangents

    _indexes.draw_indexed_triangles();

    glDisableVertexAttribArray(0);
    if (_uvs.size())        glDisableVertexAttribArray(1);
    if (_normals.size())    glDisableVertexAttribArray(2);
    if (_tangents.size())   glDisableVertexAttribArray(3);

    unbind();
}

void vao::update_positions(const float3* vert)
{
    _vertexes.upload(0, (float*)vert, 3, _vertex_count, true);
}

static const char* vertex_shader_text =
"#version 110\n"
"attribute vec3 position;\n"
"attribute vec2 textureCoords;\n"
"varying vec2 textCoords;\n"
"uniform vec2 elementPosition;\n"
"uniform vec2 elementScale;\n"
"void main(void)\n"
"{\n"
"    gl_Position = vec4(position * vec3(elementScale, 1.0) + vec3(elementPosition, 0.0), 1.0);\n"
"    textCoords = textureCoords;\n"
"}";

static const char* splash_shader_text =
"#version 110\n"
"varying vec2 textCoords;\n"
"uniform sampler2D textureSampler;\n"
"uniform float opacity;\n"
"uniform vec2 rayOrigin;\n"
"uniform float power;\n"
"void main(void) {\n"
"    vec4 FragColor = texture2D(textureSampler, textCoords);\n"
"        int samples = 120;\n"
"        vec2 delta = vec2(textCoords - rayOrigin);\n"
"        delta *= 1.0 /  float(samples) * 0.99;"
"        vec2 coord = textCoords;\n"
"        float illuminationDecay = power;\n"
"        for(int i=0; i < samples ; i++)\n"
"        {\n"
"           coord -= delta;\n"
"           vec4 texel = texture2D(textureSampler, coord);\n"
"           texel *= illuminationDecay * 0.4;\n"
"           texel.x *= 80.0 / 255.0;\n"
"           texel.y *= 99.0 / 255.0;\n"
"           texel.z *= 115.0 / 255.0;\n"
"           FragColor += texel;\n"
"           illuminationDecay *= power;\n"
"        }\n"
"        FragColor = clamp(FragColor, 0.0, 1.0);\n"
"    gl_FragColor = vec4(FragColor.xyz, opacity);\n"
"}";

static const char* fragment_shader_text =
"#version 110\n"
"varying vec2 textCoords;\n"
"uniform sampler2D textureSampler;\n"
"uniform float opacity;\n"
"void main(void) {\n"
"    vec2 tex = vec2(textCoords.x, 1.0 - textCoords.y);\n"
"    vec4 color = texture2D(textureSampler, tex);\n"
"    gl_FragColor = vec4(color.xyz, opacity);\n"
"}";

using namespace rs2;

texture_2d_shader::texture_2d_shader(std::unique_ptr<shader_program> shader)
    : _shader(std::move(shader))
{
    init();
}

const char* texture_2d_shader::default_vertex_shader()
{
    return vertex_shader_text;
}

texture_2d_shader::texture_2d_shader()
{
    _shader = shader_program::load(
        vertex_shader_text,
        fragment_shader_text, "position", "textureCoords");

    init();
}

void visualizer_2d::draw_texture(uint32_t tex, float opacity)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    tex_2d_shader->begin();
    tex_2d_shader->set_opacity(opacity);
    tex_2d_shader->end();
    draw_texture({ 0.f, 0.f }, { 1.f, 1.f }, tex);
    glDisable(GL_BLEND);
}

void texture_2d_shader::set_position_and_scale(
    const float2& position,
    const float2& scale)
{
    _shader->load_uniform(_position_location, position);
    _shader->load_uniform(_scale_location, scale);
}

void splash_screen_shader::set_ray_center(const float2& center)
{
    _shader->load_uniform(_rays_location, center);
}

void splash_screen_shader::set_power(float power)
{
    _shader->load_uniform(_power_location, power);
}

void texture_2d_shader::init()
{
    _position_location = _shader->get_uniform_location("elementPosition");
    _scale_location = _shader->get_uniform_location("elementScale");
    _opacity_location = _shader->get_uniform_location("opacity");

    auto texture0_sampler_location = _shader->get_uniform_location("textureSampler");

    _shader->begin();
    _shader->load_uniform(texture0_sampler_location, 0);
    set_opacity(1.f);
    _shader->end();
}

splash_screen_shader::splash_screen_shader()
    : texture_2d_shader(shader_program::load(
        vertex_shader_text,
        splash_shader_text, "position", "textureCoords"))
{

    _rays_location = _shader->get_uniform_location("rayOrigin");
    _power_location = _shader->get_uniform_location("power");
}

void texture_2d_shader::begin() { _shader->begin(); }
void texture_2d_shader::end() { _shader->end(); }

void texture_2d_shader::set_opacity(float opacity)
{
    _shader->load_uniform(_opacity_location, opacity);
}

void texture_visualizer::draw(texture_2d_shader& shader, uint32_t tex)
{
    shader.begin();
    shader.set_position_and_scale(_position, _scale);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    _geometry->draw();
    glBindTexture(GL_TEXTURE_2D, 0);
    shader.end();
}

obj_mesh texture_visualizer::create_mesh()
{
    obj_mesh res;

    res.positions.reserve(4);
    res.positions.emplace_back(float3{ -1.f, -1.f, 0.f });
    res.positions.emplace_back(float3{ 1.f, -1.f, 0.f });
    res.positions.emplace_back(float3{ 1.f, 1.f, 0.f });
    res.positions.emplace_back(float3{ -1.f, 1.f, 0.f });

    res.uvs.reserve(4);
    res.uvs.emplace_back(float2{ 0.f, 1.f });
    res.uvs.emplace_back(float2{ 1.f, 1.f });
    res.uvs.emplace_back(float2{ 1.f, 0.f });
    res.uvs.emplace_back(float2{ 0.f, 0.f });

    res.indexes.reserve(2);
    res.indexes.emplace_back(int3{ 0, 1, 2 });
    res.indexes.emplace_back(int3{ 2, 3, 0 });

    return res;
}

#include <iostream>

using namespace rs2;

int shader_program::get_uniform_location(const std::string& name)
{
    return glGetUniformLocation(_id, name.c_str());
}

void shader_program::load_uniform(int location, int value)
{
    glUniform1i(location, value);
}

void shader_program::load_uniform(int location, float value)
{
    glUniform1f(location, value);
}

void shader_program::load_uniform(int location, bool value)
{
    load_uniform(location, value ? 1.f : 0.f);
}

void shader_program::load_uniform(int location, const float3& vec)
{
    glUniform3f(location, vec.x, vec.y, vec.z);
}

void shader_program::load_uniform(int location, const float2& vec)
{
    glUniform2f(location, vec.x, vec.y);
}

void shader_program::load_uniform(int location, const matrix4& matrix)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, (float*)&matrix);
}

void shader_program::bind_attribute(int attr, const std::string& name)
{
    glBindAttribLocation(_id, attr, name.c_str());
}

shader::shader(const std::string& shader_code, shader_type type)
{
    auto lambda = [&]() {
        switch (type)
        {
        case shader_type::vertex: return GL_VERTEX_SHADER;
        case shader_type::fragment: return GL_FRAGMENT_SHADER;
        default:
            throw std::runtime_error("Unknown shader type!");
        }
    };
    const auto gl_type = lambda();

    GLuint shader_id = glCreateShader(gl_type);

    char const * source_ptr = shader_code.c_str();
    int length = shader_code.size();
    glShaderSource(shader_id, 1, &source_ptr, &length);

    glCompileShader(shader_id);

    GLint result;
    int log_length;

    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)) {
        std::vector<char> error_message(log_length + 1);
        glGetShaderInfoLog(shader_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        std::cerr << error;
        glDeleteShader(shader_id);
        throw std::runtime_error(error);
    }

    _id = shader_id;
}

shader::~shader()
{
    glDeleteShader(_id);
}

shader_program::shader_program()
{
    GLuint program_id = glCreateProgram();
    _id = program_id;
}
shader_program::~shader_program()
{
    glUseProgram(0);
    glDeleteProgram(_id);
}

void shader_program::attach(const shader& shader)
{
    _shaders.push_back(&shader);
}
void shader_program::link()
{
    for (auto ps : _shaders)
    {
        glAttachShader(_id, ps->get_id());
    }

    glLinkProgram(_id);

    GLint result;
    int log_length;

    glGetProgramiv(_id, GL_LINK_STATUS, &result);
    glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)) {
        std::vector<char> error_message(log_length + 1);
        glGetProgramInfoLog(_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        std::cerr << error;
        for (auto ps : _shaders)
        {
            glDetachShader(_id, ps->get_id());
        }
        throw std::runtime_error(error);
    }

    glValidateProgram(_id);

    glGetProgramiv(_id, GL_VALIDATE_STATUS, &result);
    glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)) {
        std::vector<char> error_message(log_length + 1);
        glGetProgramInfoLog(_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        std::cerr << error;
        for (auto ps : _shaders)
        {
            glDetachShader(_id, ps->get_id());
        }
        throw std::runtime_error(error);
    }

    for (auto ps : _shaders)
    {
        glDetachShader(_id, ps->get_id());
    }
    _shaders.clear();
}

void shader_program::begin() const
{
    glUseProgram(_id);
}
void shader_program::end() const
{
    glUseProgram(0);
}

std::unique_ptr<shader_program> shader_program::load(
    const std::string& vertex_shader,
    const std::string& fragment_shader,
    const char* input0,
    const char* input1,
    const char* output0,
    const char* output1)
{
    std::unique_ptr<shader_program> res(new shader_program());
    shader vertex(vertex_shader, shader_type::vertex);
    shader fragment(fragment_shader, shader_type::fragment);
    res->attach(vertex);
    res->attach(fragment);

    if (input0) glBindAttribLocation(res->get_id(), 0, input0);
    if (input1) glBindAttribLocation(res->get_id(), 1, input1);

    if (output0) glBindFragDataLocation(res->get_id(), 0, output0);
    if (output1) glBindFragDataLocation(res->get_id(), 1, output1);

    res->link();
    return std::move(res);
}

fbo::fbo(int w, int h) : _w(w), _h(h)
{
    glGenFramebuffers(1, &_id);
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void fbo::createTextureAttachment(uint32_t handle)
{
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _w, _h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, handle, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void fbo::createDepthTextureAttachment(uint32_t handle)
{
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, _w, _h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, handle, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void fbo::bind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glViewport(0, 0, _w, _h);
}

void fbo::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void fbo::createDepthBufferAttachment()
{
    if (_db) glDeleteRenderbuffers(1, &_db);
    glGenRenderbuffers(1, &_db);
    glBindRenderbuffer(GL_RENDERBUFFER, _db);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _w, _h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _db);
}

fbo::~fbo()
{
    glDeleteRenderbuffers(1, &_db);
    glDeleteFramebuffers(1, &_id);
}

std::string fbo::get_status()
{
    std::string res = "UNKNOWN";

    bind();
    auto s = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (s == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) res = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    else if (s == 0x8CD9) res = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
    else if (s == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) res = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    else if (s == GL_FRAMEBUFFER_UNSUPPORTED) res = "GL_FRAMEBUFFER_UNSUPPORTED";
    else if (s == GL_FRAMEBUFFER_COMPLETE) res = "GL_FRAMEBUFFER_COMPLETE";

    unbind();

    return res;
}
