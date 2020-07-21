// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <glad/glad.h>

#include <librealsense2/rs.hpp>
#include <librealsense2-gl/rs_processing_gl.hpp>

#include <vector>
#include <algorithm>
#include <cstring>
#include <ctype.h>
#include <memory>
#include <string>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <array>
#include <chrono>
#define _USE_MATH_DEFINES
#include <cmath>
#include <map>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _MSC_VER
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER  0x812D
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE    0x812F
#endif
#endif

namespace rs2
{
    class fps_calc
    {
    public:
        fps_calc()
            : _counter(0),
            _delta(0),
            _last_timestamp(0),
            _num_of_frames(0)
        {}

        fps_calc(const fps_calc& other)
        {
            std::lock_guard<std::mutex> lock(other._mtx);
            _counter = other._counter;
            _delta = other._delta;
            _num_of_frames = other._num_of_frames;
            _last_timestamp = other._last_timestamp;
        }
        void add_timestamp(double timestamp, unsigned long long frame_counter)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (++_counter >= _skip_frames)
            {
                if (_last_timestamp != 0)
                {
                    _delta = timestamp - _last_timestamp;
                    _num_of_frames = frame_counter - _last_frame_counter;
                }

                _last_frame_counter = frame_counter;
                _last_timestamp = timestamp;
                _counter = 0;
            }
        }

