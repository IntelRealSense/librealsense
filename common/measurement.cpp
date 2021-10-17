// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "measurement.h"
#include "ux-window.h"

#include <librealsense2/hpp/rs_export.hpp>

#include "opengl3.h"


using namespace rs2;

void measurement::enable() {
    measurement_active = true;
    config_file::instance().set(configurations::viewer::is_measuring, true);
}
void measurement::disable() {
    state.points.clear();
    state.edges.clear();
    state.polygons.clear();
    measurement_active = false;
    config_file::instance().set(configurations::viewer::is_measuring, false);
}
bool measurement::is_enabled() const { return measurement_active; }

bool measurement::display_mouse_picked_tooltip() const{
    return !(measurement_active && state.points.size() == 1) && !measurement_point_hovered;
}

bool measurement::manipulating() const { return dragging_measurement_point; }

std::vector<int> measurement_state::find_path(int from, int to)
{
    std::map<int, int> parents;
    std::deque<int> q;

    q.push_back(from);
    for (int i = 0; i < points.size(); i++)
    {
        parents[i] = -1;
    }

    while (q.size())
    {
        auto vert = q.front();
        q.pop_front();

        for (auto&& pair : edges)
        {
            int next = -1;
            if (pair.first == vert) next = pair.second;
            if (pair.second == vert) next = pair.first;
            if (next >= 0 && parents[next] == -1)
            {
                parents[next] = vert;
                q.push_back(next);
            }
        }
    }

    std::vector<int> path;
    parents[from] = -1;

    if (parents[to] != -1)
    {
        auto p = to;
        while (p != -1)
        {
            path.push_back(p);
            p = parents[p];
        }
    }

    return path;
}

void measurement::add_point(interest_point p)
{
    auto shift = ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_SHIFT);

    if (is_enabled())
    {
        commit_state();

        if (state.points.size() >= 2 && !shift)
        {
            state.points.clear();
            state.edges.clear();
            state.polygons.clear();
        }

        int last = int(state.points.size());
        if (current_hovered_point == -1 ||
            current_hovered_point >= state.points.size())
        {
            state.points.push_back(p);
        }
        else
        {
            last = current_hovered_point;
        }

        int prev = last - 1;
        if (last_hovered_point >= 0 && last_hovered_point < state.points.size())
            prev = last_hovered_point;

        if (state.edges.size())
        {
            auto path = state.find_path(prev, last);
            if (path.size())
            {
                state.polygons.push_back(path);

                std::vector<float3> poly;
                for (auto&& idx : path) poly.push_back(state.points[idx].pos);

                auto area = calculate_area(poly);
                log_function(to_string() << "Measured area of " << area_to_string(area));
            }
        }

        if (state.points.size() >= 2)
        {
            auto dist = state.points[last].pos - state.points[prev].pos;
            state.edges.push_back(std::make_pair(last, prev));
            log_function(to_string() << "Measured distance of " << length_to_string(dist.length()));
        }

        last_hovered_point = int(state.points.size() - 1);

        commit_state();
    }
}

std::string measurement::area_to_string(float area)
{
    return to_string() << std::setprecision(2) << area << " m";
}

std::string measurement::length_to_string(float distance)
{
    std::string label;
    if (is_metric())
    {
        if (distance < 0.01f)
        {
            label = to_string() << std::setprecision(3) << distance * 1000.f << " mm";
        } else if (distance < 1.f) {
            label = to_string() << std::setprecision(3) << distance * 100.f << " cm";
        } else {
            label = to_string() << std::setprecision(3) << distance << " m";
        }
    } else
    {
        if (distance < 0.0254f)
        {
            label = to_string() << std::setprecision(3) << distance * 1000.f << " mm";
        } else if (distance < 0.3048f) {
            label = to_string() << std::setprecision(3) << distance / 0.0254 << " in";
        } else if (distance < 0.9144) {
            label = to_string() << std::setprecision(3) << distance / 0.3048f << " ft";
        } else {
            label = to_string() << std::setprecision(3) << distance / 0.9144 << " yd";
        }
    }
    return label;
}

