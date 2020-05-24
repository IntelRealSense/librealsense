// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "measurement.h"

#include <librealsense2/hpp/rs_export.hpp>

using namespace rs2;

void measurement::enable() {
    measurement_active = true;
    config_file::instance().set(configurations::viewer::is_measuring, true);
}
void measurement::disable() { 
    selected_points.clear(); 
    measurement_active = false;
    config_file::instance().set(configurations::viewer::is_measuring, false);
}
bool measurement::is_enabled() const { return measurement_active; }

bool measurement::display_mouse_picked_tooltip() const{
    return !(measurement_active && selected_points.size() == 1) && !measurement_point_hovered;
}

bool measurement::manipulating() const { return dragging_measurement_point; }

void measurement::add_point(interest_point p)
{
    if (is_enabled())
    {
        if (selected_points.size() == 2) selected_points.clear();

        selected_points.push_back(p);

        if (selected_points.size() == 2)
        {
            auto dist = selected_points[1].pos - selected_points[0].pos;
            log_function(to_string() << "Measured distance of " << length_to_string(dist.length()));
        }
    }
}

std::string measurement::length_to_string(float distance)
{
    std::string label;
    if (is_metric())
    {
        if (distance < 0.01f)
        {
            label = to_string() << std::fixed << std::setprecision(3) << distance * 1000.f << " mm";
        } else if (distance < 1.f) {
            label = to_string() << std::fixed << std::setprecision(3) << distance * 100.f << " cm";
        } else {
            label = to_string() << std::fixed << std::setprecision(3) << distance << " meters";
        }
    } else 
    {
        if (distance < 0.0254f)
        {
            label = to_string() << std::fixed << std::setprecision(3) << distance * 1000.f << " mm";
        } else if (distance < 0.3048f) {
            label = to_string() << std::fixed << std::setprecision(3) << distance / 0.0254 << " in";
        } else if (distance < 0.9144) {
            label = to_string() << std::fixed << std::setprecision(3) << distance / 0.3048f << " ft";
        } else {
            label = to_string() << std::fixed << std::setprecision(3) << distance / 0.9144 << " yd";
        }
    }
    return label;
}

void draw_sphere(const float3& pos, float r, int lats, int longs) 
{
    for(int i = 0; i <= lats; i++) 
    {
        float lat0 = M_PI * (-0.5 + (float) (i - 1) / lats);
        float z0  = sin(lat0);
        float zr0 =  cos(lat0);

        float lat1 = M_PI * (-0.5 + (float) i / lats);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for(int j = 0; j <= longs; j++) 
        {
            float lng = 2 * M_PI * (float) (j - 1) / longs;
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

void measurement::draw_label(float3 pos, float distance, int height)
{
    auto w_pos = project_to_2d(pos);
    std::string label = length_to_string(distance);
    auto size = ImGui::CalcTextSize(label.c_str());

    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::PushStyleColor(ImGuiCol_Text, regular_blue);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, almost_white_bg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
    ImGui::SetNextWindowPos(ImVec2(w_pos.x - size.x / 2, height - w_pos.y - size.y / 2 - 5));
    ImGui::SetNextWindowSize(ImVec2(size.x + 10, size.y - 15));
    ImGui::Begin("", nullptr, flags);
    ImGui::Text("%s", label.c_str());
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void measurement::draw_ruler(float3 from, float3 to, float height)
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
        if (i % 2 == 0) glColor4f(1.f, 1.f, 1.f, 0.9f);
        else glColor4f(light_blue.x, light_blue.y, light_blue.z, 0.9f);
        auto from = parts[i-1];
        auto to = parts[i]; // intentional shadowing
        glVertex3d(from.x, from.y, from.z);
        glVertex3d(to.x, to.y, to.z);
    }
    glEnd();

    // calculate center of the ruler line
    float3 ctr = from + (to - from) / 2;
    float distance = (to - from).length();
    draw_label(ctr, distance, height);
}

void measurement::mouse_pick(float3 picked, float3 normal)
{
    _picked = picked; _normal = normal;
    mouse_picked_event.add_value(!input_ctrl.mouse_down);

    if (input_ctrl.click) {
        add_point({ _picked, _normal });
    }
}

void measurement::update_input(ux_window& win, const rs2::rect& viewer_rect)
{
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
                input_ctrl.click_time = glfwGetTime();
            }
        }
    }
}

void measurement::draw(ux_window& win)
{
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

        auto end = _picked + _normal * size;
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
            auto t1 = 2 * M_PI * ((float)i / segments);
            auto t2 = 2 * M_PI * ((float)(i+1) / segments);
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

        if (selected_points.size() == 1)
        {
            auto p0 = selected_points.front();
            draw_ruler(_picked, p0.pos, win.framebuf_height());
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (selected_points.size() == 2)
    {
        glDisable(GL_DEPTH_TEST);
        draw_ruler(selected_points[1].pos, selected_points[0].pos, win.framebuf_height());
        glEnable(GL_DEPTH_TEST);
    }

    if (win.get_mouse().mouse_down[1]) selected_points.clear();

    for (auto&& points: selected_points)
    {
        glColor4f(light_blue.x, light_blue.y, light_blue.z, 0.9f);
        draw_sphere(points.pos, 0.011f, 20, 20);
    }
    glDisable(GL_DEPTH_TEST);
    int i = 0;
    measurement_point_hovered = false;
    for (auto&& points: selected_points)
    {
        auto pos_2d = project_to_2d(points.pos);
        pos_2d.y = win.framebuf_height() - pos_2d.y;

        if ((pos_2d - win.get_mouse().cursor).length() < 20)
        {
            dragging_point_index = i;
            measurement_point_hovered = true;
            if (win.get_mouse().mouse_down[0] && input_ctrl.click_period() > 0.5f)
                dragging_measurement_point = true;
        }

        glColor4f(white.x, white.y, white.z, dragging_point_index == i ? 0.8f : 0.4f);
        draw_sphere(points.pos, dragging_point_index == i ? 0.012f : 0.008f, 20, 20);

        i++;
    }
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    if (!win.get_mouse().mouse_down[0] || input_ctrl.click_period() < 0.5f)
    {
        dragging_point_index = -1;
        if (dragging_measurement_point && selected_points.size() == 2)
        {
            dragging_measurement_point = false;
            auto dist = selected_points[1].pos - selected_points[0].pos;
            log_function(to_string() << "Adjusted measurement to " << length_to_string(dist.length()));
        }
    }
    if (dragging_measurement_point)
    {
        selected_points[dragging_point_index].pos = _picked;
    }
}

void measurement::show_tooltip()
{
    if (mouse_picked_event.eval())
    {
        if (display_mouse_picked_tooltip())
        {
            std::string tt = to_string() << std::fixed << std::setprecision(3) 
                << _picked.x << ", " << _picked.y << ", " << _picked.z << " meters";
            ImGui::SetTooltip("%s", tt.c_str());
        }
    }
}