        double get_fps() const
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_delta == 0)
                return 0;

            return (static_cast<double>(_numerator) * _num_of_frames) / _delta;
        }

    private:
        static const int _numerator = 1000;
        static const int _skip_frames = 5;
        int _counter;
        double _delta;
        double _last_timestamp;
        unsigned long long _num_of_frames;
        unsigned long long _last_frame_counter;
        mutable std::mutex _mtx;
    };

    inline float clamp(float x, float min, float max)
    {
        return std::max(std::min(max, x), min);
    }

    inline float smoothstep(float x, float min, float max)
    {
        x = clamp((x - min) / (max - min), 0.0, 1.0);
        return x*x*(3 - 2 * x);
    }

    inline float lerp(float a, float b, float t)
    {
        return b * t + a * (1 - t);
    }

    struct plane
    {
        float a;
        float b;
        float c;
        float d;
    };
    inline bool operator==(const plane& lhs, const plane& rhs) { return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d; }

    struct float3
    {
        float x, y, z;

        float length() const { return sqrt(x*x + y*y + z*z); }

        float3 normalize() const
        {
            return (length() > 0)? float3{ x / length(), y / length(), z / length() }:*this;
        }
    };

    struct float4
    {
        float x, y, z, w;
    };

    inline float3 cross(const float3& a, const float3& b)
    {
        return { a.y * b.z - b.y * a.z, a.x * b.z - b.x * a.z, a.x * b.y - a.y * b.x };
    }

    inline float evaluate_plane(const plane& plane, const float3& point)
    {
        return plane.a * point.x + plane.b * point.y + plane.c * point.z + plane.d;
    }

    inline float3 operator*(const float3& a, float t)
    {
        return { a.x * t, a.y * t, a.z * t };
    }

    inline float3 operator/(const float3& a, float t)
    {
        return { a.x / t, a.y / t, a.z / t };
    }

    inline float3 operator+(const float3& a, const float3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    inline float3 operator-(const float3& a, const float3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    inline float3 lerp(const float3& a, const float3& b, float t)
    {
        return b * t + a * (1 - t);
    }

    struct float2
    {
        float x, y;

        float length() const { return sqrt(x*x + y*y); }

        float2 normalize() const
        {
            return { x / length(), y / length() };
        }
    };

    inline float dot(const rs2::float2& a, const rs2::float2& b)
    {
        return a.x * b.x + a.y * b.y;
    }

    inline rs2::float2 lerp(const rs2::float2& a, const rs2::float2& b, float t)
    {
        return rs2::float2{ lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
    }

    inline float3 lerp(const std::array<float3, 4>& rect, const float2& p)
    {
        auto v1 = lerp(rect[0], rect[1], p.x);
        auto v2 = lerp(rect[3], rect[2], p.x);
        return lerp(v1, v2, p.y);
    }

    using plane_3d = std::array<float3, 4>;

    inline std::vector<plane_3d> subdivide(const plane_3d& rect, int parts = 4)
    {
        std::vector<plane_3d> res;
        res.reserve(parts*parts);
        for (float i = 0.f; i < parts; i++)
        {
            for (float j = 0.f; j < parts; j++)
            {
                plane_3d r;
                r[0] = lerp(rect, { i / parts, j / parts });
                r[1] = lerp(rect, { i / parts, (j + 1) / parts });
                r[2] = lerp(rect, { (i + 1) / parts, (j + 1) / parts });
                r[3] = lerp(rect, { (i + 1) / parts, j / parts });
                res.push_back(r);
            }
        }
        return res;
    }

    inline float operator*(const float3& a, const float3& b)
    {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    inline bool is_valid(const plane_3d& p)
    {
        std::vector<float> angles;
        angles.reserve(4);
        for (size_t i = 0; i < p.size(); i++)
        {
            auto p1 = p[i];
            auto p2 = p[(i+1) % p.size()];
            if ((p2 - p1).length() < 1e-3) return false;

            p1 = p1.normalize();
            p2 = p2.normalize();

            angles.push_back(acos((p1 * p2) / sqrt(p1.length() * p2.length())));
        }
        return std::all_of(angles.begin(), angles.end(), [](float f) { return f > 0; }) ||
               std::all_of(angles.begin(), angles.end(), [](float f) { return f < 0; });
    }

    inline float2 operator-(float2 a, float2 b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    inline float2 operator*(float a, float2 b)
    {
        return { a * b.x, a * b.y };
    }

    struct matrix4
    {
        float mat[4][4];

        operator float*() const
        {
            return (float*)&mat;
        } 

        static matrix4 identity()
        {
            matrix4 m;
            for (int i = 0; i < 4; i++)
                m.mat[i][i] = 1.f;
            return m;
        }

        matrix4()
        {
            std::memset(mat, 0, sizeof(mat));
        }

        matrix4(float vals[4][4])
        {
            std::memcpy(mat,vals,sizeof(mat));
        }

        // convert glGetFloatv output to matrix4
        //
        //   float m[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        // into
        //   rs2::matrix4 m = { {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} };
        //       0     1     2     3
        //       4     5     6     7
        //       8     9    10    11
        //       12   13    14    15
        matrix4(float vals[16])
        {
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    mat[i][j] = vals[i * 4 + j];
                }
            }
        }

        float& operator()(int i, int j) { return mat[i][j]; }
        const float& operator()(int i, int j) const { return mat[i][j]; }

        //init rotation matrix from quaternion
        matrix4(const rs2_quaternion& q)
        {
            mat[0][0] = 1 - 2*q.y*q.y - 2*q.z*q.z; mat[0][1] = 2*q.x*q.y - 2*q.z*q.w;     mat[0][2] = 2*q.x*q.z + 2*q.y*q.w;     mat[0][3] = 0.0f;
            mat[1][0] = 2*q.x*q.y + 2*q.z*q.w;     mat[1][1] = 1 - 2*q.x*q.x - 2*q.z*q.z; mat[1][2] = 2*q.y*q.z - 2*q.x*q.w;     mat[1][3] = 0.0f;
            mat[2][0] = 2*q.x*q.z - 2*q.y*q.w;     mat[2][1] = 2*q.y*q.z + 2*q.x*q.w;     mat[2][2] = 1 - 2*q.x*q.x - 2*q.y*q.y; mat[2][3] = 0.0f;
            mat[3][0] = 0.0f;                      mat[3][1] = 0.0f;                      mat[3][2] = 0.0f;                      mat[3][3] = 1.0f;
        }

        //init translation matrix from vector
        matrix4(const rs2_vector& t)
        {
            mat[0][0] = 1.0f; mat[0][1] = 0.0f; mat[0][2] = 0.0f; mat[0][3] = t.x;
            mat[1][0] = 0.0f; mat[1][1] = 1.0f; mat[1][2] = 0.0f; mat[1][3] = t.y;
            mat[2][0] = 0.0f; mat[2][1] = 0.0f; mat[2][2] = 1.0f; mat[2][3] = t.z;
            mat[3][0] = 0.0f; mat[3][1] = 0.0f; mat[3][2] = 0.0f; mat[3][3] = 1.0f;
        }

        rs2_quaternion normalize(rs2_quaternion a)
        {
            float norm = sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
            rs2_quaternion res = a;
            res.x /= norm;
            res.y /= norm;
            res.z /= norm;
            res.w /= norm;
            return res;
        }

        rs2_quaternion to_quaternion()
        {
            float tr[4];
            rs2_quaternion res;
            tr[0] = (mat[0][0] + mat[1][1] + mat[2][2]);
            tr[1] = (mat[0][0] - mat[1][1] - mat[2][2]);
            tr[2] = (-mat[0][0] + mat[1][1] - mat[2][2]);
            tr[3] = (-mat[0][0] - mat[1][1] + mat[2][2]);
            if (tr[0] >= tr[1] && tr[0] >= tr[2] && tr[0] >= tr[3])
            {
                float s = 2 * sqrt(tr[0] + 1);
                res.w = s / 4;
                res.x = (mat[2][1] - mat[1][2]) / s;
                res.y = (mat[0][2] - mat[2][0]) / s;
                res.z = (mat[1][0] - mat[0][1]) / s;
            }
            else if (tr[1] >= tr[2] && tr[1] >= tr[3]) {
                float s = 2 * sqrt(tr[1] + 1);
                res.w = (mat[2][1] - mat[1][2]) / s;
                res.x = s / 4;
                res.y = (mat[1][0] + mat[0][1]) / s;
                res.z = (mat[2][0] + mat[0][2]) / s;
            }
            else if (tr[2] >= tr[3]) {
                float s = 2 * sqrt(tr[2] + 1);
                res.w = (mat[0][2] - mat[2][0]) / s;
                res.x = (mat[1][0] + mat[0][1]) / s;
                res.y = s / 4;
                res.z = (mat[1][2] + mat[2][1]) / s;
            }
            else {
                float s = 2 * sqrt(tr[3] + 1);
                res.w = (mat[1][0] - mat[0][1]) / s;
                res.x = (mat[0][2] + mat[2][0]) / s;
                res.y = (mat[1][2] + mat[2][1]) / s;
                res.z = s / 4;
            }
            return normalize(res);
        }

        void to_column_major(float column_major[16])
        {
            column_major[0] = mat[0][0];
            column_major[1] = mat[1][0];
            column_major[2] = mat[2][0];
            column_major[3] = mat[3][0];
            column_major[4] = mat[0][1];
            column_major[5] = mat[1][1];
            column_major[6] = mat[2][1];
            column_major[7] = mat[3][1];
            column_major[8] = mat[0][2];
            column_major[9] = mat[1][2];
            column_major[10] = mat[2][2];
            column_major[11] = mat[3][2];
            column_major[12] = mat[0][3];
            column_major[13] = mat[1][3];
            column_major[14] = mat[2][3];
            column_major[15] = mat[3][3];
        }
    };

    inline matrix4 operator*(const matrix4& a, const matrix4& b)
    {
        matrix4 res;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++)
                {
                    sum += a.mat[i][k] * b.mat[k][j];
                }
                res.mat[i][j] = sum;
            }
        }
        return res;
    }

    inline float4 operator*(const matrix4& a, const float4& b)
    {
        float4 res;
        int i = 0;
        res.x = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
        res.y = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
        res.z = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
        res.w = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
        return res;
    }

    inline matrix4 tm2_pose_to_world_transformation(const rs2_pose& pose)
    {
        matrix4 rotation(pose.rotation);
        matrix4 translation(pose.translation);
        matrix4 G_tm2_body_to_tm2_world = translation * rotation;
        float rotate_180_y[4][4] = { { -1, 0, 0, 0 },
                                     { 0, 1, 0, 0 },
                                     { 0, 0,-1, 0 },
                                     { 0, 0, 0, 1 } };
        matrix4 G_vr_body_to_tm2_body(rotate_180_y);
        matrix4 G_vr_body_to_tm2_world = G_tm2_body_to_tm2_world * G_vr_body_to_tm2_body;

        float rotate_90_x[4][4] = { { 1, 0, 0, 0 },
                                    { 0, 0,-1, 0 },
                                    { 0, 1, 0, 0 },
                                    { 0, 0, 0, 1 } };
        matrix4 G_tm2_world_to_vr_world(rotate_90_x);
        matrix4 G_vr_body_to_vr_world = G_tm2_world_to_vr_world * G_vr_body_to_tm2_world;

        return G_vr_body_to_vr_world;
    }

    inline rs2_pose correct_tm2_pose(const rs2_pose& pose)
    {
        matrix4 G_vr_body_to_vr_world = tm2_pose_to_world_transformation(pose);
        rs2_pose res = pose;
        res.translation.x = G_vr_body_to_vr_world.mat[0][3];
        res.translation.y = G_vr_body_to_vr_world.mat[1][3];
        res.translation.z = G_vr_body_to_vr_world.mat[2][3];
        res.rotation = G_vr_body_to_vr_world.to_quaternion();
        return res;
    }

    struct mouse_info
    {
        float2 cursor{ 0.f, 0.f };
        float2 prev_cursor{ 0.f, 0.f };
        bool mouse_down[2] { false, false };
        int mouse_wheel = 0;
        float ui_wheel = 0.f;
    };

    template<typename T>
    T normalizeT(const T& in_val, const T& min, const T& max)
    {
        return ((in_val - min)/(max - min));
    }

    template<typename T>
    T unnormalizeT(const T& in_val, const T& min, const T& max)
    {
        if (min == max) return min;
        return ((in_val * (max - min)) + min);
    }

    struct rect
    {
        float x, y;
        float w, h;

        void operator=(const rect& other)
        {
            x = other.x;
            y = other.y;
            w = other.w;
            h = other.h;
        }

        operator bool() const
        {
            return w*w > 0 && h*h > 0;
        }

        bool operator==(const rect& other) const
        {
            return x == other.x && y == other.y && w == other.w && h == other.h;
        }

        bool operator!=(const rect& other) const
        {
            return !(*this == other);
        }

        rect normalize(const rect& normalize_to) const
        {
            return rect{normalizeT(x, normalize_to.x, normalize_to.x + normalize_to.w),
                        normalizeT(y, normalize_to.y, normalize_to.y + normalize_to.h),
                        normalizeT(w, 0.f, normalize_to.w),
                        normalizeT(h, 0.f, normalize_to.h)};
        }

        rect unnormalize(const rect& unnormalize_to) const
        {
            return rect{unnormalizeT(x, unnormalize_to.x, unnormalize_to.x + unnormalize_to.w),
                        unnormalizeT(y, unnormalize_to.y, unnormalize_to.y + unnormalize_to.h),
                        unnormalizeT(w, 0.f, unnormalize_to.w),
                        unnormalizeT(h, 0.f, unnormalize_to.h)};
        }

        // Calculate the intersection between two rects
        // If the intersection is empty, a rect with width and height zero will be returned
        rect intersection(const rect& other) const
        {
            auto x1 = std::max(x, other.x);
            auto y1 = std::max(y, other.y);
            auto x2 = std::min(x + w, other.x + other.w);
            auto y2 = std::min(y + h, other.y + other.h);

            return{
                x1, y1,
                std::max(x2 - x1, 0.f),
                std::max(y2 - y1, 0.f)
            };
        }

        // Calculate the area of the rect
        float area() const
        {
            return w * h;
        }

        rect cut_by(const rect& r) const
        {
            auto x1 = x;
            auto y1 = y;
            auto x2 = x + w;
            auto y2 = y + h;

            x1 = std::max(x1, r.x);
            x1 = std::min(x1, r.x + r.w);
            y1 = std::max(y1, r.y);
            y1 = std::min(y1, r.y + r.h);

            x2 = std::max(x2, r.x);
            x2 = std::min(x2, r.x + r.w);
            y2 = std::max(y2, r.y);
            y2 = std::min(y2, r.y + r.h);

            return { x1, y1, x2 - x1, y2 - y1 };
        }

        bool contains(const float2& p) const
        {
            return (p.x >= x) && (p.x < x + w) && (p.y >= y) && (p.y < y + h);
        }

        rect pan(const float2& p) const
        {
            return { x - p.x, y - p.y, w, h };
        }

        rect center() const
        {
            return{ x + w / 2.f, y + h / 2.f, 0, 0 };
        }

        rect lerp(float t, const rect& other) const
        {
            return{
                rs2::lerp(x, other.x, t), rs2::lerp(y, other.y, t),
                rs2::lerp(w, other.w, t), rs2::lerp(h, other.h, t),
            };
        }

        rect adjust_ratio(float2 size) const
        {
            auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
            if (W > w)
            {
                auto scale = w / W;
                W *= scale;
                H *= scale;
            }

            return{ float(floor(x + floor(w - W) / 2)),
                    float(floor(y + floor(h - H) / 2)),
                    W, H };
        }

        rect scale(float factor) const
        {
            return { x, y, w * factor, h * factor };
        }

        rect grow(int pixels) const
        {
            return { x - pixels, y - pixels, w + pixels*2, h + pixels*2 };
        }

        rect grow(int dx, int dy) const
        {
            return { x - dx, y - dy, w + dx*2, h + dy*2 };
        }

        rect shrink_by(float2 pixels) const
        {
            return { x + pixels.x, y + pixels.y, w - pixels.x * 2, h - pixels.y * 2 };
        }

        rect center_at(const float2& new_center) const
        {
            auto c = center();
            auto diff_x = new_center.x - c.x;
            auto diff_y = new_center.y - c.y;

            return { x + diff_x, y + diff_y, w, h };
        }

        rect fit(rect r) const
        {
            float new_w = w;
            float new_h = h;

            if (w < r.w)
                new_w = r.w;

            if (h < r.h)
                new_h = r.h;

            auto res = rect{x, y, new_w, new_h};
            return res.adjust_ratio({w,h});
        }

        rect zoom(float zoom_factor) const
        {
            auto c = center();
            return scale(zoom_factor).center_at({c.x,c.y});
        }

        rect enclose_in(rect in_rect) const
        {
            rect out_rect{x, y, w, h};
            if (w > in_rect.w || h > in_rect.h)
            {
                return in_rect;
            }

            if (x < in_rect.x)
            {
                out_rect.x = in_rect.x;
            }

            if (y < in_rect.y)
            {
                out_rect.y = in_rect.y;
            }


            if (x + w > in_rect.x + in_rect.w)
            {
                out_rect.x = in_rect.x + in_rect.w - w;
            }

            if (y + h > in_rect.y + in_rect.h)
            {
                out_rect.y = in_rect.y + in_rect.h - h;
            }

            return out_rect;
        }

        bool intersects(const rect& other) const
        {
            return other.contains({ x, y }) || other.contains({ x + w, y }) ||
                other.contains({ x, y + h }) || other.contains({ x + w, y + h }) ||
                contains({ other.x, other.y });
        }
    };

    //////////////////////////////
    // Simple font loading code //
    //////////////////////////////