float measurement::calculate_area(std::vector<float3> poly)
{
    if (poly.size() < 3) return 0.f;

    float3 total{ 0.f, 0.f, 0.f };

    for (int i = 0; i < poly.size(); i++)
    {
        auto v1 = poly[i];
        auto v2 = poly[(i+1) % poly.size()];
        auto prod = cross(v1, v2);
        total = total + prod;
    }

    auto a = poly[1] - poly[0];
    auto b = poly[2] - poly[0];
    auto n = cross(a, b);
    return abs(total * n.normalize()) / 2;
}

void draw_sphere(const float3& pos, float r, int lats, int longs)
{
    for(int i = 0; i <= lats; i++)
    {
        float lat0 = float(M_PI) * (-0.5f + (float) (i - 1) / lats);
        float z0  = sin(lat0);
        float zr0 =  cos(lat0);

        float lat1 = float(M_PI) * (-0.5f + (float) i / lats);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for(int j = 0; j <= longs; j++)
        {
            float lng = 2.f * float(M_PI) * (float) (j - 1) / longs;
            float x = cos(lng);
            float y = sin(lng);

            glNormal3f(pos.x + x * zr0, pos.y + y * zr0, pos.z + z0);
            glVertex3f(pos.x + r * x * zr0, pos.y + r * y * zr0, pos.z + r * z0);
            glNormal3f(pos.x + x * zr1, pos.y + y * zr1, pos.z + z1);
            glVertex3f(pos.x + r * x * zr1, pos.y + r * y * zr1, pos.z + r * z1);
        }
        glEnd();
    }
}

rs2::float2 measurement::project_to_2d(rs2::float3 pos)
{
    int32_t vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    check_gl_error();

    GLfloat model[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, model);
    GLfloat proj[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    rs2::matrix4 p(proj);
    rs2::matrix4 v(model);

    return translate_3d_to_2d(pos, p, v, rs2::matrix4::identity(), vp);
}

void measurement::draw_label(ux_window& win, float3 pos, float distance, int height, bool is_area)
{
    auto w_pos = project_to_2d(pos);
    std::string label = is_area ? area_to_string(distance) : length_to_string(distance);
    if (is_area) ImGui::PushFont(win.get_large_font());
    auto size = ImGui::CalcTextSize(label.c_str());
    if (is_area) ImGui::PopFont();

    std::string win_id = to_string() << "measurement_" << id;
    id++;

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if (!is_area)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, regular_blue);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, almost_white_bg);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
    ImGui::SetNextWindowPos(ImVec2(w_pos.x - size.x / 2, height - w_pos.y - size.y / 2 - 5));
    ImGui::SetNextWindowSize(ImVec2(size.x + 20, size.y - 15));
    ImGui::Begin(win_id.c_str(), nullptr, flags);

    if (is_area) ImGui::PushFont(win.get_large_font());
    ImGui::Text("%s", label.c_str());
    if (is_area) {
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7);
        ImGui::Text("%s", "2");
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void measurement::draw_ruler(ux_window& win, float3 from, float3 to, float height, int selected)
{
    std::vector<float3> parts;
    parts.push_back(from);
    auto dir = to - from;
    auto l = dir.length();
    auto unit = is_metric() ? 0.01f : 0.0254f;
    if (l > 0.5f) unit = is_metric() ? 0.1f : 0.03048f;
    auto parts_num = l / unit;
    for (int i = 0; i < parts_num; i++)
    {
        auto t = i / (float)parts_num;
        parts.push_back(from + dir * t);
    }
    parts.push_back(to);

    glLineWidth(3.f);
    glBegin(GL_LINES);
    for (int i = 1; i < parts.size(); i++)
    {
        auto alpha = selected == 0 ? 0.4f : 0.9f;
        alpha = selected == 2 ? 1.f : alpha;
        if (i % 2 == 0) glColor4f(1.f, 1.f, 1.f, alpha);
        else glColor4f(light_blue.x, light_blue.y, light_blue.z, alpha);
        auto from = parts[i-1];
        auto to = parts[i]; // intentional shadowing
        glVertex3d(from.x, from.y, from.z);
        glVertex3d(to.x, to.y, to.z);
    }
    glEnd();

    if (selected == 2)
    {
        // calculate center of the ruler line
        float3 ctr = from + (to - from) / 2;
        float distance = (to - from).length();
        draw_label(win, ctr, distance, int(height));
    }
}

