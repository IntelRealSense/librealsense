// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#pragma once

#include "rendering.h"
#include "model-views.h"

#include <vector>
#include <functional>
#include <string>

namespace rs2
{
    struct interest_point
    {
        float3 pos;
        float3 normal;
    };

    class measurement
    {
    public:
        void enable();
        void disable();
        bool is_enabled() const;

        bool display_mouse_picked_tooltip() const;
        bool manipulating() const;

        void add_point(interest_point p);

        std::string length_to_string(float distance);
        rs2::float2 project_to_2d(rs2::float3 pos);
        void draw_label(float3 pos, float distance, int height);
        void draw_ruler(float3 from, float3 to, float height);
        void draw(ux_window& win);
        
        void show_tooltip();
        void mouse_pick(float3 picked, float3 normal);
        void update_input(ux_window& win, const rs2::rect& viewer_rect);

        std::function<void(std::string)> log_function = [](std::string) {};
        std::function<bool()> is_metric = [](){ return true; };
    private:
        interest_point selection_point;
        std::vector<interest_point> selected_points;

        bool dragging_measurement_point = false;
        int  dragging_point_index = -1;
        bool measurement_point_hovered = false;
        bool measurement_active = false;

        temporal_event mouse_picked_event { std::chrono::milliseconds(1000) };
        float3 _normal, _picked;

        struct mouse_control
        {
            bool mouse_down = false;
            bool click = false;
            double selection_started = 0.0;
            float2 down_pos { 0.f, 0.f };
            int mouse_wheel = 0;
            double click_time = 0.0;
            float click_period() { return clamp((glfwGetTime() - click_time) * 10, 0.f, 1.f); }
        };
        mouse_control input_ctrl;
    };
}