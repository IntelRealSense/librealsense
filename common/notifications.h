// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once
#include "model-views.h"
#include "fw-update-helper.h"

#include <string>
#include <functional>
#include <vector>
#include <chrono>

namespace rs2
{
    constexpr const char* recommended_fw_url = "https://downloadcenter.intel.com/download/27522/Latest-Firmware-for-Intel-RealSense-D400-Product-Family?v=t";
    constexpr const char* store_url = "https://store.intelrealsense.com/";

    class notification_data
    {
    public:
        notification_data(std::string description,
                          rs2_log_severity severity,
                          rs2_notification_category category);

        rs2_notification_category get_category() const;
        std::string get_description() const;
        double get_timestamp() const;
        rs2_log_severity get_severity() const;
    private:
        std::string _description;
        double _timestamp;
        rs2_log_severity _severity;
        rs2_notification_category _category;
    };

    struct notification_model
    {
        notification_model();
        notification_model(const notification_data& n);
        double get_age_in_ms(bool total = false) const;
        bool interacted() const;
        std::function<void()> draw(ux_window& win, int w, int y, 
            notification_model& selected, std::string& error_message);
        void draw_text(const char* msg, int x, int y, int h);
        void set_color_scheme(float t) const;
        void unset_color_scheme() const;
        void draw_progress_bar(ux_window& win, int w);
        const int get_max_lifetime_ms() const;

        std::function<void()> custom_action;

        std::shared_ptr<firmware_update_manager> update_manager = nullptr;
        int update_state = 0;
        float progress_speed = 5.f;
        std::chrono::system_clock::time_point last_progress_time;
        int last_progress = 0;
        float curr_progress_value = 0.f;
        float threshold_progress = 5.f;

        int count = 1;
        int height = 40;
        int index = 0;
        std::string message;
        double timestamp = 0.0;
        rs2_log_severity severity = RS2_LOG_SEVERITY_NONE;
        std::chrono::system_clock::time_point created_time;
        rs2_notification_category category;
        bool to_close = false; // true when user clicks on close notification

        int width = 320;
        int stack_offset = 4;
        int max_stack = 3;

        // Behaviour variables
        bool dismissed = false;
        bool expanded = false;
        bool visible = true;
        bool pinned = false;
        bool enable_dismiss = true;
        bool enable_expand = true;
        bool enable_click = false;

        float last_x, last_y;
        bool animating = false;
        std::chrono::system_clock::time_point last_moved;
        std::chrono::system_clock::time_point last_interacted;
    };

    struct notifications_model
    {
        int add_notification(const notification_data& n);
        int add_notification(const notification_data& n, 
                             std::function<void()> custom_action, 
                             bool use_custom_action = true);
        void draw(ux_window& win, int w, int h, std::string& error_message);

        void dismiss(int idx);
        void attach_update_manager(int idx, 
            std::shared_ptr<firmware_update_manager> manager, bool expanded = false);

        void foreach_log(std::function<void(const std::string& line)> action)
        {
            std::lock_guard<std::mutex> lock(m);
            for (auto&& l : log)
            {
                action(l);
            }

            auto rc = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ rc.x, rc.y + 5 });

            if (new_log)
            {
                ImGui::SetScrollPosHere();
                new_log = false;
            }
        }

        void add_log(std::string line)
        {
            std::lock_guard<std::mutex> lock(m);
            if (!line.size()) return;
            if (line[line.size() - 1] != '\n') line += "\n";
            log.push_back(line);
            new_log = true;
        }

    private:
        std::vector<notification_model> pending_notifications;
        int index = 1;
        const int MAX_SIZE = 6;
        std::mutex m;
        bool new_log = false;

        std::vector<std::string> log;
        notification_model selected;
    };
}