void measurement::mouse_pick(ux_window& win, float3 picked, float3 normal)
{
    _picked = picked; _normal = normal;
    mouse_picked_event.add_value(!input_ctrl.mouse_down);

    if (input_ctrl.click) {
        add_point({ _picked, _normal });
    }

    if (is_enabled())
    {
        if (point_hovered(win) < 0 && hovered_edge_id < 0)
            win.cross_hovered();
    }
}

void measurement::update_input(ux_window& win, const rs2::rect& viewer_rect)
{
    id = 0;

    if (ImGui::IsKeyPressed('Z') || ImGui::IsKeyPressed('z'))
        restore_state();

    input_ctrl.prev_mouse_down = input_ctrl.mouse_down;

    auto rect_copy = viewer_rect;
    rect_copy.y += 60;
    input_ctrl.click = false;
    if (win.get_mouse().mouse_down[0] && !input_ctrl.mouse_down)
    {
        input_ctrl.mouse_down = true;
        input_ctrl.down_pos = win.get_mouse().cursor;
        input_ctrl.selection_started = win.time();
    }
    if (input_ctrl.mouse_down && !win.get_mouse().mouse_down[0])
    {
        input_ctrl.mouse_down = false;
        if (win.time() - input_ctrl.selection_started < 0.5 &&
            (win.get_mouse().cursor - input_ctrl.down_pos).length() < 100)
        {
            if (rect_copy.contains(win.get_mouse().cursor))
            {
                input_ctrl.click = true;
                input_ctrl.click_time = float(glfwGetTime());
            }
        }
    }
}

int measurement::point_hovered(ux_window& win)
{
    for (int i = 0; i < state.points.size(); i++)
    {
        auto&& point = state.points[i];
        auto pos_2d = project_to_2d(point.pos);
        pos_2d.y = win.framebuf_height() - pos_2d.y;

        if ((pos_2d - win.get_mouse().cursor).length() < 15)
        {
            return i;
        }
    }
    return -1;
}

float distance_to_line(rs2::float2 a, rs2::float2 b, rs2::float2 p)
{
    const float l2 = dot(b - a, b - a);
    if (l2 == 0.0) return (p - a).length();
    const float t = clamp(dot(p - a, b - a) / l2, 0.f, 1.f);
    return (lerp(a, b, t) - p).length();
}

int measurement::edge_hovered(ux_window& win)
{
    for (int i = 0; i < state.edges.size(); i++)
    {
        auto&& a = state.points[state.edges[i].first];
        auto&& b = state.points[state.edges[i].second];

        auto a_2d = project_to_2d(a.pos);
        auto b_2d = project_to_2d(b.pos);

        auto cursor = win.get_mouse().cursor;
        cursor.y = win.framebuf_height() - cursor.y;

        if (distance_to_line(a_2d, b_2d, cursor) < 15)
        {
            return i;
        }
    }
    return -1;
}

void measurement::commit_state()
{
    state_history.push_back(state);
    if (state_history.size() > 100)
    {
        state_history.pop_front();
    }
}

void measurement::restore_state()
{
    auto new_state = state;
    while (state_history.size() && new_state == state)
    {
        new_state = state_history.back();
        state_history.pop_back();
    }
    state = new_state;
}

