// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "process-manager.h"

#include <string>
#include <functional>
#include <vector>
#include <chrono>

#include "ux-window.h"

namespace rs2
{
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

    struct notification_model : public std::enable_shared_from_this<notification_model>
    {
        notification_model();
        notification_model(const notification_data& n);
        double get_age_in_ms(bool total = false) const;
        bool interacted() const;
        std::function<void()> draw(ux_window& win, int w, int y, 
            std::shared_ptr<notification_model>& selected, std::string& error_message);
        void draw_text(const char* msg, int x, int y, int h);
        virtual void set_color_scheme(float t) const;
        void unset_color_scheme() const;
        virtual const int get_max_lifetime_ms() const;

        virtual int calc_height();
        virtual void draw_pre_effect(int x, int y) {}
        virtual void draw_content(ux_window& win, int x, int y, float t, std::string& error_message);
        virtual void draw_expanded(ux_window& win, std::string& error_message) {}

        virtual void dismiss(bool snooze) { dismissed = true; snoozed = snooze; }

        std::string get_title();

        std::function<void()> custom_action;

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
        bool forced = false;
        bool snoozed = false;
        bool enable_dismiss = true;
        bool enable_expand = true;
        bool enable_click = false;

        float last_x, last_y;
        bool animating = false;
        std::chrono::system_clock::time_point last_moved;
        std::chrono::system_clock::time_point last_interacted;
    };

    struct process_notification_model : public notification_model
    {
        process_notification_model(std::shared_ptr<process_manager> manager)
            : update_manager(manager) {}

        void draw_progress_bar(ux_window& win, int w);

        void draw_pre_effect(int x, int y) override;

        std::shared_ptr<process_manager> update_manager = nullptr;
        int update_state = 0;
        float progress_speed = 5.f;
        std::chrono::system_clock::time_point last_progress_time;
        int last_progress = 0;
        float curr_progress_value = 0.f;
        float threshold_progress = 5.f;
    };

    struct version_upgrade_model : public process_notification_model
    {
        version_upgrade_model(int version);

        void set_color_scheme(float t) const override;
        void draw_content(ux_window& win, int x, int y, float t, std::string& error_message) override;
        int calc_height() override;
        const int get_max_lifetime_ms() const override { return 40000; }

        int _version;
        bool _first = true;
    };

    struct notifications_model
    {
        std::shared_ptr<notification_model> add_notification(const notification_data& n);
        std::shared_ptr<notification_model> add_notification(const notification_data& n,
                              std::function<void()> custom_action, 
                              bool use_custom_action = true);
        void add_notification(std::shared_ptr<notification_model> model);
        void draw(ux_window& win, int w, int h, std::string& error_message);

        void foreach_log(std::function<void(const std::string& line)> action);
        void add_log(std::string line);

        void draw_snoozed_button();

        notifications_model() : last_snoozed(std::chrono::system_clock::now()) {}
        
    private:
        std::vector<std::shared_ptr<notification_model>> pending_notifications;
        std::vector<std::shared_ptr<notification_model>> snoozed_notifications;
        int index = 1;
        const int MAX_SIZE = 6;
        std::recursive_mutex m;
        bool new_log = false;

        std::vector<std::string> log;
        std::shared_ptr<notification_model> selected;
        std::chrono::system_clock::time_point last_snoozed;
    };

    inline ImVec4 saturate(const ImVec4& a, float f)
    {
        return{ f * a.x, f * a.y, f * a.z, a.w };
    }

    inline ImVec4 alpha(const ImVec4& v, float a)
    {
        return{ v.x, v.y, v.z, a };
    }
}