#include "../third-party/stb_easy_font.h"

    inline void draw_text(int x, int y, const char * text)
    {
        char buffer[60000]; // ~300 chars
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, buffer);
        glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    ////////////////////////
    // Image display code //
    ////////////////////////

    class color_map
    {
    public:
        color_map(std::map<float, float3> map, int steps = 4000) : _map(map)
        {
            initialize(steps);
        }

        color_map(const std::vector<float3>& values, int steps = 4000)
        {
            for (size_t i = 0; i < values.size(); i++)
            {
                _map[(float)i/(values.size()-1)] = values[i];
            }
            initialize(steps);
        }

        color_map() {}

        float3 get(float value) const
        {
            if (_max == _min) return *_data;
            auto t = (value - _min) / (_max - _min);
            t = std::min(std::max(t, 0.f), 1.f);
            return _data[(int)(t * (_size - 1))];
        }

        float min_key() const { return _min; }
        float max_key() const { return _max; }

    private:
        float3 calc(float value) const
        {
            if (_map.size() == 0) return { value, value, value };
            // if we have exactly this value in the map, just return it
            if( _map.find(value) != _map.end() ) return _map.at(value);
            // if we are beyond the limits, return the first/last element
            if( value < _map.begin()->first )   return _map.begin()->second;
            if( value > _map.rbegin()->first )  return _map.rbegin()->second;

            auto lower = _map.lower_bound(value) == _map.begin() ? _map.begin() : --(_map.lower_bound(value)) ;
            auto upper = _map.upper_bound(value);

            auto t = (value - lower->first) / (upper->first - lower->first);
            auto c1 = lower->second;
            auto c2 = upper->second;
            return lerp(c1, c2, t);
        }

        void initialize(int steps)
        {
            if (_map.size() == 0) return;

            _min = _map.begin()->first;
            _max = _map.rbegin()->first;

            _cache.resize(steps + 1);
            for (int i = 0; i <= steps; i++)
            {
                auto t = (float)i/steps;
                auto x = _min + t*(_max - _min);
                _cache[i] = calc(x);
            }

            // Save size and data to avoid STL checks penalties in DEBUG
            _size = _cache.size();
            _data = _cache.data();
        }

        std::map<float, float3> _map;
        std::vector<float3> _cache;
        float _min, _max;
        size_t _size; float3* _data;
    };

    using clock = std::chrono::steady_clock;

    // Helper class to keep track of time
    class timer
    {
    public:
        timer()
        {
            _start = std::chrono::steady_clock::now();
        }

        void reset() { _start = std::chrono::steady_clock::now(); }

        // Get elapsed milliseconds since timer creation
        double elapsed_ms() const
        {
            return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed()).count();
        }

        clock::duration elapsed() const
        {
            return clock::now() - _start;
        }

        clock::time_point now() const
        {
            return clock::now();
        }
    private:
        clock::time_point _start;
    };

    class periodic_timer
    {
    public:
        periodic_timer(clock::duration delta)
            : _delta(delta)
        {
            _last = _time.now();
        }

        operator bool() const
        {
            if (_time.now() - _last > _delta)
            {
                _last = _time.now();
                return true;
            }
            return false;
        }

        void signal() const
        {
            _last = _time.now() - _delta;
        }

    private:
        timer _time;
        mutable clock::time_point _last;
        clock::duration _delta;
    };

    // Temporal event is a very simple time filter
    // that allows a concensus based on a set of measurements in time
    // You set the window, and add measurements, and the class offers
    // the most agreed upon opinion within the set time window
    // It is useful to remove noise from UX elements
    class temporal_event
    {
    public:
        temporal_event(clock::duration window) : _window(window) {}
        temporal_event() : _window(std::chrono::milliseconds(1000)) {}

        void add_value(bool val)
        {
            std::lock_guard<std::mutex> lock(_m);
            _measurements.push_back(std::make_pair(clock::now(), val));
        }

        bool eval()
        {
            return get_stat() > 0.5f;
        }

        float get_stat()
        {
            std::lock_guard<std::mutex> lock(_m);

            if (_t.elapsed() < _window) return false; // Ensure no false alarms in the warm-up time

            _measurements.erase(std::remove_if(_measurements.begin(), _measurements.end(),
                [this](std::pair<clock::time_point, bool> pair) {
                return (clock::now() - pair.first) > _window;
            }),
                _measurements.end());
            auto trues = std::count_if(_measurements.begin(), _measurements.end(),
                [](std::pair<clock::time_point, bool> pair) {
                return pair.second;
            });
            return size_t(trues) / (float)_measurements.size(); 
        }

        void reset()
        {
            std::lock_guard<std::mutex> lock(_m);
            _t.reset();
            _measurements.clear();
        }

    private:
        std::mutex _m;
        clock::duration _window;
        std::vector<std::pair<clock::time_point, bool>> _measurements;
        timer _t;
    };

    class texture_buffer
    {
        GLuint texture;
        rs2::frame_queue last_queue[2];
        mutable rs2::frame last[2];
    public:
        std::shared_ptr<colorizer> colorize;
        std::shared_ptr<yuy_decoder> yuy2rgb;
        std::shared_ptr<depth_huffman_decoder> depth_decode;
        bool zoom_preview = false;
        rect curr_preview_rect{};
        int texture_id = 0;

        texture_buffer(const texture_buffer& other)
        {
            texture = other.texture;
        }

        texture_buffer& operator=(const texture_buffer& other)
        {
            texture = other.texture;
            return *this;
        }

        rs2::frame get_last_frame(bool with_texture = false) const {
            auto idx = with_texture ? 1 : 0;
            last_queue[idx].poll_for_frame(&last[idx]);
            return last[idx];
        }

        texture_buffer() : last_queue(), texture() {}

        GLuint get_gl_handle() const {
            // If the frame is already a GPU frame
            // Just get the texture from the frame
            if (auto gf = get_last_frame(true).as<gl::gpu_frame>())
            {
                auto tex = gf.get_texture_id(texture_id);
                return tex;
            }
            else return texture;
        }

        // Simplified version of upload that lets us load basic RGBA textures
        // This is used for the splash screen
        void upload_image(int w, int h, void* data, int format = GL_RGBA)
        {
            if (!texture)
                glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void upload(rs2::frame frame, rs2_format prefered_format = RS2_FORMAT_ANY)
        {
            if (!texture)
                glGenTextures(1, &texture);

            int width = 0;
            int height = 0;
            int stride = 0;
            auto format = frame.get_profile().format();

            // Decode compressed data required for mouse pointer depth calculations
            if (RS2_FORMAT_Z16H==format)
            {
                frame = depth_decode->process(frame);
                format = frame.get_profile().format();
            }

            last_queue[0].enqueue(frame);

            // When frame is a GPU frame
            // we don't need to access pixels, keep data NULL
            auto data = !frame.is<gl::gpu_frame>() ? frame.get_data() : nullptr;

            auto rendered_frame = frame;
            auto image = frame.as<video_frame>();

            if (image)
            {
                width = image.get_width();
                height = image.get_height();
                stride = image.get_stride_in_bytes();
            }
            else if (auto profile = frame.get_profile().as<rs2::video_stream_profile>())
            {
                width = profile.width();
                height = profile.height();
                stride = width;
            }

            glBindTexture(GL_TEXTURE_2D, texture);
            stride = stride == 0 ? width : stride;
            //glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

            // Allow upload of points frame type
            if (auto pc = frame.as<points>())
            {
                if (!frame.is<gl::gpu_frame>())
                {
                    // Points can be uploaded as two different 
                    // formats: XYZ for verteces and UV for texture coordinates
                    if (prefered_format == RS2_FORMAT_XYZ32F)
                    {
                        // Upload vertices
                        data = pc.get_vertices();
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
                    }
                    else
                    {
                        // Upload texture coordinates
                        data = pc.get_texture_coordinates();
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, data);
                    }

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }
                else
                {
                    // Update texture_id based on desired format
                    if (prefered_format == RS2_FORMAT_XYZ32F) texture_id = 0;
                    else texture_id = 1;
                }
            }
            else
            {
                switch (format)
                {
                case RS2_FORMAT_ANY:
                    throw std::runtime_error("not a valid format");
                case RS2_FORMAT_Z16H:
                    throw std::runtime_error("unexpected format: Z16H. Check decoder processing block");
                case RS2_FORMAT_Z16:
                case RS2_FORMAT_DISPARITY16:
                case RS2_FORMAT_DISPARITY32:
                    if (frame.is<depth_frame>())
                    {
                        if (prefered_format == RS2_FORMAT_Z16)
                        {
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, data);
                        }
                        else if (prefered_format == RS2_FORMAT_DISPARITY32)
                        {
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
                        }
                        else
                        {
                            if (auto colorized_frame = colorize->colorize(frame).as<video_frame>())
                            {
                                if (!colorized_frame.is<gl::gpu_frame>())
                                {
                                    data = colorized_frame.get_data();
                                    // Override the first pixel in the colorized image for occlusion invalidation.
                                    memset((void*)data, 0, colorized_frame.get_bytes_per_pixel());
                                    {
                                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                                            colorized_frame.get_width(),
                                            colorized_frame.get_height(),
                                            0, GL_RGB, GL_UNSIGNED_BYTE,
                                            data);
                                    }
                                }
                                rendered_frame = colorized_frame;
                            }
                        }
                    }
                    else glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, data);
                    break;
                case RS2_FORMAT_XYZ32F:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data);
                    break;
                case RS2_FORMAT_YUYV:
                    if (yuy2rgb)
                    {
                        if (auto colorized_frame = yuy2rgb->process(frame).as<video_frame>())
                        {
                            if (!colorized_frame.is<gl::gpu_frame>())
                            {
                                glBindTexture(GL_TEXTURE_2D, texture);
                                data = colorized_frame.get_data();

                                // Override the first pixel in the colorized image for occlusion invalidation.
                                memset((void*)data, 0, colorized_frame.get_bytes_per_pixel());
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                                    colorized_frame.get_width(),
                                    colorized_frame.get_height(),
                                    0, GL_RGB, GL_UNSIGNED_BYTE,
                                    colorized_frame.get_data());
                            }
                            rendered_frame = colorized_frame;
                        }
                    }
                    else
                    {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
                    }
                    break;
                case RS2_FORMAT_UYVY: // Use luminance component only to avoid costly UVUY->RGB conversion
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
                    break;
                case RS2_FORMAT_RGB8: case RS2_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    break;
                case RS2_FORMAT_RGBA8: case RS2_FORMAT_BGRA8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    break;
                case RS2_FORMAT_Y8:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                    break;
                case RS2_FORMAT_MOTION_XYZ32F:
                {
                    if (auto motion = frame.as<motion_frame>())
                    {
                        auto axes = motion.get_motion_data();
                        draw_motion_data(axes.x, axes.y, axes.z);
                    }
                    else
                    {
                        throw std::runtime_error("Not expecting a frame with motion format that is not a motion_frame");
                    }
                    break;
                }
                case RS2_FORMAT_Y16:
                case RS2_FORMAT_Y10BPACK:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
                    break;
                case RS2_FORMAT_RAW8:
                case RS2_FORMAT_MOTION_RAW:
                case RS2_FORMAT_GPIO_RAW:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                    break;
                case RS2_FORMAT_6DOF:
                {
                    if (auto pose = frame.as<pose_frame>())
                    {
                        rs2_pose pose_data = pose.get_pose_data();
                        draw_pose_data(pose_data, frame.get_profile().unique_id());
                    }
                    else
                    {
                        throw std::runtime_error("Not expecting a frame with 6DOF format that is not a pose_frame");
                    }
                    break;
                }
                //case RS2_FORMAT_RAW10:
                //{
                //    // Visualize Raw10 by performing a naive down sample. Each 2x2 block contains one red pixel, two green pixels, and one blue pixel, so combine them into a single RGB triple.
                //    rgb.clear(); rgb.resize(width / 2 * height / 2 * 3);
                //    auto out = rgb.data(); auto in0 = reinterpret_cast<const uint8_t *>(data), in1 = in0 + width * 5 / 4;
                //    for (auto y = 0; y<height; y += 2)
                //    {
                //        for (auto x = 0; x<width; x += 4)
                //        {
                //            *out++ = in0[0]; *out++ = (in0[1] + in1[0]) / 2; *out++ = in1[1]; // RGRG -> RGB RGB
                //            *out++ = in0[2]; *out++ = (in0[3] + in1[2]) / 2; *out++ = in1[3]; // GBGB
                //            in0 += 5; in1 += 5;
                //        }
                //        in0 = in1; in1 += width * 5 / 4;
                //    }
                //    glPixelStorei(GL_UNPACK_ROW_LENGTH, width / 2);        // Update row stride to reflect post-downsampling dimensions of the target texture
                //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width / 2, height / 2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
                //}
                //break;
                default:
                {
                    memset((void*)data, 0, height*width);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                }
                }

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            last_queue[1].enqueue(rendered_frame);
        }

        static void  draw_axes(float axis_size = 1.f, float axisWidth = 4.f)
        {

            // Triangles For X axis
            glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(axis_size * 1.1f, 0.f, 0.f);
            glVertex3f(axis_size, -axis_size * 0.05f, 0.f);
            glVertex3f(axis_size,  axis_size * 0.05f, 0.f);
            glVertex3f(axis_size * 1.1f, 0.f, 0.f);
            glVertex3f(axis_size, 0.f, -axis_size * 0.05f);
            glVertex3f(axis_size, 0.f,  axis_size * 0.05f);
            glEnd();

            // Triangles For Y axis
            glBegin(GL_TRIANGLES);
            glColor3f(0.f, 1.f, 0.f);
            glVertex3f(0.f, axis_size * 1.1f, 0.0f);
            glVertex3f(0.f, axis_size,  0.05f * axis_size);
            glVertex3f(0.f, axis_size, -0.05f * axis_size);
            glVertex3f(0.f, axis_size * 1.1f, 0.0f);
            glVertex3f( 0.05f * axis_size, axis_size, 0.f);
            glVertex3f(-0.05f * axis_size, axis_size, 0.f);
            glEnd();

            // Triangles For Z axis
            glBegin(GL_TRIANGLES);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 1.1f * axis_size);
            glVertex3f(0.0f, 0.05f * axis_size, 1.0f * axis_size);
            glVertex3f(0.0f, -0.05f * axis_size, 1.0f * axis_size);
            glVertex3f(0.0f, 0.0f, 1.1f * axis_size);
            glVertex3f(0.05f * axis_size, 0.f, 1.0f * axis_size);
            glVertex3f(-0.05f * axis_size, 0.f, 1.0f * axis_size);
            glEnd();

            glLineWidth(axisWidth);

            // Drawing Axis
            glBegin(GL_LINES);
            // X axis - Red
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(axis_size, 0.0f, 0.0f);

            // Y axis - Green
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, axis_size, 0.0f);

            // Z axis - Blue
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, axis_size);
            glEnd();
        }

        // intensity is grey intensity
        static void draw_circle(float xx, float xy, float xz, float yx, float yy, float yz, float radius = 1.1, float3 center = { 0.0, 0.0, 0.0 }, float intensity = 0.5f)
        {
            const auto N = 50;
            glColor3f(intensity, intensity, intensity);
            glLineWidth(2);
            glBegin(GL_LINE_STRIP);

            for (int i = 0; i <= N; i++)
            {
                const double theta = (2 * M_PI / N) * i;
                const auto cost = static_cast<float>(cos(theta));
                const auto sint = static_cast<float>(sin(theta));
                glVertex3f(
                    center.x + radius * (xx * cost + yx * sint),
                    center.y + radius * (xy * cost + yy * sint),
                    center.z + radius * (xz * cost + yz * sint)
                    );
            }

            glEnd();
        }

        void multiply_vector_by_matrix(GLfloat vec[], GLfloat mat[], GLfloat* result)
        {
            const auto N = 4;
            for (int i = 0; i < N; i++)
            {
                result[i] = 0;
                for (int j = 0; j < N; j++)
                {
                    result[i] += vec[j] * mat[N*j + i];
                }
            }
            return;
        }

        float2 xyz_to_xy(float x, float y, float z, GLfloat model[], GLfloat proj[], float vec_norm)
        {
            GLfloat vec[4] = { x, y, z, 0 };
            float tmp_result[4];
            float result[4];

            const auto canvas_size = 230;

            multiply_vector_by_matrix(vec, model, tmp_result);
            multiply_vector_by_matrix(tmp_result, proj, result);

            return{ canvas_size * vec_norm *result[0], canvas_size * vec_norm *result[1] };
        }

        void print_text_in_3d(float x, float y, float z, const char* text, bool center_text, GLfloat model[], GLfloat proj[], float vec_norm)
        {
            auto xy = xyz_to_xy(x, y, z, model, proj, vec_norm);
            auto w = (center_text) ? stb_easy_font_width((char*)text) : 0;
            glColor3f(1.0f, 1.0f, 1.0f);
            draw_text((int)(xy.x - w / 2), (int)xy.y, text);
        }

        void draw_motion_data(float x, float y, float z)
        {
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();

            glViewport(0, 0, 768, 768);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            glOrtho(-2.8, 2.8, -2.4, 2.4, -7, 7);

            glRotatef(25, 1.0f, 0.0f, 0.0f);

            glTranslatef(0, -0.33f, -1.f);

            float norm = std::sqrt(x*x + y*y + z*z);

            glRotatef(-135, 0.0f, 1.0f, 0.0f);

            draw_axes();

            draw_circle(1, 0, 0, 0, 1, 0);
            draw_circle(0, 1, 0, 0, 0, 1);
            draw_circle(1, 0, 0, 0, 0, 1);

            const auto canvas_size = 230;
            const auto vec_threshold = 0.2f;
            if (norm < vec_threshold)
            {
                const auto radius = 0.05;
                static const int circle_points = 100;
                static const float angle = 2.0f * 3.1416f / circle_points;

                glColor3f(1.0f, 1.0f, 1.0f);
                glBegin(GL_POLYGON);
                double angle1 = 0.0;
                glVertex2d(radius * cos(0.0), radius * sin(0.0));
                int i;
                for (i = 0; i < circle_points; i++)
                {
                    glVertex2d(radius * cos(angle1), radius *sin(angle1));
                    angle1 += angle;
                }
                glEnd();
            }
            else
            {
                auto vectorWidth = 5.f;
                glLineWidth(vectorWidth);
                glBegin(GL_LINES);
                glColor3f(1.0f, 1.0f, 1.0f);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(x / norm, y / norm, z / norm);
                glEnd();

                // Save model and projection matrix for later
                GLfloat model[16];
                glGetFloatv(GL_MODELVIEW_MATRIX, model);
                GLfloat proj[16];
                glGetFloatv(GL_PROJECTION_MATRIX, proj);

                glLoadIdentity();
                glOrtho(-canvas_size, canvas_size, -canvas_size, canvas_size, -1, +1);

                std::ostringstream s1;
                const auto precision = 3;

                s1 << std::setprecision(precision) << norm;
                print_text_in_3d(x / 2, y / 2, z / 2, s1.str().c_str(), true, model, proj, 1 / norm);
            }

            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 768, 768, 0);

            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
        }


        void draw_grid(float step)
        {
            glLineWidth(1);
            glBegin(GL_LINES);
            glColor4f(0.1f, 0.1f, 0.1f, 0.8f);
            
            for (float x = -1.5; x < 1.5; x += step)
            {
                for (float y = -1.5; y < 1.5; y += step)
                {
                    glVertex3f(x, y, 0);
                    glVertex3f(x + step, y, 0);
                    glVertex3f(x + step, y, 0);
                    glVertex3f(x + step, y + step, 0);
                }
            }
            glEnd();
        }

        void draw_pose_data(const rs2_pose& pose, int id)
        {
            //TODO: use id if required to keep track of some state
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();

            glViewport(0, 0, 1024, 1024);

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            draw_grid(1.f);
            draw_axes(0.3f, 2.f);

            // Drawing pose:
            matrix4 pose_trans = tm2_pose_to_world_transformation(pose);
            float model[16];
            pose_trans.to_column_major(model);

            // set the pose transformation as the model matrix to draw the axis
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadMatrixf(model);

            draw_axes(0.3f, 2.f);

            // remove model matrix from the rest of the render
            glPopMatrix();

            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 1024, 1024, 0);

            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
        }

        bool try_pick(int x, int y, float* result)
        {
            auto image = get_last_frame().as<video_frame>();
            if (!image) return false;

            auto format = image.get_profile().format();
            switch (format)
            {
            case RS2_FORMAT_Z16:
            case RS2_FORMAT_Y16:
            case RS2_FORMAT_DISPARITY16:
            {
                auto ptr = (const uint16_t*)image.get_data();
                *result = ptr[y * (image.get_stride_in_bytes() / sizeof(uint16_t)) + x];
                return true;
            }
            case RS2_FORMAT_DISPARITY32:
            {
                auto ptr = (const float*)image.get_data();
                *result = ptr[y * (image.get_stride_in_bytes() / sizeof(float)) + x];
                return true;
            }
            case RS2_FORMAT_RAW8:
            case RS2_FORMAT_Y8:
            {
                auto ptr = (const uint8_t*)image.get_data();
                *result = ptr[y * image.get_stride_in_bytes() + x];
                return true;
            }
            default:
                return false;
            }
        }

        void draw_texture(const rect& s, const rect& t) const
        {
            glBegin(GL_QUAD_STRIP);
            {
                glTexCoord2f(s.x, s.y + s.h); glVertex2f(t.x, t.y + t.h);
                glTexCoord2f(s.x, s.y); glVertex2f(t.x, t.y);
                glTexCoord2f(s.x + s.w, s.y + s.h); glVertex2f(t.x + t.w, t.y + t.h);
                glTexCoord2f(s.x + s.w, s.y); glVertex2f(t.x + t.w, t.y);
            }
            glEnd();
        }

        void show(const rect& r, float alpha, const rect& normalized_zoom = rect{0, 0, 1, 1}) const
        {
            glEnable(GL_BLEND);

            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glBegin(GL_QUADS);
            glColor4f(1.0f, 1.0f, 1.0f, 1 - alpha);
            glEnd();

            glBindTexture(GL_TEXTURE_2D, get_gl_handle());

            glEnable(GL_TEXTURE_2D);
            draw_texture(normalized_zoom, r);

            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            glDisable(GL_BLEND);
        }

        void show_preview(const rect& r, const rect& normalized_zoom = rect{0, 0, 1, 1})
        {
            glBindTexture(GL_TEXTURE_2D, get_gl_handle());

            glEnable(GL_TEXTURE_2D);

            // Show stream thumbnail
            static const rect unit_square_coordinates{0, 0, 1, 1};
            static const float2 thumbnail_size = {141, 141};
            static const float2 thumbnail_margin = { 10, 27 };
            rect thumbnail{r.x + r.w, r.y + r.h, thumbnail_size.x, thumbnail_size.y };
            thumbnail = thumbnail.adjust_ratio({r.w, r.h}).enclose_in(r.shrink_by(thumbnail_margin));
            rect zoomed_rect = normalized_zoom.unnormalize(r);

            if (r != zoomed_rect)
            {
                draw_texture(unit_square_coordinates, thumbnail);

                // Draw thumbnail border
                static const auto top_line_offset = 0.5f;
                static const auto right_line_offset = top_line_offset / 2;
                glColor3f(0.0, 0.0, 0.0);
                glBegin(GL_LINE_LOOP);
                glVertex2f(thumbnail.x - top_line_offset, thumbnail.y - top_line_offset);
                glVertex2f(thumbnail.x + thumbnail.w + right_line_offset / 2, thumbnail.y - top_line_offset);
                glVertex2f(thumbnail.x + thumbnail.w + right_line_offset / 2, thumbnail.y + thumbnail.h + top_line_offset);
                glVertex2f(thumbnail.x - top_line_offset, thumbnail.y + thumbnail.h + top_line_offset);
                glEnd();

                curr_preview_rect = thumbnail;
                zoom_preview = true;
            }
            else
            {
                zoom_preview = false;
            }

            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            if (r != zoomed_rect)
            {
                // Draw ROI
                auto normalized_thumbnail_roi = normalized_zoom.unnormalize(thumbnail);
                glLineWidth(1);
                glBegin(GL_LINE_STRIP);
                glColor4f(1,1,1,1);
                glVertex2f(normalized_thumbnail_roi.x, normalized_thumbnail_roi.y);
                glVertex2f(normalized_thumbnail_roi.x, normalized_thumbnail_roi.y + normalized_thumbnail_roi.h);
                glVertex2f(normalized_thumbnail_roi.x + normalized_thumbnail_roi.w, normalized_thumbnail_roi.y + normalized_thumbnail_roi.h);
                glVertex2f(normalized_thumbnail_roi.x + normalized_thumbnail_roi.w, normalized_thumbnail_roi.y);
                glVertex2f(normalized_thumbnail_roi.x, normalized_thumbnail_roi.y);
                glEnd();
            }
        }
    };

    // Helper class that lets smoothly animate between its values
    template<class T>
    class animated
    {
    private:
        T _old, _new;
        std::chrono::system_clock::time_point _last_update;
        std::chrono::system_clock::duration _duration;
    public:
        animated(T def, std::chrono::system_clock::duration duration = std::chrono::milliseconds(200))
            : _duration(duration), _old(def), _new(def)
        {
            _last_update = std::chrono::system_clock::now();
        }
        animated& operator=(const T& other)
        {
            if (other != _new)
            {
                _old = get();
                _new = other;
                _last_update = std::chrono::system_clock::now();
            }
            return *this;
        }
        T get() const
        {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now - _last_update).count();
            auto duration_ms = std::chrono::duration_cast<std::chrono::microseconds>(_duration).count();
            auto t = (float)ms / duration_ms;
            t = std::max(0.f, std::min(rs2::smoothstep(t, 0.f, 1.f), 1.f));
            return _old * (1.f - t) + _new * t;
        }
        operator T() const { return get(); }
        T value() const { return _new; }
    };

    inline bool is_integer(float f)
    {
        return (fabs(fmod(f, 1)) < std::numeric_limits<float>::min());
    }

    struct to_string
    {
        std::ostringstream ss;
        template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };

    inline std::string error_to_string(const error& e)
    {
        return to_string() << rs2_exception_type_to_string(e.get_type())
            << " in " << e.get_failed_function() << "("
            << e.get_failed_args() << "):\n" << e.what();
    }

    inline std::string api_version_to_string(int version)
    {
        if (version / 10000 == 0) return to_string() << version;
        return to_string() << (version / 10000) << "." << (version % 10000) / 100 << "." << (version % 100);
    }

    // Comparing parameter against a range of values of the same type
    // https://stackoverflow.com/questions/15181579/c-most-efficient-way-to-compare-a-variable-to-multiple-values
    template <typename T>
    bool val_in_range(const T& val, const std::initializer_list<T>& list)
    {
        for (const auto& i : list) {
            if (val == i) {
                return true;
            }
        }
        return false;
    }

    inline bool is_rasterizeable(rs2_format format)
    {
        // Check whether the produced
        switch (format)
        {
            case RS2_FORMAT_ANY:
            case RS2_FORMAT_XYZ32F:
            case RS2_FORMAT_MOTION_RAW:
            case RS2_FORMAT_MOTION_XYZ32F:
            case RS2_FORMAT_GPIO_RAW:
            case RS2_FORMAT_6DOF:
                return false;
            default:
                return true;
        }
    }

    inline float to_rad(float deg)
    {
        return static_cast<float>(deg * (M_PI / 180.f));
    }

    inline matrix4 identity_matrix()
    {
        matrix4 data;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                data.mat[i][j] = (i == j) ? 1.f : 0.f;
        return data;
    }

    // Single-Wave - helper function that smoothly goes from 0 to 1 between 0 and 0.5,
    // and then smoothly returns from 1 to 0 between 0.5 and 1.0, and stays 0 anytime after
    // Useful to animate variable on and off based on last time something happened
    inline float single_wave(float x)
    {
        auto c = clamp(x, 0.f, 1.f);
        return 0.5f * (sinf(2.f * float(M_PI) * c - float(M_PI_2)) + 1.f);
    }

    // convert 3d points into 2d viewport coordinates
    inline	float2 translate_3d_to_2d(float3 point, matrix4 p, matrix4 v, matrix4 f, int32_t vp[4])
    {
        //
        // retrieve model view and projection matrix
        //
        //   RS2_GL_MATRIX_CAMERA contains the model view matrix
        //   RS2_GL_MATRIX_TRANSFORMATION is identity matrix
        //   RS2_GL_MATRIX_PROJECTION is the projection matrix
        //
        // internal representation is in column major order, i.e., 13th, 14th, and 15th elelments
        // of the 16 element model view matrix represents translations
        //   float mat[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        //
        //   rs2::matrix4 m = { {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} };
        //       0     1     2     3
        //       4     5     6     7
        //       8     9    10    11
        //       12   13    14    15
        //
        // when use matrix4 in glUniformMatrix4fv, transpose option is GL_FALSE so data is passed into
        // shader as column major order
        //
        //			rs2::matrix4 p = get_matrix(RS2_GL_MATRIX_PROJECTION);
        //			rs2::matrix4 v = get_matrix(RS2_GL_MATRIX_CAMERA);
        //			rs2::matrix4 f = get_matrix(RS2_GL_MATRIX_TRANSFORMATION);

        // matrix * operation in column major, transpose matrix
        //   0   4    8   12
        //   1   5    9   13
        //   2   6   10   14
        //   3   7   11   15
        //
        matrix4 vc;
        matrix4 pc;
        matrix4 fc;

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                pc(i, j) = p(j, i);
                vc(i, j) = v(j, i);
                fc(i, j) = f(j, i);
             }
        }

        // obtain the final transformation matrix
        auto mvp = pc * vc * fc;

        // test - origin (0, 0, -1.0, 1) should be translated into (0, 0, 0, 0) at this point
        //float4 origin{ 0.f, 0.f, -1.f, 1.f };

        // translate 3d vertex into 2d windows coordinates
        float4 p3d;
        p3d.x = point.x;
        p3d.y = point.y;
        p3d.z = point.z;
        p3d.w = 1.0;

        // transform from object coordinates into clip coordinates
        float4 p2d = mvp * p3d;

        // clip to [-w, w] and normalize
        if (abs(p2d.w) > 0.0)
        {
            p2d.x /= p2d.w;
            p2d.y /= p2d.w;
            p2d.z /= p2d.w;
            p2d.w /= p2d.w;
        }

        p2d.x = clamp(p2d.x, -1.0, 1.0);
        p2d.y = clamp(p2d.y, -1.0, 1.0);
        p2d.z = clamp(p2d.z, -1.0, 1.0);

        // viewport coordinates
        float x_vp = round((p2d.x + 1.f) / 2.f * vp[2]) + vp[0];
        float y_vp = round((p2d.y + 1.f) / 2.f * vp[3]) + vp[1];

        float2 p_w;
        p_w.x = x_vp;
        p_w.y = y_vp;

        return p_w;
    }
}