void measurement::draw(ux_window& win)
{
    auto shift = ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_SHIFT);

    auto p_idx = point_hovered(win);
    if (p_idx >= 0 && !win.get_mouse().mouse_down[0])
    {
        _picked = state.points[p_idx].pos;
        _normal = state.points[p_idx].normal;
    }
    if (mouse_picked_event.eval() && is_enabled())
    {
        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float size = _picked.z * 0.03f;

        glBegin(GL_LINES);
        glColor3f(1.f, 1.f, 1.f);
        glVertex3d(_picked.x, _picked.y, _picked.z);
        auto nend = _picked + _normal * size * 0.3f;
        glVertex3d(nend.x, nend.y, nend.z);
        glEnd();

        glBegin(GL_TRIANGLES);

        if (input_ctrl.mouse_down) size -= _picked.z * 0.01f;
        size += _picked.z * 0.01f * single_wave(input_ctrl.click_period());

        auto axis1 = cross(vec3d{ _normal.x, _normal.y, _normal.z }, vec3d{ 0.f, 1.f, 0.f });
        auto faxis1 = float3 { axis1.x, axis1.y, axis1.z };
        faxis1.normalize();
        auto axis2 = cross(vec3d{ _normal.x, _normal.y, _normal.z }, axis1);
        auto faxis2 = float3 { axis2.x, axis2.y, axis2.z };
        faxis2.normalize();

        matrix4 basis = matrix4::identity();
        basis(0, 0) = faxis1.x;
        basis(0, 1) = faxis1.y;
        basis(0, 2) = faxis1.z;

        basis(1, 0) = faxis2.x;
        basis(1, 1) = faxis2.y;
        basis(1, 2) = faxis2.z;

        basis(2, 0) = _normal.x;
        basis(2, 1) = _normal.y;
        basis(2, 2) = _normal.z;

        const int segments = 50;
        for (int i = 0; i < segments; i++)
        {
            auto t1 = 2.f * float(M_PI) * ((float)i / segments);
            auto t2 = 2.f * float(M_PI) * ((float)(i+1) / segments);
            float4 xy1 { cosf(t1) * size, sinf(t1) * size, 0.f, 1.f };
            xy1 = basis * xy1;
            xy1 = float4 { _picked.x + xy1.x, _picked.y + xy1.y, _picked.z  + xy1.z, 1.f };
            float4 xy2 { cosf(t1) * size * 0.5f, sinf(t1) * size * 0.5f, 0.f, 1.f };
            xy2 = basis * xy2;
            xy2 = float4 { _picked.x + xy2.x, _picked.y + xy2.y, _picked.z  + xy2.z, 1.f };
            float4 xy3 { cosf(t2) * size * 0.5f, sinf(t2) * size * 0.5f, 0.f, 1.f };
            xy3 = basis * xy3;
            xy3 = float4 { _picked.x + xy3.x, _picked.y + xy3.y, _picked.z  + xy3.z, 1.f };
            float4 xy4 { cosf(t2) * size, sinf(t2) * size, 0.f, 1.f };
            xy4 = basis * xy4;
            xy4 = float4 { _picked.x + xy4.x, _picked.y + xy4.y, _picked.z  + xy4.z, 1.f };
            //glVertex3fv(&_picked.x);

            glColor4f(white.x, white.y, white.z, 0.5f);
            glVertex3fv(&xy1.x);
            glColor4f(white.x, white.y, white.z, 0.8f);
            glVertex3fv(&xy2.x);
            glVertex3fv(&xy3.x);

            glColor4f(white.x, white.y, white.z, 0.5f);
            glVertex3fv(&xy1.x);
            glVertex3fv(&xy4.x);
            glColor4f(white.x, white.y, white.z, 0.8f);
            glVertex3fv(&xy3.x);
        }
        //glVertex3fv(&_picked.x); glVertex3fv(&end.x);
        glEnd();

        if (state.points.size() == 1 || (shift && state.points.size()))
        {
            auto p0 = (last_hovered_point >= 0 && last_hovered_point < state.points.size())
                ? state.points[last_hovered_point] : state.points.back();
            draw_ruler(win, _picked, p0.pos, win.framebuf_height(), 2);
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hovered_edge_id = edge_hovered(win);

    for (auto&& poly : state.polygons)
    {
        auto ancor = state.points[poly.front()];

        std::vector<float3> points;
        points.push_back(ancor.pos);

        auto mid = ancor.pos;

        glDisable(GL_DEPTH_TEST);

        glBegin(GL_TRIANGLES);
        glColor4f(light_blue.x, light_blue.y, light_blue.z, 0.3f);
        for (int i = 0; i < poly.size() - 1; i++)
        {
            auto b = state.points[poly[i]];
            auto c = state.points[poly[i+1]];
            glVertex3fv(&ancor.pos.x);
            glVertex3fv(&b.pos.x);
            glVertex3fv(&c.pos.x);
            mid = mid + c.pos;
            points.push_back(c.pos);
        }
        glEnd();

        glEnable(GL_DEPTH_TEST);

        mid = mid * (1.f / poly.size());

        auto area = calculate_area(points);

        draw_label(win, mid, area, int(win.framebuf_height()), true);
    }

    for (int i = 0; i < state.edges.size(); i++)
    {
        auto&& pair = state.edges[i];
        glDisable(GL_DEPTH_TEST);
        int selected = 1;
        if (hovered_edge_id >= 0)
            selected = hovered_edge_id == i ? 2 : 0;
        draw_ruler(win, state.points[pair.second].pos, state.points[pair.first].pos, win.framebuf_height(), selected);
        glEnable(GL_DEPTH_TEST);
    }

    if (win.get_mouse().mouse_down[1]) {
        commit_state();
        state.points.clear();
        state.edges.clear();
        state.polygons.clear();
    }

    for (auto&& points: state.points)
    {
        glColor4f(light_blue.x, light_blue.y, light_blue.z, 0.9f);
        draw_sphere(points.pos, 0.011f, 20, 20);
    }
    glDisable(GL_DEPTH_TEST);

    current_hovered_point = -1;
    measurement_point_hovered = false;
    int hovered_point = point_hovered(win);
    if (hovered_point >= 0)
    {
        if (!shift) last_hovered_point = hovered_point;
        current_hovered_point = hovered_point;
        if (!input_ctrl.prev_mouse_down && win.get_mouse().mouse_down[0])
        {
            dragging_point_index = hovered_point;
            measurement_point_hovered = true;
            if (input_ctrl.click_period() > 0.5f)
            {
                dragging_measurement_point = true;
            }
        }
    }

    int i = 0;
    for (auto&& points: state.points)
    {
        if (measurement_point_hovered)
            glColor4f(white.x, white.y, white.z, dragging_point_index == i ? 0.8f : 0.1f);
        else
            glColor4f(white.x, white.y, white.z, 0.6f);

        draw_sphere(points.pos, dragging_point_index == i ? 0.012f : 0.008f, 20, 20);
        i++;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    if (!win.get_mouse().mouse_down[0] || input_ctrl.click_period() < 0.5f)
    {
        if (dragging_measurement_point && state.points.size() >= 2)
        {
            dragging_measurement_point = false;
            input_ctrl.click_time = 0;

            for (auto&& e : state.edges)
            {
                if (e.first == dragging_point_index || e.second == dragging_point_index)
                {
                    auto dist = state.points[e.first].pos - state.points[e.second].pos;
                    log_function(to_string() << "Adjusted measurement to " << length_to_string(dist.length()));
                }
            }

            for (auto&& path : state.polygons)
            {
                if (std::find(path.begin(), path.end(), dragging_point_index) != path.end())
                {
                    std::vector<float3> poly;
                    for (auto&& idx : path) poly.push_back(state.points[idx].pos);

                    auto area = calculate_area(poly);
                    log_function(to_string() << "Adjusted area of " << area_to_string(area));
                }
            }

            commit_state();
        }
        dragging_point_index = -1;
    }
    if (dragging_measurement_point && dragging_point_index >= 0)
    {
        state.points[dragging_point_index].pos = _picked;
    }

    if (point_hovered(win) >= 0 || hovered_edge_id >= 0)
        win.link_hovered();
}

void measurement::show_tooltip(ux_window& win)
{
    if (mouse_picked_event.eval() && ImGui::IsWindowHovered())
    {
        if (display_mouse_picked_tooltip() && hovered_edge_id  < 0)
        {
            std::string tt = to_string() << std::fixed << std::setprecision(3)
                << _picked.x << ", " << _picked.y << ", " << _picked.z << " meters";
            ImGui::SetTooltip("%s", tt.c_str());
        }
    }
}
