#pragma once

#include "rendering.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace rs2
{
    enum class shader_type
    {
        vertex,
        fragment
    };

    struct int3
    {
        int x, y, z;
    };

    struct obj_mesh
    {
        std::string         name;
        std::vector<int3>   indexes;
        std::vector<float3> positions;
        std::vector<float3> normals;
        std::vector<float2> uvs;
        std::vector<float3> tangents;
    };

    class shader
    {
    public:
        shader(const std::string& code, shader_type type);
        ~shader();

        unsigned int get_id() const { return _id; }

    private:
        unsigned int _id;
    };

    class shader_program
    {
    public:
        shader_program();
        ~shader_program();

        void attach(const shader& shader);
        void link();

        void begin() const;
        void end() const;

        static std::unique_ptr<shader_program> load(
            const std::string& vertex_shader,
            const std::string& fragment_shader,
            const char* input0 = nullptr,
            const char* input1 = nullptr,
            const char* output0 = nullptr,
            const char* output1 = nullptr);

        unsigned int get_id() const { return _id; }

        int get_uniform_location(const std::string& name);
        void load_uniform(int location, float value);
        void load_uniform(int location, const float2& vec);
        void load_uniform(int location, const float3& vec);
        void load_uniform(int location, bool value);
        void load_uniform(int location, int value);
        void load_uniform(int location, const matrix4& matrix);

        void bind_attribute(int attr, const std::string& name);

    private:
        std::vector<const shader*> _shaders;
        unsigned int _id;
    };

    enum class vbo_type
    {
        array_buffer,
        element_array_buffer,
    };

    class vbo
    {
    public:
        vbo(vbo_type type = vbo_type::array_buffer);
        vbo(vbo&& other);
        ~vbo();

        void upload(int attribute, const float* xyz, int size, int count, bool dynamic = false);
        void upload(const int3* indx, int count);

        void draw_points();
        void draw_triangles();
        void draw_indexed_triangles();

        void bind();
        void unbind();

        uint32_t size() const { return _size; }
    private:
        vbo(const vbo& other) = delete;
        static int convert_type(vbo_type type);

        uint32_t _id;
        uint32_t _size = 0;
        vbo_type _type;
    };

    struct float3;
    struct float2;
    struct int3;

    struct obj_mesh;

    class vao
    {
    public:
        static std::unique_ptr<vao> create(const obj_mesh& m);

        vao(const float3* vert, const float2* uvs, const float3* normals,
            const float3* tangents, int vert_count, const int3* indx, int indx_count);
        ~vao();
        void bind();
        void unbind();
        void draw();
        void draw_points();

        // void update_uvs(const float2* uvs);
        void update_positions(const float3* vert);

        vao(vao&& other);

    private:
        vao(const vao& other) = delete;

        uint32_t _id;
        int _vertex_count;
        vbo _vertexes, _normals, _indexes, _uvs, _tangents;
    };

    class texture_2d_shader
    {
    public:
        texture_2d_shader();

        void begin();
        void end();

        void set_opacity(float opacity);

        void set_position_and_scale(
            const float2& position,
            const float2& scale);
    protected:
        texture_2d_shader(std::unique_ptr<shader_program> shader);

        std::unique_ptr<shader_program> _shader;

        static const char* default_vertex_shader();

    private:
        void init();

        uint32_t _position_location;
        uint32_t _scale_location;
        uint32_t _opacity_location;
    };

    class splash_screen_shader : public texture_2d_shader
    {
    public:
        splash_screen_shader();

        void set_ray_center(const float2& center);
        void set_power(float power);
    private:
        uint32_t _rays_location;
        uint32_t _power_location;
    };

    class texture_visualizer
    {
    public:
        texture_visualizer(float2 pos, float2 scale)
            : _position(std::move(pos)),
            _scale(std::move(scale)),
            _geometry(vao::create(create_mesh()))
        {

        }

        texture_visualizer()
            : texture_visualizer({ 0.f, 0.f }, { 1.f, 1.f }) {}

        void set_position(float2 pos) { _position = pos; }
        void set_scale(float2 scale) { _scale = scale; }

        void draw(texture_2d_shader& shader, uint32_t tex);

    private:
        static obj_mesh create_mesh();

        float2 _position;
        float2 _scale;
        std::shared_ptr<vao> _geometry;
    };

    class visualizer_2d
    {
    public:
        visualizer_2d()
            : visualizer_2d(std::make_shared<texture_2d_shader>())
        {
        }

        visualizer_2d(std::shared_ptr<texture_2d_shader> shader)
            : tex_2d_shader(shader)
        {
        }

        void draw_texture(uint32_t tex, float opacity = 1.f);

        void draw_texture(float2 pos, float2 scale, uint32_t tex)
        {
            tex_2d_shader->begin();
            _visualizer.set_position(pos);
            _visualizer.set_scale(scale);
            _visualizer.draw(*tex_2d_shader, tex);
            tex_2d_shader->end();
        }

        texture_2d_shader& get_shader() { return *tex_2d_shader; }

    private:
        texture_visualizer _visualizer;
        std::shared_ptr<texture_2d_shader> tex_2d_shader;
    };

    inline obj_mesh make_grid(int a, int b)
    {
        obj_mesh res;

        auto toidx = [&](int i, int j) {
            return i * b + j;
        };

        for (auto i = 0; i < a; i++)
        {
            for (auto j = 0; j < b; j++)
            {
                float3 point{ (float)j, (float)i, 1.f };
                res.positions.push_back(point);
                res.normals.push_back(float3{ 0.f, 0.f, -1.f });

                res.uvs.emplace_back(float2{ (i+0.5f)/(float)(a), (j+0.5f)/(float)(b) });

                if (i < a - 1 && j < b - 1)
                {
                    auto curr = toidx(i, j);
                    auto next_a = toidx(i + 1, j);
                    auto next_b = toidx(i, j + 1);
                    auto next_ab = toidx(i + 1, j + 1);
                    res.indexes.emplace_back(int3{ curr, next_a, next_b });
                    res.indexes.emplace_back(int3{ next_a, next_ab, next_b });
                }
            }
        }

        return res;
    }

    class fbo
    {
    public:
        fbo(int w, int h);

        void createTextureAttachment(uint32_t texture);

        void createDepthTextureAttachment(uint32_t texture);

        void bind();

        void unbind();

        void createDepthBufferAttachment();

        ~fbo();

        std::string get_status();

        int get_width() const { return _w; }
        int get_height() const { return _h; }
        uint32_t get() const { return _id; }
    private:
        uint32_t _id;
        uint32_t _db = 0;
        int _w, _h;
    };
}