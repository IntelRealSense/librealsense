// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#pragma once

#include "device-model.h"

#include <vector>
#include <functional>
#include <string>
#include <set>

namespace rs2
{
    struct interest_point
    {
        float3 pos;
        float3 normal;

        bool operator==(const interest_point& other) const
        {
            return (pos - other.pos).length() < 0.001f;
        }
    };

    struct measurement_state
    {
        std::vector<interest_point> points;
        std::vector<std::pair<int, int>> edges;
        std::vector<std::vector<int>> polygons;

        bool operator==(const measurement_state& other) const
        {
            return points == other.points && edges == other.edges && polygons == other.polygons;
        }

        std::vector<int> find_path(int from, int to);
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

        void commit_state();
        void restore_state();

        float calculate_area(std::vector<float3> points);

        std::string length_to_string(float distance);
        std::string area_to_string(float area);
        rs2::float2 project_to_2d(rs2::float3 pos);
        void draw_label(ux_window& win, float3 pos, float distance, int height, bool is_area = false);
        void draw_ruler(ux_window& win, float3 from, float3 to, float height, int selected);
        void draw(ux_window& win);

        void show_tooltip(ux_window& win);
        void mouse_pick(ux_window& win, float3 picked, float3 normal);
        void update_input(ux_window& win, const rs2::rect& viewer_rect);

        std::function<void(std::string)> log_function = [](std::string) {};
        std::function<bool()> is_metric = [](){ return true; };

    private:
        int point_hovered(ux_window& win);
        int edge_hovered(ux_window& win);

        interest_point selection_point;
        
        measurement_state state;
        std::deque<measurement_state> state_history;

        bool dragging_measurement_point = false;
        int  dragging_point_index = -1;
        bool measurement_point_hovered = false;
        bool measurement_active = false;
        int  hovered_edge_id = -1;
        int  last_hovered_point = -1;
        int  current_hovered_point = -1;

        temporal_event mouse_picked_event { std::chrono::milliseconds(1000) };
        float3 _normal, _picked;

        struct mouse_control
        {
            bool mouse_down = false;
            bool prev_mouse_down = false;
            bool click = false;
            double selection_started = 0.0;
            float2 down_pos { 0.f, 0.f };
            int mouse_wheel = 0;
            float click_time = 0.f;
            float click_period() { return clamp(float(glfwGetTime() - click_time) * 10, 0.f, 1.f); }
        };
        mouse_control input_ctrl;
        int id = 0;
    };
}